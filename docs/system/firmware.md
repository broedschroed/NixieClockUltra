# Firmware-Dokumentation

Die Firmware ist als Arduino-Sketch für den ESP32-S3 geschrieben und in mehrere
`.ino`-Dateien aufgeteilt. Arduino verknüpft alle `.ino`-Dateien im Projektordner
automatisch zu einer einzigen Compilation Unit — globale Variablen und Funktionen
aus einer Datei sind in allen anderen sichtbar.

## Modulübersicht

| Datei                | Zeilen | Inhalt                                                      |
|----------------------|--------|-------------------------------------------------------------|
| `NixieClockUltra.ino`| ~360   | Globals, `setup()`, `loop()`, Edit-Mode-FSM, Slot-Animation |
| `nixie_driver.ino`   | 91     | `nixieInit()`, `nixieWrite()`, MCP23017-Abstraktion         |
| `display.ino`        | 47     | `setDisplayTime()`, Fade-In/Out-Mechanismus                 |
| `buttons.ino`        | 119    | Entprell-FSM für 4 Taster, Kurz-/Langdruck-Erkennung       |
| `rtc.ino`            | 15     | `readRTC()`, `writeRTC()` via DS1302/ThreeWire              |
| `neo_animation.ino`  | 99     | Rainbow, Statisch, Puls, Slot-Machine, Trennpunkt-Blinken  |
| `web_server.ino`     | 559    | Eingebettetes HTML/JS, alle API-Handler, WiFi-Setup, NTP   |

## Setup-Ablauf

Die `setup()`-Funktion läuft einmalig nach dem Start in dieser Reihenfolge:

1. Serial-Port öffnen (115200 Baud)
2. Taster-Pins konfigurieren (INPUT_PULLUP)
3. NeoPixel-Strip initialisieren, Helligkeit aus `BRIGHTNESS_LEVELS[brightLevel]` setzen
4. NVS laden — Helligkeit, Animation, WiFi-Zugangsdaten, IR-Codes, `colonAlwaysOn`
5. `nixieInit()` — I²C initialisieren, alle 4 MCP23017 auf Output-Modus setzen, alle Bits 0
6. RTC lesen (`readRTC()`), Uhrzeit in `curHour`, `curMin`, `curSec` laden
7. `setupWifi()` — AP starten (SSID: `NixieClock`, PW: `nixie1234`), ggf. STA verbinden; bei STA-Verbindung NTP konfigurieren (`configTzTime()`)
8. `setupWebServer()` — alle API-Routen registrieren, `server.begin()`
9. IR-Empfänger starten (`IrReceiver.begin(IR_RECV_PIN)`)
10. Fade-In-Flag setzen (`startFadeIn = true`), Röhren werden in `loop()` eingeblendet

## Wichtige Defines

```cpp
// GPIO-Belegung
#define RTC_IO_PIN   4      // DS1302 Data
#define RTC_CLK_PIN  5      // DS1302 Clock
#define RTC_CE_PIN   2      // DS1302 Chip Enable
#define BTN_SET      13
#define BTN_UP       14
#define BTN_DOWN     15
#define BTN_LIGHT    16
#define NEO_PIN      21
#define NEO_COUNT    10     // 6 Hintergrund + 4 Trennpunkte
#define IR_RECV_PIN  48
#define I2C_SDA      8
#define I2C_SCL      9

// Verhalten
#define FADE_STEPS          20    // Schritte für Fade-In/Out
#define FADE_INTERVAL_MS     2    // ms zwischen Fade-Schritten
#define EDIT_TIMEOUT_MS  15000    // 15 s Timeout im Einstellmodus

const uint8_t BRIGHTNESS_LEVELS[4] = {10, 30, 50, 80};  // NeoPixel-Helligkeiten
```

## Globale State-Variablen

| Variable          | Typ              | Beschreibung                                          |
|-------------------|------------------|-------------------------------------------------------|
| `displayDigits[6]`| `uint8_t[6]`     | Aktuell angezeigte Ziffern (0–9; 10 = blank)         |
| `brightLevel`     | `uint8_t`        | Helligkeitsstufe 0–3 (Index in `BRIGHTNESS_LEVELS[]`)|
| `neoBright`       | `uint8_t`        | NeoPixel-Feinwert 10–255 (Hintergrund-LEDs Pixel 0–5)|
| `colonBright`     | `uint8_t`        | NeoPixel-Feinwert (Trennpunkt-LEDs Pixel 6–9)        |
| `neoHue`          | `uint8_t`        | Auto-inkrement für Rainbow-Farbverlauf                |
| `animMode`        | `AnimMode`       | Aktueller Animationsmodus (s. u.)                     |
| `editState`       | `EditState`      | Aktueller Einstellschritt (s. u.)                     |
| `colonAlwaysOn`   | `bool`           | Trennpunkte dauerhaft an (kein Blinken)               |
| `colonStatic`     | `bool`           | Trennpunkte statisch (kein Blinken, Variante 2)       |
| `irCodes[7]`      | `uint64_t[7]`    | Angelernte IR-Codes, Index = IR_ACTION_SET..IR_ACTION_COLON_TOGGLE (0–6) |
| `wifiStaConnected`| `bool`           | STA-Verbindung aktiv                                  |
| `ntpSynced`       | `bool`           | NTP-Synchronisierung erfolgreich                      |
| `slotActive`      | `bool`           | Slot-Machine-Animation läuft                          |
| `startFadeIn`     | `bool`           | Fade-In beim Start noch aktiv                         |

### Enumerationen

```cpp
enum AnimMode  { ANIM_RAINBOW, ANIM_STATIC, ANIM_PULSE, ANIM_SLOTS, ANIM_COUNT };
enum EditState { EDIT_NONE, EDIT_HOUR, EDIT_MIN, EDIT_SEC };
enum IrAction  {
    IR_LEARN_NONE        = -1,
    IR_ACTION_SET        = 0,
    IR_ACTION_UP         = 1,
    IR_ACTION_DOWN       = 2,
    IR_ACTION_BRIGHTNESS = 3,
    IR_ACTION_ANIM_NEXT  = 4,
    IR_ACTION_SLOT       = 5,
    IR_ACTION_COLON_TOGGLE = 6,
    IR_ACTION_COUNT      = 7
};
```

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

Die 60 Kathoden (6 Röhren × 10 Ziffern) verteilen sich auf die 4 MCPs wie folgt.
Jeder Eintrag in der Tabelle zeigt, welcher MCP-Index (0–3, entspricht 0x20–0x23)
und welches Bit (0–15) für Röhre N, Ziffer D aktiviert wird:

| Röhre | Anzeige      | MCP(en) | Bits                               |
|-------|--------------|---------|------------------------------------|
| 0     | Stundenzehner| 0 (0x20)| Bits 0–9 (Ziffern 1–9,0 in dieser Reihenfolge) |
| 1     | Stundeneiner | 0+1     | MCP0 Bits 10–15 + MCP1 Bits 0–3   |
| 2     | Minutenzehner| 1 (0x21)| Bits 4–13                          |
| 3     | Minuteneiner | 2 (0x22)| Bits 0–9                           |
| 4     | Sekundenzehner| 2+3    | MCP2 Bits 10–15 + MCP3 Bits 0–3   |
| 5     | Sekundeneiner | 3 (0x23)| Bits 4–13                         |

> Die genaue Bit-Reihenfolge pro Ziffer steht im `digitPin[6][10]`-Array in
> `nixie_driver.ino`. Wichtig beim Schaltplan-Abgleich: Ziffer 0 einer Röhre
> belegt nicht unbedingt Bit 0 des MCP — die Reihenfolge folgt der PCB-Verdrahtung.

## Web-API

Der HTTP-Server läuft auf Port 80. Im AP-Modus ist er unter `http://192.168.4.1`
erreichbar, im STA-Modus zusätzlich unter der vom Heimrouter zugewiesenen IP.

Alle POST-Endpunkte erwarten JSON im Request-Body (`Content-Type: application/json`).
Alle GET-Endpunkte liefern JSON zurück.

| Pfad               | Methode | Request-Body                         | Antwort / Funktion                      |
|--------------------|---------|--------------------------------------|-----------------------------------------|
| `/`                | GET     | —                                    | Vollständiges Web-Interface (HTML/CSS/JS, eingebettet) |
| `/api/status`      | GET     | —                                    | `{h, m, s, brightLevel, neoBright, animMode, wifiSta, ntpSynced, colonAlwaysOn}` |
| `/api/settime`     | POST    | `{"h":H,"m":M,"s":S}`               | Zeit in DS1302 RTC schreiben            |
| `/api/bright`      | POST    | `{"level":0..3}`                     | Helligkeitsstufe setzen + NVS speichern |
| `/api/neobright`   | POST    | `{"val":10..255}`                    | NeoPixel-Feinwert setzen + NVS          |
| `/api/anim`        | POST    | `{"mode":0..3}`                      | Animationsmodus setzen + NVS            |
| `/api/colonbright` | POST    | `{"val":1..100}`                     | Trennpunkt-Helligkeit setzen            |
| `/api/colonon`     | POST    | `{"enabled":true\|false}`            | Trennpunkte dauerhaft an/aus + NVS      |
| `/api/colonstatic` | POST    | `{"enabled":true\|false}`            | Trennpunkte statisch (kein Blinken)     |
| `/api/slot`        | POST    | —                                    | Slot-Machine-Animation sofort auslösen  |
| `/api/wifi`        | GET     | —                                    | `{apIP, staIP, staConnected, ntpSynced}`|
| `/api/wifi`        | POST    | `{"ssid":"…","pass":"…"}`            | STA-Verbindung herstellen, Neustart     |
| `/api/wifi/scan`   | GET     | —                                    | Liste verfügbarer WLANs als JSON-Array  |
| `/api/ir/status`   | GET     | —                                    | Aktuelle IR-Code-Belegung (7 Einträge)  |
| `/api/ir/learn`    | POST    | `{"action":"SET"\|"UP"\|"DOWN"\|"BRIGHTNESS"\|"ANIM_NEXT"\|"SLOT"\|"COLTOGGLE"}` | IR-Lernmodus starten (Timeout 10 s) |
| `/api/ir/clear`    | POST    | `{"action":"SET"\|"UP"\|…}`          | IR-Code für Funktion löschen + NVS      |

## NVS-Persistenz

Alle Einstellungen werden bei Änderung sofort in den ESP32-NVS-Flash geschrieben
(über `Preferences`-Bibliothek, Namespace `nixie`). Beim Start werden sie automatisch
geladen.

| NVS-Schlüssel  | Typ     | Standardwert | Inhalt                              |
|----------------|---------|--------------|-------------------------------------|
| `bright`       | UInt8   | 3            | Helligkeitsstufe (0–3)              |
| `neoBright`    | UInt8   | 30           | NeoPixel-Feinwert Hintergrund       |
| `colonBright`  | UInt8   | 15           | NeoPixel-Feinwert Trennpunkte       |
| `animMode`     | UInt8   | 0            | Animationsmodus (AnimMode-Enum)     |
| `colonOn`      | Bool    | false        | Trennpunkte dauerhaft an            |
| `colonStatic`  | Bool    | false        | Kein Blinken der Trennpunkte        |
| `wifiSsid`     | String  | `""`         | Heimnetz SSID                       |
| `wifiPass`     | String  | `""`         | Heimnetz Passwort                   |
| `ir_SET`       | UInt64  | 0            | IR-Code für IR_ACTION_SET           |
| `ir_UP`        | UInt64  | 0            | IR-Code für IR_ACTION_UP            |
| `ir_DOWN`      | UInt64  | 0            | IR-Code für IR_ACTION_DOWN          |
| `ir_BRIGHTNESS`| UInt64  | 0            | IR-Code für IR_ACTION_BRIGHTNESS    |
| `ir_ANIM_NEXT` | UInt64  | 0            | IR-Code für IR_ACTION_ANIM_NEXT     |
| `ir_SLOT`      | UInt64  | 0            | IR-Code für IR_ACTION_SLOT          |
| `ir_COLTOGGLE` | UInt64  | 0            | IR-Code für IR_ACTION_COLON_TOGGLE  |
