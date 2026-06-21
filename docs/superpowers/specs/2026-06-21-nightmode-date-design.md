# NixieClockUltra – Nacht-Modus & Datum-Anzeige

**Datum:** 2026-06-21  
**Status:** Genehmigt

---

## Überblick

Zwei unabhängige Erweiterungen, die zusammen implementiert werden:

1. **Nacht-Modus** – Nixie-Röhren und NeoPixel gedimmt oder abgeschaltet, ausgelöst durch Zeitfenster und/oder LDR-Sensor.
2. **Datum-Anzeige** – DS1302 liefert volles Datum; Anzeige für 5 Sekunden nach jeder Slot-Animation; editierbar per Taster und Web-UI.

---

## 1. Nacht-Modus

### 1.1 Zustände

```
enum NightState { NIGHT_NORMAL, NIGHT_DIM, NIGHT_DARK };
NightState nightState = NIGHT_NORMAL;
```

### 1.2 Trigger-Logik (Priorität: Zeitbereich > LDR)

**Zeitbereich** (konfigurierbar via Web-UI):
- `nightTimeEnabled` (bool)
- `nightStart`, `nightEnd` (uint8_t, 0–23)
- `nightTimeMode` (uint8_t, 0 = DIM, 1 = DARK)
- Mitternacht-Übergang wird unterstützt: `nightStart > nightEnd` → Bereich gilt wenn `h >= nightStart || h < nightEnd`

**LDR-Sensor** (nur DIM, nie DARK):
- `ldrEnabled` (bool)
- `ldrThreshold` (uint16_t, 0–4095; höherer Wert = Dimmen bei hellerer Umgebung)
- Pin: GPIO 6 (ADC1-Channel 5, funktioniert mit WiFi)
- Abtastung alle 500 ms, 4 ADC-Samples gemittelt
- ADC-Wert ≤ ldrThreshold → Dimmen (niedriger ADC-Wert = dunkle Umgebung, LDR hochohmig, Spannung am Pin sinkt)

**Priorität:**
```
if (nightTimeEnabled && timeInRange(curHour))
    nightState = (nightTimeMode == 1) ? NIGHT_DARK : NIGHT_DIM;
else if (ldrEnabled && ldrReading <= ldrThreshold)
    nightState = NIGHT_DIM;
else
    nightState = NIGHT_NORMAL;
```

### 1.3 Nixie-Dimming (Software-PWM)

Definiert in `NixieClockUltra.ino`:

```cpp
#define NIGHT_DIM_DUTY_PCT    25   // Nixie Ein-Anteil in % (~25 % Helligkeit)
#define NIGHT_DIM_PWM_PERIOD  20   // PWM-Periode in ms (= 50 Hz)
#define NIGHT_DIM_NEO_PCT     15   // NeoPixel-Helligkeit im Dimm-Modus in % der Normalhelligkeit
```

PWM-Gate in `loop()` (nur aktiv wenn `nightState == NIGHT_DIM`):
- `nixiePwmOn` toggled via `millis()`-Vergleich
- Toggle-Zeitpunkte: `lastPwmToggle + (nixiePwmOn ? onTime : offTime)`
  - `onTime  = NIGHT_DIM_PWM_PERIOD * NIGHT_DIM_DUTY_PCT / 100`
  - `offTime = NIGHT_DIM_PWM_PERIOD - onTime`
- Bei Toggle → `nixieWrite(displayDigits)` oder `nixieWrite(blank[6]={10,…})`

Bei `NIGHT_DARK`: einmalig `nixieWrite(blank)` beim Zustandswechsel.

### 1.4 NeoPixel-Dimming

In `updateNeoPixel()` vor der Pixel-Berechnung temporäre Skalierung (kein NVS-Schreiben):
- `NIGHT_NORMAL`: `neoBright` / `colonBright` unverändert
- `NIGHT_DIM`:    `neoBright * NIGHT_DIM_NEO_PCT / 100`, `colonBright * NIGHT_DIM_NEO_PCT / 100`
- `NIGHT_DARK`:   `neoBright = 0`, `colonBright = 0` (lokal, Original bleibt erhalten)

### 1.5 Neue Datei: `night_mode.ino`

Enthält:
- Globale Variablen (Konfiguration + Zustand)
- `updateNightMode()` – wird in `loop()` aufgerufen

### 1.6 NVS-Keys (Namespace `"nixie"`)

| Key      | Typ     | Bedeutung                         |
|----------|---------|-----------------------------------|
| `ntEn`   | bool    | Zeitbereich aktiv                 |
| `ntFrom` | uint8   | Startzeit (Stunde)                |
| `ntTo`   | uint8   | Endzeit (Stunde)                  |
| `ntMode` | uint8   | 0 = DIM, 1 = DARK                 |
| `ldrEn`  | bool    | LDR aktiv                         |
| `ldrThr` | uint16  | ADC-Schwellwert (0–4095)          |

### 1.7 Web-UI – Neue Karte „🌙 Nacht-Modus"

Position: zwischen Slot-Animation und IR-Fernbedienung.

Elemente:
- Toggle: Zeitbereich aktiv
- Zahlenfeld: Von (0–23), Bis (0–23)
- Dropdown: Modus (Gedimmt / Dunkel)
- Toggle: LDR-Sensor aktiv
- Slider: Schwellwert (0–4095; 0 = nur bei Totaldunkel dimmen, 4095 = immer gedimmt)
- Badge: aktueller Zustand (Normal / Gedimmt / Dunkel) – aus `/api/status`

### 1.8 Neue API-Endpunkte

**`GET /api/nightmode`** – liefert alle Nacht-Modus-Parameter.

**`POST /api/nightmode`** – setzt Parameter:
```json
{ "ntEn": true, "ntFrom": 23, "ntTo": 7, "ntMode": 1,
  "ldrEn": true, "ldrThr": 512 }
```

**`GET /api/status`** – erhält zusätzlich `"nightState": 0/1/2`.

---

## 2. Datum-Anzeige

### 2.1 Neue globale Variablen

```cpp
uint8_t curDay = 1, curMonth = 1, curYear = 24;  // Jahr zweistellig (00–99)
```

### 2.2 RTC-Anpassungen (`rtc.ino`)

**`readRTC()`**: liest zusätzlich `Day()`, `Month()`, `Year() % 100`.

**`writeRTC()`**: ersetzt hardcodierten `2024-1-1` durch `curDay`, `curMonth`, `curYear + 2000`.

### 2.3 Datum-Anzeige-Funktion (`display.ino`)

```cpp
void setDisplayDate() {
    displayDigits[0] = curDay   / 10;
    displayDigits[1] = curDay   % 10;
    displayDigits[2] = curMonth / 10;
    displayDigits[3] = curMonth % 10;
    displayDigits[4] = curYear  / 10;
    displayDigits[5] = curYear  % 10;
    nixieWrite(displayDigits);
}
```

Format: **TT MM JJ** (Tag-Monat-Jahr zweistellig), von links nach rechts.

### 2.4 Datum-Anzeige nach Slot-Animation

Neue Variablen in `NixieClockUltra.ino`:
```cpp
bool     dateShowActive = false;
uint32_t dateShowStart  = 0;
#define  DATE_SHOW_MS   5000
```

Ablauf in `loop()`:
- Wenn `slotActive` → false wechselt: `dateShowActive = true; dateShowStart = millis();`
- Solange `dateShowActive && millis() - dateShowStart < DATE_SHOW_MS`: `setDisplayDate()` aufrufen (alle 500 ms, synchron mit RTC-Read-Takt)
- Danach: `dateShowActive = false` → zurück zur Uhrzeitanzeige

`dateShowActive` verhindert gleichzeitig, dass ein neuer Slot oder RTC-Update das Display überschreibt.

### 2.5 Taster-Editiermodus – Erweiterung

```cpp
enum EditState {
    EDIT_NONE,
    EDIT_HOUR, EDIT_MIN, EDIT_SEC,
    EDIT_DAY, EDIT_MONTH, EDIT_YEAR
};
```

Erweiterte Sequenz:
```
SET → Stunde → SET → Minute → SET → Sekunde →
SET → Tag    → SET → Monat  → SET → Jahr    → SET (speichern)
```

- Tag: 1–31, Monat: 1–12, Jahr: 0–99 (zweistellig)
- Blinken der aktiven Stelle wie bei Uhrzeit (Röhrenpaar blinkt aus)
- `writeRTC()` wird erst beim letzten SET aufgerufen

### 2.6 Web-UI – Erweiterung der Zeit-Karte

Karte umbenannt: **„⏱ Zeit & Datum stellen"**

Neue Felder:
- Tag (1–31), Monat (1–12), Jahr (0–99)

„Browser-Zeit"-Button synchronisiert zusätzlich `new Date().getDate()`, `.getMonth()+1`, `.getFullYear() % 100`.

**`POST /api/settime`** – erweitert um optionale Felder `d`, `mo`, `y` (rückwärtskompatibel; wenn nicht übergeben bleibt das Datum unverändert).

**`GET /api/status`** – erhält zusätzlich `"date": "TT.MM.JJ"`.

### 2.7 NTP-Sync

Bereits korrekt: `RtcDateTime ntpTime(year, month, day, ...)` setzt volles Datum. Nach dem Sync liest `readRTC()` das Datum zurück (sobald es date-fähig ist).

---

## 3. Dateien-Übersicht

| Datei                  | Änderung                                                         |
|------------------------|------------------------------------------------------------------|
| `NixieClockUltra.ino`  | Neue Globals, #defines, NVS-Ladevorgänge, Loop-Erweiterungen    |
| `night_mode.ino`       | **Neu** – Night-Mode-Logik, LDR-Abtastung, `updateNightMode()`  |
| `rtc.ino`              | `readRTC()` + `writeRTC()` mit Datum                            |
| `display.ino`          | `setDisplayDate()`, dateShowActive-Logik                         |
| `buttons.ino`          | EditState um EDIT_DAY/MONTH/YEAR erweitert                       |
| `neo_animation.ino`    | Temporäre Helligkeitsskalierung per NightState                   |
| `web_server.ino`       | Neue Karte, neue API-Endpunkte, Status-Erweiterungen             |

---

## 4. Nicht im Scope

- Wochentag-Anzeige
- Sommerzeit-Umschaltung (läuft bereits über NTP_TZ)
- Alarm-Funktion
- Hardware-PWM für Nixie (kann später als Option B nachgerüstet werden)
