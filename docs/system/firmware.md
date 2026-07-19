# Firmware-Dokumentation

Die Firmware ist als Arduino-Sketch für den ESP32-S3 geschrieben und in mehrere
`.ino`-Dateien aufgeteilt. Arduino verknüpft alle `.ino`-Dateien im Projektordner
automatisch zu einer einzigen Compilation Unit — globale Variablen und Funktionen
aus einer Datei sind in allen anderen sichtbar. **Wichtig:** Arduino hängt die Dateien
alphabetisch an, die Hauptdatei (`NixieClockUltra.ino`) kommt immer zuerst. Globale
Variablen, die in mehreren Dateien sichtbar sein müssen (z. B. `nightState`), müssen
deshalb in `NixieClockUltra.ino` deklariert werden.

## Modulübersicht

| Datei                | Zeilen | Inhalt                                                                  |
|----------------------|--------|-------------------------------------------------------------------------|
| `NixieClockUltra.ino`| 481    | Globals, `setup()`, `loop()`, Edit-Mode-FSM (Zeit+Datum), Nacht-Modus-Globals, Röhrentest-Globals |
| `nixie_driver.ino`   | 91     | `nixieInit()`, `nixieWrite()`, MCP23017-Abstraktion, FreeRTOS-Mutex     |
| `display.ino`        | 88     | `setDisplayTime()`, `setDisplayDate()`, `commitDigits()` (weicher Ziffernwechsel), Slot-Animation |
| `digit_fade.ino`     | 77     | `startDigitFade()`, `updateDigitFade()`, `cancelDigitFade()` — non-blocking Crossfade über HV-Dimmer-Duty |
| `buttons.ino`        | 127    | Entprell-FSM für 4 Taster, Kurz-/Langdruck, Edit-Mode Zeit+Datum       |
| `rtc.ino`            | 18     | `readRTC()`, `writeRTC()` via DS1302/ThreeWire, liest auch Tag/Monat/Jahr |
| `night_mode.ino`     | 34     | LDR-Abtastung (GPIO6, ADC1), `updateNightMode()`, Zeitbereich-Logik    |
| `hv_dimmer.ino`      | 12     | `hvDimmerInit()`, `hvDimmerSetDuty()` — LEDC-Hardware-PWM für TLP627 auf Anodenspannung |
| `neo_animation.ino`  | 107    | Rainbow, Statisch, Puls, Slot, Nacht-Modus-Dimming, Datumsanzeige-Override |
| `ir_remote.ino`      | 107    | `executeAction()`, `dispatchIRAction()`, `handleIR()`, 8 IR-Aktionen   |
| `tube_test.ino`      | 50     | `startTubeTest()`, `updateTubeTest()`, `stopTubeTest()` — non-blocking Röhrentest-State-Machine |
| `web_server.ino`     | 781    | Eingebettetes HTML/JS, alle API-Handler, WiFi-Setup, NTP, mDNS         |

Reine Interpolationsmathematik für den Crossfade liegt zusätzlich in `digit_fade_math.h`
(header-only, host-testbar ohne Arduino-Framework, siehe `test/digit_fade_math_test.cpp`).
Analog dazu liegt die reine Ziffern-/Testende-Logik des Röhrentests in `tube_test_math.h`
(siehe `test/tube_test_math_test.cpp`).

## Setup-Ablauf

Die `setup()`-Funktion läuft einmalig nach dem Start in dieser Reihenfolge:

1. Serial-Port öffnen (115200 Baud)
2. Taster-Pins konfigurieren (INPUT_PULLUP)
3. NeoPixel-Strip initialisieren, Helligkeit aus `BRIGHTNESS_LEVELS[brightLevel]` setzen
4. `hvDimmerInit()` — LEDC-PWM auf `HV_SWITCH_PIN` (GPIO7) anlegen, Duty zunächst 255
   (volle Anodenspannung)
5. NVS laden — Helligkeit, Animation, Slot-Intervall, WiFi-Zugangsdaten, IR-Codes,
   `colonAlwaysOn`, `colonStatic`, sowie Nacht-Modus-Konfiguration
   (`nightTimeEnabled`, `nightStart`, `nightEnd`, `nightTimeMode`, `ldrEnabled`, `ldrThreshold`,
   `hvDimPct`), außerdem `softFadeSecondEnabled`, `softFadeDateEnabled` und `slotSpeedPct`
6. `nixieInit()` — I²C initialisieren, alle 4 MCP23017 auf Output-Modus setzen, alle Bits 0
7. RTC lesen (`readRTC()`), Uhrzeit in `curHour/curMin/curSec` und Datum in
   `curDay/curMonth/curYear` laden
8. `setupWifi()` — DHCP-Hostname `nixieclockcs` setzen, AP starten (SSID: `NixieClockCS`,
   PW: `nixie1234`), ggf. STA verbinden (Timeout 20 s); bei STA-Verbindung NTP konfigurieren
   (`configTzTime()`) und mDNS unter `nixieclockcs.local` registrieren
9. `setupWebServer()` — alle API-Routen registrieren, `server.begin()`
10. IR-Empfänger starten (`irrecv.enableIRIn()`)
11. Fade-In-Flag setzen (`startFadeIn = true`), Röhren werden in `loop()` eingeblendet

## Wichtige Defines

```cpp
// GPIO-Belegung
#define RTC_IO_PIN   4      // DS1302 Data
#define RTC_CLK_PIN  5      // DS1302 Clock
#define RTC_CE_PIN   2      // DS1302 Chip Enable
#define BTN_SET      13
#define BTN_UP       12
#define BTN_DOWN     11
#define BTN_LIGHT    10
#define NEO_PIN      21
#define NEO_COUNT    10     // 6 Hintergrund + 4 Trennpunkte
#define IR_RECV_PIN  48
#define I2C_SDA      8
#define I2C_SCL      9

// Warmweiß-Farben (NeoPixel)
#define BG_WARM_R  255      // Hintergrund-LEDs (Pixel 0–5)
#define BG_WARM_G  130
#define BG_WARM_B    0
#define COLON_WARM_R  255   // Trennpunkt-LEDs (Pixel 6–9)
#define COLON_WARM_G  100
#define COLON_WARM_B    0

// Verhalten
#define FADE_STEPS          20    // Schritte für Fade-In/Out
#define FADE_INTERVAL_MS     2    // ms zwischen Fade-Schritten
#define EDIT_TIMEOUT_MS  15000    // 15 s Timeout im Einstellmodus
#define DATE_SHOW_MS      5000    // Datumsanzeige-Dauer nach Slot-Animation
#define TUBE_TEST_STEP_MS 2000    // Anzeigedauer je Ziffer im Röhrentest

// Nacht-Modus: NeoPixel-Skalierung (Nixie-Dimmung: siehe HV-Dimmer unten)
#define NIGHT_DIM_NEO_PCT     15  // NeoPixel-Helligkeit im Dimm-Modus in %

// LDR (night_mode.ino)
#define LDR_PIN        6          // ADC1-Kanal (GPIO6), LDR→VCC, 100kΩ→GND
#define LDR_SAMPLE_MS  500        // Abtastintervall

// HV-Dimmer (TLP627, LEDC-Hardware-PWM auf Anodenspannung)
#define HV_SWITCH_PIN   7         // GPIO → TLP627 → Anodenspannung
#define HV_PWM_FREQ_HZ  200       // Hz, LEDC 8-Bit-Auflösung (Duty 0–255)

const uint8_t BRIGHTNESS_LEVELS[4] = {10, 40, 80, 200};  // NeoPixel-Helligkeiten
```

## Globale State-Variablen

| Variable            | Typ           | Beschreibung                                                  |
|---------------------|---------------|---------------------------------------------------------------|
| `displayDigits[6]`  | `uint8_t[6]`  | Aktuell angezeigte Ziffern (0–9; 10 = blank)                 |
| `brightLevel`       | `uint8_t`     | Helligkeitsstufe 0–3 (Index in `BRIGHTNESS_LEVELS[]`)        |
| `neoBright`         | `uint8_t`     | NeoPixel-Feinwert 10–255 (Hintergrund-LEDs Pixel 0–5)        |
| `colonBright`       | `uint8_t`     | NeoPixel-Feinwert (Trennpunkt-LEDs Pixel 6–9)                |
| `neoHue`            | `uint8_t`     | Auto-inkrement für Rainbow-Farbverlauf                        |
| `animMode`          | `AnimMode`    | Aktueller Animationsmodus (s. u.)                             |
| `slotInterval`      | `SlotInterval`| Automatisches Slot-Intervall (s. u.)                         |
| `editState`         | `EditState`   | Aktueller Einstellschritt (s. u.)                             |
| `colonAlwaysOn`     | `bool`        | Trennpunkte dauerhaft an (kein Blinken)                       |
| `colonStatic`       | `bool`        | Trennpunkte statisch warmweiß                                 |
| `curHour/Min/Sec`   | `uint8_t`     | Lokal gecachte Uhrzeit                                        |
| `curDay`            | `uint8_t`     | Lokal gecachter Tag (1–31)                                    |
| `curMonth`          | `uint8_t`     | Lokal gecachter Monat (1–12)                                  |
| `curYear`           | `uint8_t`     | Lokal gecachtes Jahr, zweistellig (0–99)                      |
| `dateShowActive`    | `bool`        | Datumsanzeige nach Slot-Animation gerade aktiv                |
| `dateShowStart`     | `uint32_t`    | `millis()` beim Start der Datumsanzeige                       |
| `nightState`        | `NightState`  | Aktueller Nacht-Modus-Zustand (s. u.)                         |
| `nightTimeEnabled`  | `bool`        | Zeitbereich-Steuerung aktiv                                   |
| `nightStart/End`    | `uint8_t`     | Start-/Endstunde des Nacht-Bereichs (0–23)                    |
| `nightTimeMode`     | `uint8_t`     | 0 = Gedimmt, 1 = Dunkel                                       |
| `ldrEnabled`        | `bool`        | Lichtsensor-Steuerung aktiv                                   |
| `ldrThreshold`      | `uint16_t`    | ADC-Schwellwert (0–4095); bei ≤ Schwellwert → Dimmen          |
| `ldrReading`        | `uint16_t`    | Aktueller ADC-Messwert (wird immer alle 500 ms gelesen)       |
| `hvDimPct`          | `uint8_t`     | Röhren-Dimm-Helligkeit im Nacht-Modus in % (2–60, Default 25) |
| `irCodes[8]`        | `uint64_t[8]` | Angelernte IR-Codes, Index = IR_ACTION_SET..IR_ACTION_DATE    |
| `wifiStaConnected`  | `bool`        | STA-Verbindung aktiv                                          |
| `ntpSynced`         | `bool`        | NTP-Synchronisierung erfolgreich                              |
| `slotActive`        | `bool`        | Slot-Machine-Animation läuft                                  |
| `startFadeIn`       | `bool`        | Fade-In beim Start noch aktiv                                 |
| `softFadeSecondEnabled` | `bool`    | Weicher Ziffernwechsel im Sekundentakt aktiv                  |
| `softFadeDateEnabled`   | `bool`    | Weicher Ziffernwechsel bei Zeit/Datum-Übergang aktiv          |
| `slotSpeedPct`      | `uint8_t`     | Slot-Machine-Rollgeschwindigkeit 20–100 % (100 = Standard)    |
| `tubeTestActive`    | `bool`        | Röhrentest läuft gerade                                       |
| `tubeTestDigit`     | `uint8_t`     | Aktuell angezeigte Testziffer (0–9)                           |
| `tubeTestStepStart` | `uint32_t`    | `millis()` beim Anzeigen der aktuellen Testziffer             |

### Enumerationen

```cpp
enum AnimMode  { ANIM_RAINBOW, ANIM_STATIC, ANIM_PULSE, ANIM_COUNT };

enum SlotInterval { SLOT_OFF, SLOT_10S, SLOT_1MIN, SLOT_15MIN, SLOT_1HR };

enum EditState {
    EDIT_NONE,
    EDIT_HOUR, EDIT_MIN, EDIT_SEC,    // Uhrzeit-Felder
    EDIT_DAY, EDIT_MONTH, EDIT_YEAR   // Datum-Felder
};

enum NightState { NIGHT_NORMAL, NIGHT_DIM, NIGHT_DARK };

enum IrAction {
    IR_LEARN_NONE          = -1,
    IR_ACTION_SET          = 0,
    IR_ACTION_UP           = 1,
    IR_ACTION_DOWN         = 2,
    IR_ACTION_BRIGHTNESS   = 3,
    IR_ACTION_ANIM_NEXT    = 4,
    IR_ACTION_SLOT         = 5,
    IR_ACTION_COLON_TOGGLE = 6,
    IR_ACTION_DATE         = 7,
    IR_ACTION_COUNT        = 8
};
```

## Nacht-Modus-Logik

`updateNightMode()` wird jeden Loop-Durchlauf aufgerufen:

1. **LDR-Abtastung** (immer, alle 500 ms): 4-fache Mittelung von `analogRead(LDR_PIN)` →
   `ldrReading`. Licht = hoher ADC-Wert (LDR→VCC, 100 kΩ→GND), Dunkel = niedriger Wert.
2. **Priorität Zeitbereich:** Ist `nightTimeEnabled` und `curHour` im konfigurierten Bereich
   (Mitternachtsübergang wird korrekt behandelt) → `nightState` = DIM oder DARK.
3. **Fallback LDR:** Greift kein Zeitbereich, aber `ldrEnabled` und `ldrReading <= ldrThreshold`
   → `nightState` = NIGHT_DIM (DARK ist per LDR nicht möglich).
4. **Sonst:** `nightState` = NIGHT_NORMAL.

**HV-Anodendimmung** in `loop()`: Bei einem Wechsel von `nightState` (Guard
`nightState != prevNightState`) ruft `loop()` `hvDimmerSetDuty()` auf (`hv_dimmer.ino`):

| `nightState`   | Duty (`hvDimmerSetDuty()`)      |
|----------------|----------------------------------|
| `NIGHT_NORMAL` | 255 (volle Anodenspannung)       |
| `NIGHT_DIM`    | `hvDimPct * 255 / 100` (2–60 %)  |
| `NIGHT_DARK`   | 0 (Anodenspannung aus)           |

Die Dimmung erfolgt per LEDC-Hardware-PWM (~200 Hz) direkt auf der Anodenspannung über
einen TLP627-Optokoppler (`HV_SWITCH_PIN`, GPIO7) — nicht mehr per Software-PWM auf den
Kathoden. Ein separater Blitzschutz für Sekundenwechsel ist nicht mehr nötig, da die
Kathoden-Ansteuerung unabhängig von der Anodendimmung läuft.

## Weicher Ziffernwechsel (`digit_fade.ino`)

`commitDigits()` in `display.ino` ist die zentrale Stelle, über die alle Ziffernänderungen
laufen (`setDisplayTime()`, `setDisplayDate()` sowie die Soft-Varianten
`setDisplayTimeSoft()`/`setDisplayDateSoft()`). Sie vergleicht per `memcmp()` gegen
`displayDigits`, um unveränderte Aufrufe zu ignorieren, und entscheidet dann:

- `fadeMs == 0` oder `nightState != NIGHT_NORMAL` → sofortiger Hart-Wechsel über `nixieWrite()`
  (im Nacht-Modus kein Fade, da die Anodenspannung dort bereits gedimmt/aus ist)
- `fadeMs > 0` → `startDigitFade()` in `digit_fade.ino`

`startDigitFade()`/`updateDigitFade()` bilden eine non-blocking State-Machine (angetrieben aus
`loop()`), die den HV-Dimmer-Duty (`hvDimmerSetDuty()`) in `DIGIT_FADE_STEP_MS`-Schritten
(5 ms) erst auf `DIGIT_FADE_MIN_DUTY` (13, ≈5 %) abblendet, bei Minimalhelligkeit die
Zielziffern schreibt (`nixieWrite()`), und wieder auf 255 aufblendet. Die Dauer wird über
`fadeMs` gesteuert (je zur Hälfte Ab-/Aufblenden); aktuell fest verdrahtet auf 400 ms, siehe
`NixieClockUltra.ino` (`softFadeSecondEnabled ? 400 : 0` bzw. `softFadeDateEnabled ? 400 : 0`).
Die reine Interpolationsmathematik (`fadeDutyForStep()`) liegt in `digit_fade_math.h` und ist
per Host-Unit-Test (`test/digit_fade_math_test.cpp`) ohne Arduino-Framework testbar.

`cancelDigitFade()` schließt einen laufenden Fade sofort ab (Zielziffern schreiben, Duty
unangetastet) und wird vor `startSlotAnimation()` sowie beim Eintritt in den Edit-Modus
aufgerufen — Absicherung gegen eine Race zwischen laufendem Fade und neuer Display-Aktivität.

## Slot-Machine-Geschwindigkeit

`slotSpeedPct` (20–100 %, Standard 100) skaliert sowohl `slotRollIntervalMs` als auch die
gestaffelten Stopp-Zeitpunkte je Röhre (`slotStopMs[i]`) in `startSlotAnimation()`
(`display.ino`) — bei kleineren Werten rollen die Ziffern langsamer und die Animation dauert
insgesamt länger.

## Datumsanzeige

Nach jeder Slot-Animation (und per IR-Taste `DATUM`) wird `dateShowActive = true` gesetzt.
Für `DATE_SHOW_MS` (5000 ms) zeigt `displayDigits` das Datum im Format TT MM JJ.
In `neo_animation.ino` überschreibt ein Abschluss-Block alle Pixel: Hintergrund (0–5) und
obere Trennpunkte (6, 8) werden gelöscht, nur die unteren Trennpunkte (Pixel 7, 9) leuchten
in warmweißem `COLON_WARM_*`.

## Röhrentest (`tube_test.ino`)

Diagnose-Feature: Auf Anfrage zeigen alle 6 Röhren synchron dieselbe Ziffer, in
Schritten von 0 bis 9, je `TUBE_TEST_STEP_MS` (2000 ms) — insgesamt ~20 s. Zweck:
jede Ziffer auf jeder Röhre einmal sichtbar durchprüfen (defekte Kathode, kalter
Lötpunkt an Transistor/MCP23017), was die Slot-Machine-Animation nicht leistet
(dort steht nie jede Röhre synchron auf derselben Ziffer still).

- `startTubeTest()`: bricht konkurrierende Display-Nutzer ab (`cancelDigitFade()`,
  `slotActive = false`, `dateShowActive = false`, `editState = EDIT_NONE`),
  erzwingt `hvDimmerSetDuty(255)` unabhängig vom Nacht-Modus, schreibt Ziffer 0
  sofort hart auf alle Röhren. Erneuter Aufruf während eines laufenden Tests
  setzt ihn einfach auf Ziffer 0 zurück (kein Fehlerfall).
- `updateTubeTest()` (aus `loop()`): alle `TUBE_TEST_STEP_MS` eine Ziffer weiter,
  bis `tubeTestIsFinished()` (aus `tube_test_math.h`) nach Ziffer 9 `true`
  liefert → `stopTubeTest()`.
- `stopTubeTest()`: stellt die HV-Duty passend zum *aktuellen* `nightState`
  wieder her (identischer Switch wie im Nacht-Modus-Block in `loop()`), setzt
  `prevNightState` synchron mit (verhindert eine doppelte Duty-Anwendung im
  nächsten `loop()`-Durchlauf) und zeigt sofort wieder die aktuelle Uhrzeit an.
  Aufruf ohne laufenden Test ist ein No-op.
- Die reine Ziffern-/Testende-Logik (`tubeTestFillDigits()`, `tubeTestIsFinished()`)
  liegt in `tube_test_math.h`, host-testbar ohne Arduino-Framework (siehe
  `test/tube_test_math_test.cpp`).

**Guards in `loop()`:** Der bestehende Nacht-Modus-Duty-Block
(`nightState != prevNightState`) prüft zusätzlich `!tubeTestActive`, damit ein
Nacht-Modus-Wechsel die erzwungene volle Helligkeit während des Tests nicht
überschreibt. Ebenso pausiert `handleEditMode()` sowie der Sekundentakt-/
Slot-Trigger-Block vollständig, solange `tubeTestActive` gesetzt ist.
**Bekannte Einschränkung:** Die IR-Fernbedienung (`handleIR()`) läuft
unguardet vor diesen Prüfungen — SLOT/DATE/SET per Fernbedienung während
eines laufenden Tests können die Anzeige durcheinanderbringen bzw. den
Editiermodus nach Testende hängen lassen. Bewusst akzeptierter Edge-Case
(seltene gleichzeitige IR-Eingabe während der ~20 s Testdauer), nicht behoben.

**Web-API:** `POST /api/tubetest/start` / `POST /api/tubetest/stop` rufen
direkt `startTubeTest()`/`stopTubeTest()` aus dem Async-Webserver-Task auf —
analog zu `/api/settime` (siehe Web-API-Tabelle unten), nicht zum
Flag-only-Muster von `/api/slot`. `nixieWrite()` ist über den bestehenden
FreeRTOS-Mutex gegen den gleichzeitigen Zugriff aus `loop()` abgesichert.

## Nixie-Ansteuerung (`nixie_driver.ino`) {#nixie-ansteuerung}

Die direkte Ansteuerung ohne Multiplexing funktioniert so:

1. `nixieInit()` initialisiert den I²C-Bus und setzt alle 64 Ausgänge der 4 MCP23017
   auf Output-Modus (IODIR-Register = 0x00). Alle Ausgänge starten auf LOW (0).
2. `nixieWrite(digits[6])` berechnet für jede Röhre das zugehörige MCP-Bit über die
   Lookup-Tabelle `digitPin[tube][digit]` (Typ `DigitPin {uint8_t mcp; uint8_t bit;}`).
3. Der neue Zustand wird in `newState[4]` (Shadow-Register) zusammengestellt. Nur
   geänderte MCPs werden via I²C beschrieben (Optimierung: Vergleich mit `mcpState[]`).
4. Der Schreibzugriff ist durch einen FreeRTOS-Mutex (`nixieMutex`) geschützt, da
   `nixieWrite()` aus dem Loop-Task und aus dem Web-Server-Task aufgerufen werden kann.
5. Digit-Wert **10** → kein Bit gesetzt → Röhre ist blank (alle Kathoden HIGH).

### Tube-zu-MCP-Bit-Zuordnung

Die 60 Kathoden (6 Röhren × 10 Ziffern) verteilen sich auf die 4 MCPs wie folgt:

| Röhre | Anzeige       | MCP(en) | Bits                               |
|-------|---------------|---------|------------------------------------|
| 0     | Stundenzehner | 0 (0x20)| Bits 0–9                           |
| 1     | Stundeneiner  | 0+1     | MCP0 Bits 10–15 + MCP1 Bits 0–3   |
| 2     | Minutenzehner | 1 (0x21)| Bits 4–13                          |
| 3     | Minuteneiner  | 2 (0x22)| Bits 0–9                           |
| 4     | Sekundenzehner| 2+3     | MCP2 Bits 10–15 + MCP3 Bits 0–3   |
| 5     | Sekundeneiner | 3 (0x23)| Bits 4–13                          |

> Die genaue Bit-Reihenfolge pro Ziffer steht im `digitPin[6][10]`-Array in
> `nixie_driver.ino`. Ziffer 0 einer Röhre belegt nicht unbedingt Bit 0 des MCP —
> die Reihenfolge folgt der PCB-Verdrahtung.

## HV-Anodendimmung (`hv_dimmer.ino`)

Schaltet die Anodenspannung (~170 V) über einen TLP627-Optokoppler per LEDC-Hardware-PWM:

```cpp
void hvDimmerInit() {
  ledcAttach(HV_SWITCH_PIN, HV_PWM_FREQ_HZ, 8);
  ledcWrite(HV_SWITCH_PIN, 255);   // volle Helligkeit (Anode dauerhaft an)
}

void hvDimmerSetDuty(uint8_t duty0to255) {
  ledcWrite(HV_SWITCH_PIN, duty0to255);
}
```

`hvDimmerSetDuty()` wird ausschließlich bei einem `nightState`-Wechsel in `loop()`
aufgerufen, nicht pro Loop-Durchlauf — kein zusätzlicher CPU-Overhead im Normalbetrieb.

## Web-API

Der HTTP-Server läuft auf Port 80. Erreichbar über `192.168.4.1` (AP) bzw.
`http://nixieclockcs.local` (Heimnetz via mDNS).

Alle POST-Endpunkte erwarten JSON im Request-Body (`Content-Type: application/json`).
Alle GET-Endpunkte liefern JSON zurück.

| Pfad               | Methode | Request-Body / Antwort                                                      |
|--------------------|---------|-----------------------------------------------------------------------------|
| `/`                | GET     | Vollständiges Web-Interface (HTML/CSS/JS, eingebettet als PROGMEM)          |
| `/api/status`      | GET     | `{time, date, neoBright, animMode, slotIval, slotSpeed, colonOn, colonStatic, colonBright, slot, tubeTest, tubeTestDigit, nightState, ldrVal, wifiSta, ntpSynced}` |
| `/api/settime`     | POST    | `{"h":H,"m":M,"s":S}` + optional `{"d":D,"mo":Mo,"y":Y}` → RTC schreiben  |
| `/api/neobright`   | POST    | `{"val":10..255}` → NeoPixel-Feinwert + NVS                                |
| `/api/anim`        | POST    | `{"mode":0..2}` → Animationsmodus + NVS                                    |
| `/api/slotinterval`| POST    | `{"interval":0..4}` → Slot-Intervall (SlotInterval-Enum) + NVS             |
| `/api/colonbright` | POST    | `{"val":1..100}` → Trennpunkt-Helligkeit + NVS                             |
| `/api/colonon`     | POST    | `{"enabled":true\|false}` → Trennpunkte dauerhaft an/aus + NVS             |
| `/api/colonstatic` | POST    | `{"enabled":true\|false}` → Trennpunkte statisch warmweiß + NVS            |
| `/api/slot`        | POST    | — → Slot-Machine-Animation sofort auslösen                                  |
| `/api/slotspeed`   | POST    | `{"val":20..100}` → Slot-Machine-Rollgeschwindigkeit + NVS                  |
| `/api/tubetest/start` | POST | — → Röhrentest starten (bzw. auf Ziffer 0 zurücksetzen, falls bereits aktiv) |
| `/api/tubetest/stop`  | POST | — → Röhrentest sofort beenden (No-op, falls nicht aktiv)                    |
| `/api/softfade`    | GET     | `{sec, date}` — weicher Ziffernwechsel Sekundentakt/Datum-Übergang          |
| `/api/softfade`    | POST    | `{"sec":true\|false,"date":true\|false}` → weicher Ziffernwechsel + NVS     |
| `/api/nightmode`   | GET     | `{ntEn, ntFrom, ntTo, ntMode, hvDimPct, ldrEn, ldrThr, ldrVal, state}`     |
| `/api/nightmode`   | POST    | `{ntEn, ntFrom, ntTo, ntMode, hvDimPct, ldrEn, ldrThr}` → Nacht-Modus + NVS |
| `/api/wifi`        | GET     | `{mode, staSsid, staIp, ntp}`                                               |
| `/api/wifi`        | POST    | `{"ssid":"…","pass":"…"}` → STA-Verbindung herstellen, Neustart            |
| `/api/ir/status`   | GET     | Aktuelle IR-Code-Belegung (8 Einträge)                                      |
| `/api/ir/learn`    | POST    | `{"action":"SET"\|"UP"\|…\|"DATUM"}` → IR-Lernmodus starten (10 s Timeout)|
| `/api/ir/clear`    | POST    | `{"action":"…"}` → IR-Code löschen + NVS                                   |

## NVS-Persistenz

Alle Einstellungen werden bei Änderung sofort in den ESP32-NVS-Flash geschrieben
(über `Preferences`-Bibliothek, Namespace `nixie`). Beim Start werden sie automatisch
geladen.

| NVS-Schlüssel  | Typ     | Standardwert | Inhalt                                   |
|----------------|---------|--------------|------------------------------------------|
| `bright`       | UInt8   | 3            | Helligkeitsstufe (0–3)                   |
| `neoBright`    | UInt8   | 30           | NeoPixel-Feinwert Hintergrund            |
| `colonBright`  | UInt8   | 80           | NeoPixel-Feinwert Trennpunkte            |
| `animMode`     | UInt8   | 0            | Animationsmodus (AnimMode-Enum)          |
| `slotIval`     | UInt8   | 0            | Slot-Intervall (SlotInterval-Enum)       |
| `slotSpeed`    | UInt8   | 100          | Slot-Machine-Rollgeschwindigkeit (20–100 %) |
| `sfSecEn`      | Bool    | false        | Weicher Ziffernwechsel Sekundentakt aktiv |
| `sfDateEn`     | Bool    | false        | Weicher Ziffernwechsel Zeit/Datum aktiv  |
| `colonOn`      | Bool    | false        | Trennpunkte dauerhaft an                 |
| `colonStatic`  | Bool    | false        | Trennpunkte statisch warmweiß            |
| `ntEn`         | Bool    | false        | Nacht-Modus Zeitbereich aktiv            |
| `ntFrom`       | UInt8   | 23           | Nacht-Modus Startzeit (Stunde)           |
| `ntTo`         | UInt8   | 7            | Nacht-Modus Endzeit (Stunde)             |
| `ntMode`       | UInt8   | 0            | Nacht-Modus Typ (0=Gedimmt, 1=Dunkel)    |
| `hvDimPct`     | UInt8   | 25           | Röhren-Dimm-Helligkeit (2–60 %)          |
| `ldrEn`        | Bool    | false        | Lichtsensor-Steuerung aktiv              |
| `ldrThr`       | UInt16  | 512          | LDR-Schwellwert (0–4095)                 |
| `wifiSsid`     | String  | `""`         | Heimnetz SSID                            |
| `wifiPass`     | String  | `""`         | Heimnetz Passwort                        |
| `ir_SET`       | UInt64  | 0            | IR-Code für IR_ACTION_SET                |
| `ir_UP`        | UInt64  | 0            | IR-Code für IR_ACTION_UP                 |
| `ir_DOWN`      | UInt64  | 0            | IR-Code für IR_ACTION_DOWN               |
| `ir_BRIGHTNESS`| UInt64  | 0            | IR-Code für IR_ACTION_BRIGHTNESS         |
| `ir_ANIM_NEXT` | UInt64  | 0            | IR-Code für IR_ACTION_ANIM_NEXT          |
| `ir_SLOT`      | UInt64  | 0            | IR-Code für IR_ACTION_SLOT               |
| `ir_COLTOGGLE` | UInt64  | 0            | IR-Code für IR_ACTION_COLON_TOGGLE       |
| `ir_DATE`      | UInt64  | 0            | IR-Code für IR_ACTION_DATE               |
