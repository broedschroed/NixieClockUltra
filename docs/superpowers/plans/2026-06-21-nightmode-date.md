# Nacht-Modus & Datum-Anzeige – Implementierungsplan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Nixie-Uhr um zeitgesteuerten/lichtgesteuerten Nacht-Modus (Dimmen/Ausschalten) und vollständige Datum-Anzeige mit Taster/Web-UI-Editierung erweitern.

**Architecture:** Neues `night_mode.ino` berechnet jeden Loop-Durchlauf einen `NightState`-Enum aus Zeitbereich (Prio) und LDR-ADC-Wert. Nixie-Dimming erfolgt als Software-PWM via millis()-Gate in `loop()`; NeoPixel-Dimming via temporärer Skalierung in `updateNeoPixel()`. Datum-Erweiterung: `readRTC()`/`writeRTC()` um Tag/Monat/Jahr erweitert, `setDisplayDate()` zeigt 5 s nach jeder Slot-Animation das Datum; Taster-Editiersequenz um 3 Datum-Felder verlängert.

**Tech Stack:** Arduino/ESP32-S3, Adafruit NeoPixel, RtcDS1302 (Makuna), ESPAsyncWebServer, ArduinoJson 6, Preferences (NVS), Wire/MCP23017.

## Global Constraints

- ESP32-S3, Arduino-Framework
- LDR-Pin: GPIO 6 (ADC1-Channel 5, mit WiFi kompatibel)
- Nixie PWM: `#define NIGHT_DIM_DUTY_PCT 25`, `NIGHT_DIM_PWM_PERIOD 20`, `NIGHT_DIM_NEO_PCT 15`
- Datum-Anzeige: `#define DATE_SHOW_MS 5000`
- NVS-Namespace: `"nixie"` (wie bisher)
- Alle NVS-Keys: `ntEn` (bool), `ntFrom` (uint8), `ntTo` (uint8), `ntMode` (uint8), `ldrEn` (bool), `ldrThr` (uint16)
- `curYear` ist zweistellig (0–99); `writeRTC()` addiert 2000 für den DS1302
- LDR-Schaltung: LDR gegen VCC, 100 kΩ gegen GND → dunkle Umgebung = niedriger ADC-Wert → Bedingung: `ldrReading <= ldrThreshold`
- Kein Refactoring jenseits der spec; bestehende Funktionsnamen bleiben

---

## Dateiübersicht

| Datei                 | Rolle                                                                 |
|-----------------------|-----------------------------------------------------------------------|
| `NixieClockUltra.ino` | Neue Globals + #defines; NVS-Ladeblock erweitern; `loop()` erweitern |
| `night_mode.ino`      | **Neu** – `NightState`, Konfiguration, LDR-Lesen, `updateNightMode()` |
| `rtc.ino`             | `readRTC()` + `writeRTC()` mit Tag/Monat/Jahr                        |
| `display.ino`         | `setDisplayDate()`; dateShowActive-Logik in `loop()`                 |
| `buttons.ino`         | `EditState` um EDIT_DAY/MONTH/YEAR; `handleEditMode()` erweitern     |
| `neo_animation.ino`   | Temporäre Helligkeitsskalierung in `updateNeoPixel()`                |
| `web_server.ino`      | Datum-Felder + Nacht-Modus-Karte; API-Endpunkte                      |

---

## Task 1: RTC Datum-Lesen/Schreiben + globale Datum-Variablen

**Files:**
- Modify: `NixieClockUltra.ino` (Globals-Block)
- Modify: `rtc.ino`

**Interfaces:**
- Produces: `uint8_t curDay`, `curMonth`, `curYear` (global, 1-basiert für Day/Month, 0–99 für Year)
- Produces: `readRTC()` liest auch Datum; `writeRTC()` schreibt korrektes Datum

- [ ] **Schritt 1: Globale Datum-Variablen in `NixieClockUltra.ino` hinzufügen**

Im Block „GLOBALE VARIABLEN" (nach `uint8_t curHour = 0, curMin = 0, curSec = 0;`):

```cpp
uint8_t  curDay   = 1;
uint8_t  curMonth = 1;
uint8_t  curYear  = 24;   // zweistellig: 0–99
```

- [ ] **Schritt 2: `readRTC()` in `rtc.ino` um Datum erweitern**

```cpp
void readRTC() {
  if (!Rtc.IsDateTimeValid()) return;
  RtcDateTime now = Rtc.GetDateTime();
  curHour  = now.Hour();
  curMin   = now.Minute();
  curSec   = now.Second();
  curDay   = now.Day();
  curMonth = now.Month();
  curYear  = now.Year() % 100;
}
```

- [ ] **Schritt 3: `writeRTC()` in `rtc.ino` reparieren**

Alten Hardcode `2024, 1, 1` durch echte Werte ersetzen:

```cpp
void writeRTC() {
  RtcDateTime dt(2000 + curYear, curMonth, curDay, curHour, curMin, curSec);
  Rtc.SetDateTime(dt);
}
```

- [ ] **Schritt 4: Kompilieren und flashen; Serial Monitor öffnen**

Erwartete Ausgabe: `[NixieClock] Bereit.` ohne Fehler. Uhr läuft wie bisher. Datum wird intern gelesen (noch nicht angezeigt).

- [ ] **Schritt 5: Commit**

```bash
git add NixieClockUltra.ino rtc.ino
git commit -m "feat: RTC Datum-Lesen/-Schreiben; curDay/Month/Year globals"
```

---

## Task 2: Datum-Anzeige + 5-Sekunden-Fenster nach Slot

**Files:**
- Modify: `display.ino` (neue Funktion)
- Modify: `NixieClockUltra.ino` (Globals, #define, loop())

**Interfaces:**
- Consumes: `curDay`, `curMonth`, `curYear` (Task 1); `nixieWrite()`, `displayDigits[]`
- Produces: `void setDisplayDate()`, `bool dateShowActive`, `uint32_t dateShowStart`

- [ ] **Schritt 1: `setDisplayDate()` in `display.ino` hinzufügen**

Nach `updateSlotAnimation()`:

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

- [ ] **Schritt 2: Globals und #define in `NixieClockUltra.ino` hinzufügen**

Im Block „KONSTANTEN & KONFIGURATION":
```cpp
#define DATE_SHOW_MS  5000
```

Im Block „GLOBALE VARIABLEN" (nach `slotStartMs`):
```cpp
bool     dateShowActive = false;
uint32_t dateShowStart  = 0;
```

- [ ] **Schritt 3: Slot-Ende-Detektion in `loop()` einbauen**

Aktueller Code in `loop()`:
```cpp
if (slotActive) updateSlotAnimation();
```

Ersetzen durch:
```cpp
bool wasSlotActive = slotActive;
if (slotActive) updateSlotAnimation();
if (wasSlotActive && !slotActive) {
  dateShowActive = true;
  dateShowStart  = millis();
  setDisplayDate();
}
```

- [ ] **Schritt 4: Datum-Timeout-Check und RTC-Guard in `loop()` einbauen**

Nach dem Slot-Block, vor `updateNeoPixel()`:
```cpp
// Datum-Anzeige beenden
if (dateShowActive && millis() - dateShowStart >= DATE_SHOW_MS) {
  dateShowActive = false;
  setDisplayTime(curHour, curMin, curSec);
}
```

Im RTC-Abschnitt von `loop()` die Bedingung anpassen, damit RTC-Update nicht in Datum-Anzeige und aktive Slot schreibt:

```cpp
if (editState == EDIT_NONE && millis() - lastRtcRead >= 500) {
  lastRtcRead = millis();
  readRTC();

  if (curSec != lastSec) {
    lastSec = curSec;

    bool triggerSlot = false;
    switch (slotInterval) {
      case SLOT_10S:   triggerSlot = (curSec % 10 == 0); break;
      case SLOT_1MIN:  triggerSlot = (curSec == 0); break;
      case SLOT_15MIN: triggerSlot = (curSec == 0 && curMin % 15 == 0); break;
      case SLOT_1HR:   triggerSlot = (curSec == 0 && curMin == 0); break;
      default: break;
    }
    if (!slotActive && !dateShowActive) {
      if (triggerSlot) startSlotAnimation(curHour, curMin, curSec);
      else             setDisplayTime(curHour, curMin, curSec);
    }
  }
}
```

- [ ] **Schritt 5: Kompilieren, flashen, testen**

Test: Slot-Interval auf 10 Sek stellen. Nach Slot-Ende beobachten: Datum erscheint 5 Sekunden (TT MM JJ), dann Uhrzeit. Serial Monitor zeigt keine Fehler.

- [ ] **Schritt 6: Commit**

```bash
git add NixieClockUltra.ino display.ino
git commit -m "feat: Datum-Anzeige 5s nach Slot-Animation (setDisplayDate)"
```

---

## Task 3: Taster-Editiermodus um Tag/Monat/Jahr erweitern

**Files:**
- Modify: `buttons.ino`
- Modify: `NixieClockUltra.ino` (EditState-Enum)

**Interfaces:**
- Consumes: `curDay`, `curMonth`, `curYear` (Task 1); bestehende `btnSet`, `btnUp`, `btnDown`
- Produces: Erweiterte `EditState`-Enum mit EDIT_DAY, EDIT_MONTH, EDIT_YEAR

- [ ] **Schritt 1: `EditState`-Enum in `NixieClockUltra.ino` erweitern**

```cpp
enum EditState { EDIT_NONE, EDIT_HOUR, EDIT_MIN, EDIT_SEC,
                 EDIT_DAY, EDIT_MONTH, EDIT_YEAR };
```

- [ ] **Schritt 2: `handleEditMode()` in `buttons.ino` vollständig ersetzen**

```cpp
void handleEditMode() {
  if (btnSet.pressed) {
    switch (editState) {
      case EDIT_NONE:   editState = EDIT_HOUR;  break;
      case EDIT_HOUR:   editState = EDIT_MIN;   break;
      case EDIT_MIN:    editState = EDIT_SEC;   break;
      case EDIT_SEC:    editState = EDIT_DAY;   break;
      case EDIT_DAY:    editState = EDIT_MONTH; break;
      case EDIT_MONTH:  editState = EDIT_YEAR;  break;
      case EDIT_YEAR:   writeRTC(); editState = EDIT_NONE; return;
    }
    editEnterTime = millis();
  }

  if (editState != EDIT_NONE && millis() - editEnterTime > EDIT_TIMEOUT_MS) {
    writeRTC();
    editState = EDIT_NONE;
  }
  if (editState == EDIT_NONE) return;

  int delta = 0;
  if (btnUp.pressed   || btnUp.held)   delta = +1;
  if (btnDown.pressed || btnDown.held) delta = -1;

  if (delta != 0) {
    switch (editState) {
      case EDIT_HOUR:  curHour  = (curHour  + 24  + delta) % 24; break;
      case EDIT_MIN:   curMin   = (curMin   + 60  + delta) % 60; break;
      case EDIT_SEC:   curSec   = (curSec   + 60  + delta) % 60; break;
      case EDIT_DAY:   curDay   = (uint8_t)((curDay   - 1 + 31 + delta) % 31) + 1; break;
      case EDIT_MONTH: curMonth = (uint8_t)((curMonth - 1 + 12 + delta) % 12) + 1; break;
      case EDIT_YEAR:  curYear  = (uint8_t)((curYear  + 100 + delta) % 100);       break;
      default: break;
    }
    editEnterTime = millis();
  }

  bool blinkOn = ((millis() / 250) % 2 == 0);

  if (editState == EDIT_HOUR || editState == EDIT_MIN || editState == EDIT_SEC) {
    uint8_t dH = curHour, dM = curMin, dS = curSec;
    if (!blinkOn) {
      if      (editState == EDIT_HOUR) dH = 99;
      else if (editState == EDIT_MIN)  dM = 99;
      else if (editState == EDIT_SEC)  dS = 99;
    }
    displayDigits[0] = (dH == 99) ? 10 : dH / 10;
    displayDigits[1] = (dH == 99) ? 10 : dH % 10;
    displayDigits[2] = (dM == 99) ? 10 : dM / 10;
    displayDigits[3] = (dM == 99) ? 10 : dM % 10;
    displayDigits[4] = (dS == 99) ? 10 : dS / 10;
    displayDigits[5] = (dS == 99) ? 10 : dS % 10;
  } else {
    uint8_t dD = curDay, dMo = curMonth, dY = curYear;
    if (!blinkOn) {
      if      (editState == EDIT_DAY)   dD  = 99;
      else if (editState == EDIT_MONTH) dMo = 99;
      else if (editState == EDIT_YEAR)  dY  = 99;
    }
    displayDigits[0] = (dD  == 99) ? 10 : dD  / 10;
    displayDigits[1] = (dD  == 99) ? 10 : dD  % 10;
    displayDigits[2] = (dMo == 99) ? 10 : dMo / 10;
    displayDigits[3] = (dMo == 99) ? 10 : dMo % 10;
    displayDigits[4] = (dY  == 99) ? 10 : dY  / 10;
    displayDigits[5] = (dY  == 99) ? 10 : dY  % 10;
  }
  nixieWrite(displayDigits);
}
```

`handleBrightness()` bleibt unverändert.

- [ ] **Schritt 3: Kompilieren, flashen, testen**

Test: SET drücken → Std blinkt → SET → Min blinkt → SET → Sek blinkt → SET → **Tag blinkt** → SET → Monat blinkt → SET → Jahr blinkt → SET speichert. UP/DOWN ändert den jeweiligen Wert. Timeout (15 s) speichert ebenfalls.

- [ ] **Schritt 4: Commit**

```bash
git add NixieClockUltra.ino buttons.ino
git commit -m "feat: Taster-Editiermodus um Tag/Monat/Jahr erweitert"
```

---

## Task 4: Web-UI Datum-Felder + API-Erweiterung

**Files:**
- Modify: `web_server.ino`

**Interfaces:**
- Consumes: `curDay`, `curMonth`, `curYear` (Task 1); `/api/status`; `/api/settime`
- Produces: `/api/status` liefert `"date"`; `/api/settime` nimmt optionale `d`, `mo`, `y`

- [ ] **Schritt 1: `/api/status`-Handler erweitern**

In `setupWebServer()`, den bestehenden `/api/status`-Handler anpassen.  
`StaticJsonDocument<256>` → `StaticJsonDocument<384>` und zwei neue Felder ergänzen:

```cpp
server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *req) {
  StaticJsonDocument<384> doc;
  char buf[10];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", curHour, curMin, curSec);
  doc["time"]       = buf;
  doc["bright"]     = brightLevel;
  doc["neoBright"]  = neoBright;
  doc["anim"]       = (int)animMode;
  doc["slot"]       = slotActive;
  doc["slotIval"]   = (int)slotInterval;
  doc["colonOn"]    = colonAlwaysOn;
  doc["colonStatic"]= colonStatic;
  doc["colonBright"]= colonBright;
  char dateBuf[9];
  snprintf(dateBuf, sizeof(dateBuf), "%02d.%02d.%02d", curDay, curMonth, curYear);
  doc["date"]       = dateBuf;
  doc["nightState"] = (int)nightState;
  String out; serializeJson(doc, out);
  req->send(200, "application/json", out);
});
```

- [ ] **Schritt 2: `/api/settime`-Handler um Datum-Felder erweitern**

Im Body-Callback des bestehenden `/api/settime`-Handlers, nach den drei bestehenden Zeilen:

```cpp
curHour = constrain((int)doc["h"], 0, 23);
curMin  = constrain((int)doc["m"], 0, 59);
curSec  = constrain((int)doc["s"], 0, 59);
// NEU:
if (doc.containsKey("d"))  curDay   = constrain((int)doc["d"],  1, 31);
if (doc.containsKey("mo")) curMonth = constrain((int)doc["mo"], 1, 12);
if (doc.containsKey("y"))  curYear  = constrain((int)doc["y"],  0, 99);
writeRTC();
setDisplayTime(curHour, curMin, curSec);
```

- [ ] **Schritt 3: HTML – Datum-Anzeige und Datum-Felder in Karte einfügen**

Im PROGMEM-HTML-String:

**Datum-Anzeige unter der Uhr** (nach `<div class="clock" id="clkDisp">--:--:--</div>`):
```html
<div style="font-size:1.2em;color:var(--dim);letter-spacing:.1em;margin-bottom:20px" id="dateDisp">--.--.--</div>
```

**Karten-Überschrift** ändern:
```html
<h2>⏱ Zeit & Datum stellen</h2>
```

**Drei neue Zeilen** nach dem Sekunden-Feld, vor den Buttons:
```html
<div class="row"><label>Tag</label><input type="number" id="id" min="1" max="31" value="1"></div>
<div class="row"><label>Monat</label><input type="number" id="imo" min="1" max="12" value="1"></div>
<div class="row"><label>Jahr (2-stellig)</label><input type="number" id="iy" min="0" max="99" value="24"></div>
```

- [ ] **Schritt 4: JavaScript-Funktionen aktualisieren**

`setTime()` ersetzen:
```javascript
async function setTime(){
  let h  = parseInt(document.getElementById('ih').value);
  let m  = parseInt(document.getElementById('im').value);
  let s  = parseInt(document.getElementById('is').value);
  let d  = parseInt(document.getElementById('id').value);
  let mo = parseInt(document.getElementById('imo').value);
  let y  = parseInt(document.getElementById('iy').value);
  let r  = await api('/api/settime',{h,m,s,d,mo,y});
  document.getElementById('statusMsg').textContent=r.ok?'Zeit & Datum gesetzt ✓':'Fehler ✗';
}
```

`syncBrowser()` ersetzen:
```javascript
function syncBrowser(){
  let now=new Date();
  document.getElementById('ih').value  = now.getHours();
  document.getElementById('im').value  = now.getMinutes();
  document.getElementById('is').value  = now.getSeconds();
  document.getElementById('id').value  = now.getDate();
  document.getElementById('imo').value = now.getMonth()+1;
  document.getElementById('iy').value  = now.getFullYear()%100;
  setTime();
}
```

`refreshClock()` erweitern – nach dem bestehenden Block mit `d.time` usw., folgendes hinzufügen:
```javascript
if(d.date){
  let parts=d.date.split('.');
  if(parts.length===3){
    document.getElementById('id').value  = parseInt(parts[0]);
    document.getElementById('imo').value = parseInt(parts[1]);
    document.getElementById('iy').value  = parseInt(parts[2]);
    document.getElementById('dateDisp').textContent = d.date;
  }
}
```

- [ ] **Schritt 5: Kompilieren, flashen, testen**

Test: Web-UI öffnen → Datum-Felder erscheinen befüllt → Datum ändern und „Übernehmen" → Röhren zeigen das Datum an (nach nächstem Slot oder via Browser-Test mit Slot-Trigger). „Browser-Zeit" synchronisiert auch das Datum.

- [ ] **Schritt 6: Commit**

```bash
git add web_server.ino
git commit -m "feat: Web-UI Datum-Felder; /api/status liefert date+nightState"
```

---

## Task 5: Night-Mode-Kern (`night_mode.ino` + NVS + loop-Integration)

**Files:**
- Create: `night_mode.ino`
- Modify: `NixieClockUltra.ino` (Forward-Deklaration, NVS-Ladeblock, loop())

**Interfaces:**
- Consumes: `curHour` (global); `LDR_PIN` = GPIO 6
- Produces: `NightState nightState`; `void updateNightMode()`; `uint16_t ldrReading` (intern, sichtbar für Web-API in Task 8)

- [ ] **Schritt 1a: `NightState`-Enum in `NixieClockUltra.ino` eintragen**

Arduino concateniert alle .ino-Dateien alphabetisch nach dem Haupt-Sketch — der Enum muss daher in `NixieClockUltra.ino` stehen (wie `AnimMode`, `EditState`, `SlotInterval`), damit er beim Kompilieren der Hauptdatei bereits bekannt ist.

Im Block „GLOBALE VARIABLEN", nach `enum SlotInterval { ... };`:
```cpp
enum NightState { NIGHT_NORMAL, NIGHT_DIM, NIGHT_DARK };
```

- [ ] **Schritt 1b: `night_mode.ino` erstellen**

```cpp
// ═══════════════════════════════════════════════════════════
//  NACHT-MODUS – LDR + Zeitbereich
//  LDR-Schaltung: LDR→VCC, 100kΩ→GND, ADC an GPIO 6
//  Dunkle Umgebung = niedriger ADC-Wert
// ═══════════════════════════════════════════════════════════

#define LDR_PIN        6
#define LDR_SAMPLE_MS  500

// NightState-Enum ist in NixieClockUltra.ino definiert
NightState nightState = NIGHT_NORMAL;

// Konfiguration (aus NVS geladen)
bool     nightTimeEnabled = false;
uint8_t  nightStart       = 23;
uint8_t  nightEnd         = 7;
uint8_t  nightTimeMode    = 0;    // 0 = DIM, 1 = DARK
bool     ldrEnabled       = false;
uint16_t ldrThreshold     = 512;

// Interner Zustand
uint16_t ldrReading    = 4095;   // default = hell → kein Trigger
uint32_t lastLdrRead   = 0;

static bool timeInNightRange(uint8_t h) {
  if (nightStart <= nightEnd)
    return (h >= nightStart && h < nightEnd);
  else
    return (h >= nightStart || h < nightEnd);
}

void updateNightMode() {
  if (ldrEnabled && millis() - lastLdrRead >= LDR_SAMPLE_MS) {
    lastLdrRead = millis();
    uint32_t sum = 0;
    for (int i = 0; i < 4; i++) sum += analogRead(LDR_PIN);
    ldrReading = (uint16_t)(sum / 4);
  }

  if (nightTimeEnabled && timeInNightRange(curHour)) {
    nightState = (nightTimeMode == 1) ? NIGHT_DARK : NIGHT_DIM;
  } else if (ldrEnabled && ldrReading <= ldrThreshold) {
    nightState = NIGHT_DIM;
  } else {
    nightState = NIGHT_NORMAL;
  }
}
```

- [ ] **Schritt 2: Forward-Deklaration in `NixieClockUltra.ino` ergänzen**

Im vorhandenen Block der Forward-Deklarationen (Zeile ~227):
```cpp
void updateNightMode();
```

- [ ] **Schritt 3: NVS-Ladeblock in `setup()` erweitern**

Nach dem bestehenden `prefs.getBool("colonStatic", false);`-Block:
```cpp
nightTimeEnabled = prefs.getBool("ntEn",     false);
nightStart       = prefs.getUChar("ntFrom",  23);
nightEnd         = prefs.getUChar("ntTo",    7);
nightTimeMode    = prefs.getUChar("ntMode",  0);
ldrEnabled       = prefs.getBool("ldrEn",    false);
ldrThreshold     = prefs.getUShort("ldrThr", 512);
```

- [ ] **Schritt 4: `updateNightMode()` in `loop()` einbinden**

Am Anfang von `loop()`, direkt nach dem Start-Fade-In-Block und vor den Taster-Reads – aber **nach** `handleBrightness()` und `handleEditMode()`:
```cpp
// --- Nacht-Modus ---
updateNightMode();
```

- [ ] **Schritt 5: Kompilieren, flashen, Serial Monitor prüfen**

Test: `nightTimeEnabled = true` manuell im Code setzen (temporär), `nightStart = curHour`, `nightEnd = (curHour+1)%24` → Serial zeigt keine Fehler. Dann zurücksetzen und erneut flashen. LDR-Werte via Serial prüfen: `Serial.printf("[Night] ldrReading=%d nightState=%d\n", ldrReading, nightState);` temporär in `updateNightMode()` hinzufügen.

- [ ] **Schritt 6: Debug-Print entfernen, Commit**

```bash
git add night_mode.ino NixieClockUltra.ino
git commit -m "feat: night_mode.ino – NightState, LDR-Abtastung, updateNightMode()"
```

---

## Task 6: Nixie Software-PWM + DARK-Zustand in `loop()`

**Files:**
- Modify: `NixieClockUltra.ino`

**Interfaces:**
- Consumes: `nightState` (Task 5); `displayDigits[]`, `nixieWrite()`
- Produces: Nixie-Röhren gedimmt (25 % Duty) bei NIGHT_DIM; aus bei NIGHT_DARK

- [ ] **Schritt 1: #defines und PWM-Zustandsvariablen in `NixieClockUltra.ino` hinzufügen**

Im Block „KONSTANTEN & KONFIGURATION":
```cpp
#define NIGHT_DIM_DUTY_PCT    25   // Nixie Ein-Anteil in % (Software-PWM)
#define NIGHT_DIM_PWM_PERIOD  20   // PWM-Periode in ms (~50 Hz)
#define NIGHT_DIM_NEO_PCT     15   // NeoPixel-Helligkeit im Dimm-Modus in % der Normalhelligkeit
```

Im Block „GLOBALE VARIABLEN":
```cpp
static bool        nixiePwmOn      = true;
static uint32_t    lastPwmToggle   = 0;
static NightState  prevNightState  = NIGHT_NORMAL;
```

- [ ] **Schritt 2: PWM-Gate in `loop()` einbauen**

Nach `updateNightMode();`, vor dem Taster-Block:

```cpp
// --- Nacht-Modus: Nixie-Ausgabe steuern ---
if (nightState != prevNightState) {
  prevNightState = nightState;
  if (nightState == NIGHT_DARK) {
    uint8_t blank[6] = {10, 10, 10, 10, 10, 10};
    nixieWrite(blank);
    nixiePwmOn = false;
  } else if (nightState == NIGHT_DIM) {
    nixiePwmOn    = true;
    lastPwmToggle = millis();
    nixieWrite(displayDigits);
  } else {  // NIGHT_NORMAL
    nixiePwmOn = true;
    nixieWrite(displayDigits);
  }
}

if (nightState == NIGHT_DIM) {
  const uint32_t onTime  = NIGHT_DIM_PWM_PERIOD * NIGHT_DIM_DUTY_PCT / 100;
  const uint32_t offTime = NIGHT_DIM_PWM_PERIOD - onTime;
  uint32_t elapsed = millis() - lastPwmToggle;
  if (nixiePwmOn && elapsed >= onTime) {
    nixiePwmOn    = false;
    lastPwmToggle = millis();
    uint8_t blank[6] = {10, 10, 10, 10, 10, 10};
    nixieWrite(blank);
  } else if (!nixiePwmOn && elapsed >= offTime) {
    nixiePwmOn    = true;
    lastPwmToggle = millis();
    nixieWrite(displayDigits);
  }
}
```

- [ ] **Schritt 3: Kompilieren, flashen, testen**

Test A (DIM): Temporär `nightState = NIGHT_DIM;` direkt nach `updateNightMode();` setzen → Röhren sollten deutlich dunkler leuchten (25 % Helligkeit). Kein sichtbares Flimmern bei 50 Hz.

Test B (DARK): `nightState = NIGHT_DARK;` → alle Röhren aus. Nach Rücksetzen auf NORMAL → Röhren zeigen wieder die Zeit.

Temporären Code wieder entfernen.

- [ ] **Schritt 4: Commit**

```bash
git add NixieClockUltra.ino
git commit -m "feat: Nixie Software-PWM (25% Duty/50Hz) und DARK-Modus in loop()"
```

---

## Task 7: NeoPixel Nacht-Dimming in `updateNeoPixel()`

**Files:**
- Modify: `neo_animation.ino`

**Interfaces:**
- Consumes: `nightState` (Task 5); `NIGHT_DIM_NEO_PCT` (Task 6); `neoBright`, `colonBright`
- Produces: NeoPixel mit temporärer Helligkeitsskalierung je NightState

- [ ] **Schritt 1: Effektive Helligkeit am Anfang von `updateNeoPixel()` berechnen**

Direkt nach `if (now - lastNeoUpdate < 20) return;` und `lastNeoUpdate = now;`:

```cpp
// Temporäre Skalierung für Nacht-Modus (kein NVS-Schreiben)
uint8_t effNeoBright   = neoBright;
uint8_t effColonBright = colonBright;
if (nightState == NIGHT_DARK) {
  effNeoBright   = 0;
  effColonBright = 0;
} else if (nightState == NIGHT_DIM) {
  effNeoBright   = neoBright   * NIGHT_DIM_NEO_PCT / 100;
  effColonBright = colonBright * NIGHT_DIM_NEO_PCT / 100;
}
```

- [ ] **Schritt 2: Alle `neoBright`/`colonBright`-Vorkommen in `updateNeoPixel()` durch `effNeoBright`/`effColonBright` ersetzen**

In `ANIM_RAINBOW`:
- `scaleColor(..., neoBright)` → `scaleColor(..., effNeoBright)`
- `scaleColor(colonColor, colonBright)` → `scaleColor(colonColor, effColonBright)`

In `ANIM_STATIC`:
- `scaleColor(warm, neoBright)` → `scaleColor(warm, effNeoBright)`
- `scaleColor(colonColor, colonBright)` → `scaleColor(colonColor, effColonBright)`

In `ANIM_PULSE`:
- `scaleColor(col, neoBright)` → `scaleColor(col, effNeoBright)`
- `scaleColor(colonColor, colonBright)` → `scaleColor(colonColor, effColonBright)`

Im Slot-Effekt-Block (nach `if (slotActive)`):
- `scaleColor(col, neoBright)` → `scaleColor(col, effNeoBright)`
- `scaleColor(col, colonBright)` → `scaleColor(col, effColonBright)`

Im `colonStatic`-Block:
- `scaleColor(warmwhite, colonBright)` → `scaleColor(warmwhite, effColonBright)`

- [ ] **Schritt 3: Kompilieren, flashen, testen**

Test: Temporär `nightState = NIGHT_DIM;` → NeoPixel leuchtet auf ~15 % der normalen Helligkeit. `nightState = NIGHT_DARK;` → NeoPixel komplett aus. Zurücksetzen → normale Helligkeit.

- [ ] **Schritt 4: Commit**

```bash
git add neo_animation.ino
git commit -m "feat: NeoPixel Helligkeitsskalierung per NightState (DIM 15%, DARK 0%)"
```

---

## Task 8: Nacht-Modus Web-UI + API-Endpunkte

**Files:**
- Modify: `web_server.ino`

**Interfaces:**
- Consumes: `nightState`, `nightTimeEnabled`, `nightStart`, `nightEnd`, `nightTimeMode`, `ldrEnabled`, `ldrThreshold`, `ldrReading` (alle aus Task 5)
- Produces: `GET/POST /api/nightmode`; Nacht-Modus-Karte im Web-UI; nightState-Badge

- [ ] **Schritt 1: `GET /api/nightmode`-Endpunkt in `setupWebServer()` hinzufügen**

Vor `server.begin();`:
```cpp
server.on("/api/nightmode", HTTP_GET, [](AsyncWebServerRequest *req) {
  StaticJsonDocument<256> doc;
  doc["ntEn"]   = nightTimeEnabled;
  doc["ntFrom"] = nightStart;
  doc["ntTo"]   = nightEnd;
  doc["ntMode"] = nightTimeMode;
  doc["ldrEn"]  = ldrEnabled;
  doc["ldrThr"] = ldrThreshold;
  doc["ldrVal"] = ldrReading;
  doc["state"]  = (int)nightState;
  String out; serializeJson(doc, out);
  req->send(200, "application/json", out);
});
```

- [ ] **Schritt 2: `POST /api/nightmode`-Endpunkt hinzufügen**

Direkt nach dem GET-Handler:
```cpp
server.on("/api/nightmode", HTTP_POST, [](AsyncWebServerRequest *req){},
  nullptr,
  [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
    StaticJsonDocument<256> doc;
    if (!deserializeJson(doc, data, len)) {
      nightTimeEnabled = doc["ntEn"].as<bool>();
      nightStart       = constrain((int)doc["ntFrom"], 0, 23);
      nightEnd         = constrain((int)doc["ntTo"],   0, 23);
      nightTimeMode    = constrain((int)doc["ntMode"], 0, 1);
      ldrEnabled       = doc["ldrEn"].as<bool>();
      ldrThreshold     = (uint16_t)constrain((int)doc["ldrThr"], 0, 4095);
      prefs.putBool("ntEn",     nightTimeEnabled);
      prefs.putUChar("ntFrom",  nightStart);
      prefs.putUChar("ntTo",    nightEnd);
      prefs.putUChar("ntMode",  nightTimeMode);
      prefs.putBool("ldrEn",    ldrEnabled);
      prefs.putUShort("ldrThr", ldrThreshold);
      req->send(200, "application/json", "{\"ok\":true}");
    } else {
      req->send(400, "application/json", "{\"ok\":false}");
    }
  }
);
```

- [ ] **Schritt 3: HTML-Karte „Nacht-Modus" in PROGMEM-String einbauen**

Nach der Slot-Karte (`</div>` nach dem Slot-Button-Block), vor der IR-Karte:

```html
<div class="card">
  <h2>🌙 Nacht-Modus</h2>
  <div class="row"><label>Aktuell</label><span class="badge" id="nightStateBadge">Normal</span></div>
  <div class="row"><label>Zeitbereich aktiv</label>
    <label class="toggle"><input type="checkbox" id="ntEn"><span class="slider"></span></label></div>
  <div class="row"><label>Von (Stunde 0–23)</label>
    <input type="number" id="ntFrom" min="0" max="23" value="23"></div>
  <div class="row"><label>Bis (Stunde 0–23)</label>
    <input type="number" id="ntTo" min="0" max="23" value="7"></div>
  <div class="row"><label>Modus</label>
    <select id="ntMode">
      <option value="0">Gedimmt</option>
      <option value="1">Dunkel</option>
    </select></div>
  <div class="row"><label>LDR-Sensor aktiv</label>
    <label class="toggle"><input type="checkbox" id="ldrEn"><span class="slider"></span></label></div>
  <div class="row"><label>Schwellwert (0–4095)</label>
    <input type="range" id="ldrThr" min="0" max="4095" value="512"
      oninput="document.getElementById('ldrThrVal').textContent=this.value">
    <span id="ldrThrVal">512</span></div>
  <div class="row"><label>LDR-Rohwert</label><span class="badge" id="ldrVal">--</span></div>
  <div class="row"><button onclick="saveNightMode()">Übernehmen</button></div>
</div>
```

- [ ] **Schritt 4: JavaScript-Funktionen hinzufügen**

Im `<script>`-Block, vor dem abschließenden `</script>`:

```javascript
const NS_LABELS=['Normal','Gedimmt','Dunkel'];
async function refreshNightMode(){
  let d=await get('/api/nightmode');
  if(d.ntEn===undefined)return;
  document.getElementById('ntEn').checked  = d.ntEn;
  document.getElementById('ntFrom').value  = d.ntFrom;
  document.getElementById('ntTo').value    = d.ntTo;
  document.getElementById('ntMode').value  = d.ntMode;
  document.getElementById('ldrEn').checked = d.ldrEn;
  document.getElementById('ldrThr').value  = d.ldrThr;
  document.getElementById('ldrThrVal').textContent = d.ldrThr;
  document.getElementById('ldrVal').textContent    = d.ldrVal!==undefined?d.ldrVal:'--';
}
async function saveNightMode(){
  let r=await api('/api/nightmode',{
    ntEn:  document.getElementById('ntEn').checked,
    ntFrom:parseInt(document.getElementById('ntFrom').value),
    ntTo:  parseInt(document.getElementById('ntTo').value),
    ntMode:parseInt(document.getElementById('ntMode').value),
    ldrEn: document.getElementById('ldrEn').checked,
    ldrThr:parseInt(document.getElementById('ldrThr').value)
  });
  document.getElementById('statusMsg').textContent=r.ok?'Nacht-Modus gespeichert ✓':'Fehler ✗';
}
```

In `refreshClock()` folgende Zeile ergänzen (nach dem nightState-Block aus Task 4, Schritt 1):
```javascript
if(d.nightState!==undefined) document.getElementById('nightStateBadge').textContent=NS_LABELS[d.nightState]||'Normal';
```

Am Ende des Skripts, nach `refreshWifi();`:
```javascript
refreshNightMode();
```

- [ ] **Schritt 5: Kompilieren, flashen, testen**

Test A: Web-UI öffnen → Nacht-Modus-Karte erscheint, Badge zeigt „Normal".  
Test B: Zeitbereich auf aktuelle Stunde ±1 setzen, Modus „Gedimmt", Übernehmen → Badge wechselt zu „Gedimmt", Röhren dimmen, NeoPixel dunkler.  
Test C: Modus auf „Dunkel" → Röhren und NeoPixel aus.  
Test D: LDR aktivieren, Schwellwert auf 4095 → LDR-Dimmen aktiv (sofern kein Zeitbereich greift).  
Test E: LDR-Rohwert-Badge zeigt ADC-Wert (~0–4095).

- [ ] **Schritt 6: Commit**

```bash
git add web_server.ino
git commit -m "feat: Nacht-Modus Web-UI; /api/nightmode GET+POST"
```

---

## Abschluss-Checkliste

- [ ] Alle 8 Tasks committed
- [ ] Slot-Animation → 5 s Datum → Uhrzeit: getestet
- [ ] Taster-Editiersequenz Std/Min/Sek/Tag/Monat/Jahr: getestet
- [ ] Web-UI Datum setzen und per NTP sync: getestet
- [ ] Nacht-Modus Zeitbereich (DIM + DARK): getestet
- [ ] Nacht-Modus LDR (nur DIM): getestet
- [ ] NVS-Werte bleiben nach Neustart erhalten: getestet
