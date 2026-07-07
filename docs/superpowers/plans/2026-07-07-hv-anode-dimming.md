# Hardware-Dimmung der Anodenspannung (TLP627) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the software-PWM (Kathoden-Blanking) dimming of the Nixie tubes in Night-Mode with real hardware PWM on the anode supply, driven by a new ESP32-S3 GPIO through a TLP627 optocoupler.

**Architecture:** A new self-contained module `hv_dimmer.ino` wraps the ESP32-S3 LEDC hardware-PWM peripheral behind two functions (`hvDimmerInit()`, `hvDimmerSetDuty()`). `NixieClockUltra.ino`'s night-mode transition code in `loop()` calls `hvDimmerSetDuty()` instead of toggling cathode data via `nixieWrite()`. A new `hvDimPct` setting (NVS-backed, 5–95%) replaces the fixed `NIGHT_DIM_DUTY_PCT` constant and is exposed as a Web-UI slider.

**Tech Stack:** Arduino Core for ESP32 (3.x, LEDC API: `ledcAttach()`/`ledcWrite()`), ESPAsyncWebServer, ArduinoJson 6.x, Preferences (NVS).

## Global Constraints

- GPIO: `HV_SWITCH_PIN = 7` (unbenutzt, kein Strapping-Pin).
- PWM: LEDC, 8-Bit-Auflösung (Duty 0–255), Frequenz `HV_PWM_FREQ_HZ = 200`.
- Signal-Vertrag: HIGH-Duty = Anode an (nicht invertiert).
- `hvDimPct`: uint8, Range 5–95, Default 25, NVS-Key `"hvDimPct"` im Namespace `"nixie"`.
- Scope: nur Ersatz von `NIGHT_DIM`/`NIGHT_DARK` für die Röhren. `NIGHT_NORMAL` bleibt Duty 255 (voll an). NeoPixel-Dimmung (`NIGHT_DIM_NEO_PCT`) bleibt unverändert.
- Kein `arduino-cli` in dieser Umgebung verfügbar — Kompilierung muss vom User via Arduino IDE oder lokalem `arduino-cli` geprüft werden. Jeder Task enthält dafür einen expliziten manuellen Schritt.
- Spec: `docs/superpowers/specs/2026-07-07-hv-anode-dimming-design.md`
- Branch: `feature/hv-anode-dimming` (bereits angelegt, HEAD bei Commit `0ff092c`)

---

### Task 1: `hv_dimmer.ino`-Modul + Init-Aufruf

**Files:**
- Create: `hv_dimmer.ino`
- Modify: `NixieClockUltra.ino:64-68` (Pin-Definitionen), `NixieClockUltra.ino:103-106` (Nacht-Modus-Defines), `NixieClockUltra.ino:283-287` (setup(), NeoPixel-Init-Block)

**Interfaces:**
- Produces: `void hvDimmerInit()`, `void hvDimmerSetDuty(uint8_t duty0to255)` — genutzt von Task 2.

- [ ] **Step 1: Pin-Define ergänzen**

In `NixieClockUltra.ino`, nach Zeile 68 (`#define I2C_SCL 9`), im Block „PIN-DEFINITIONEN“:

```cpp
// HV-Dimmer (TLP627 → Anodenspannung)
#define HV_SWITCH_PIN  7
```

- [ ] **Step 2: Nacht-Modus-Defines ersetzen**

In `NixieClockUltra.ino`, nach Zeile 106 (`#define NIGHT_DIM_NEO_PCT ...`) ergänzen — `NIGHT_DIM_DUTY_PCT`/`NIGHT_DIM_PWM_PERIOD` bleiben vorerst unverändert bestehen, da `loop()` sie noch verwendet und erst Task 2 diese Verwendung entfernt (self-contained/kompilierbarer Zwischenstand):

```cpp
// HV-Dimmer (TLP627, LEDC-Hardware-PWM auf HV_SWITCH_PIN)
#define HV_PWM_FREQ_HZ        200  // Hz — nach Hardwareaufbau per Oszilloskop verifizieren
```

- [ ] **Step 3: Neue Datei `hv_dimmer.ino` anlegen**

```cpp
// ═══════════════════════════════════════════════════════════
//  HARDWARE-DIMMUNG DER ANODENSPANNUNG (TLP627, LEDC-PWM)
// ═══════════════════════════════════════════════════════════

void hvDimmerInit() {
  ledcAttach(HV_SWITCH_PIN, HV_PWM_FREQ_HZ, 8);
  ledcWrite(HV_SWITCH_PIN, 255);   // volle Helligkeit (Anode dauerhaft an)
}

void hvDimmerSetDuty(uint8_t duty0to255) {
  ledcWrite(HV_SWITCH_PIN, duty0to255);
}
```

- [ ] **Step 4: `hvDimmerInit()` in `setup()` aufrufen**

In `NixieClockUltra.ino`, im Block:

```cpp
  // --- NeoPixel ---
  strip.begin();
  strip.setBrightness(255);  // Skalierung erfolgt per Pixel in neo_animation
  strip.clear();
  strip.show();
```

direkt danach ergänzen:

```cpp

  // --- HV-Dimmer (TLP627) ---
  hvDimmerInit();
```

- [ ] **Step 5: Strukturprüfung (grep)**

Kein Compiler in dieser Umgebung verfügbar — stattdessen strukturelle Prüfung:

Run: `grep -n "HV_SWITCH_PIN\|HV_PWM_FREQ_HZ\|hvDimmerInit\|NIGHT_DIM_DUTY_PCT\|NIGHT_DIM_PWM_PERIOD" NixieClockUltra.ino hv_dimmer.ino`

Expected:
- `HV_SWITCH_PIN` und `HV_PWM_FREQ_HZ` je genau einmal als `#define` in `NixieClockUltra.ino`, je einmal referenziert in `hv_dimmer.ino`.
- `hvDimmerInit` erscheint in `hv_dimmer.ino` (Definition) und in `NixieClockUltra.ino` (Aufruf in `setup()`).
- `NIGHT_DIM_DUTY_PCT` und `NIGHT_DIM_PWM_PERIOD` bleiben unverändert vorhanden (je ein `#define` + Verwendung in `loop()`) — ihre Entfernung ist Teil von Task 2, zusammen mit der Verwendung.

- [ ] **Step 6: Manueller Kompilier-Check (durch User)**

In der Arduino IDE (oder `arduino-cli compile --fqbn esp32:esp32:esp32s3 .`) verifizieren, dass das Sketch fehlerfrei kompiliert — insbesondere dass `ledcAttach()`/`ledcWrite()` mit der installierten ESP32-Core-3.x-Version übereinstimmen (bekannter Breaking Change gegenüber Core 2.x, siehe Projekt-Notizen zu Timer-API).

- [ ] **Step 7: Commit**

```bash
git add NixieClockUltra.ino hv_dimmer.ino
git commit -m "feat: hv_dimmer.ino – LEDC-Hardware-PWM für TLP627 auf HV_SWITCH_PIN"
```

---

### Task 2: Nacht-Modus nutzt `hvDimmerSetDuty()` statt Software-PWM

**Files:**
- Modify: `NixieClockUltra.ino:150-158` (Nacht-Modus-Globals), `NixieClockUltra.ino:197-200` (PWM-Zustand-Globals), `NixieClockUltra.ino:283-313` (setup(), NVS-Laden), `NixieClockUltra.ino:384-415` (loop(), Nacht-Modus-Block), `display.ino:5-11` (`nixieWriteSafe()` — referenziert `nixiePwmOn`, das mit diesem Task entfällt)

**Interfaces:**
- Consumes: `void hvDimmerSetDuty(uint8_t duty0to255)` aus Task 1.
- Produces: globale Variable `uint8_t hvDimPct` (5–95, Default 25) — genutzt von Task 4 (Web-API).

- [ ] **Step 1: `hvDimPct`-Global ergänzen**

In `NixieClockUltra.ino`, nach Zeile 157 (`uint16_t ldrReading = 4095;`):

```cpp
uint8_t  hvDimPct           = 25;   // Röhren-Dimm-Helligkeit in % (5–95), NVS-Key "hvDimPct"
```

- [ ] **Step 2: Software-PWM-Zustand entfernen**

In `NixieClockUltra.ino`, Zeilen 197–200 ersetzen:

```cpp
// Nacht-Modus: Nixie Software-PWM Zustand
static bool        nixiePwmOn      = true;
static uint32_t    lastPwmToggle   = 0;
static NightState  prevNightState  = NIGHT_NORMAL;
```

durch:

```cpp
// Nacht-Modus: letzter angewendeter Zustand (für Übergangserkennung)
static NightState  prevNightState  = NIGHT_NORMAL;
```

- [ ] **Step 3: `hvDimPct` aus NVS laden**

In `NixieClockUltra.ino`, `setup()`, nach der Zeile `ldrThreshold = prefs.getUShort("ldrThr", 512);` ergänzen:

```cpp
  hvDimPct         = (uint8_t)constrain((int)prefs.getUChar("hvDimPct", 25), 5, 95);
```

- [ ] **Step 4: `loop()`-Block ersetzen**

In `NixieClockUltra.ino`, den kompletten Block ersetzen:

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

durch:

```cpp
  // --- Nacht-Modus: Röhren-Helligkeit per HV-Dimmer steuern ---
  if (nightState != prevNightState) {
    prevNightState = nightState;
    switch (nightState) {
      case NIGHT_DARK:   hvDimmerSetDuty(0);                        break;
      case NIGHT_DIM:    hvDimmerSetDuty(hvDimPct * 255 / 100);     break;
      case NIGHT_NORMAL: hvDimmerSetDuty(255);                      break;
    }
  }
```

- [ ] **Step 5: Jetzt verwaiste Defines entfernen**

Mit Step 4 verschwindet die letzte Verwendung von `NIGHT_DIM_DUTY_PCT`/`NIGHT_DIM_PWM_PERIOD` (Task 1 hatte sie bewusst noch stehen lassen, um kompilierbar zu bleiben). In `NixieClockUltra.ino` die Zeile

```cpp
// Nacht-Modus: Nixie Software-PWM
#define NIGHT_DIM_DUTY_PCT    25   // Nixie Ein-Anteil in % (Software-PWM)
#define NIGHT_DIM_PWM_PERIOD  20   // PWM-Periode in ms (~50 Hz)
```

vollständig entfernen (der Kommentar „Nacht-Modus: Nixie Software-PWM“ gehört mit dazu, `NIGHT_DIM_NEO_PCT` bleibt unverändert stehen).

- [ ] **Step 6: `nixieWriteSafe()` in `display.ino` anpassen**

`display.ino:5-11` referenziert das mit Step 2 entfernte `nixiePwmOn` — ohne Anpassung kompiliert das Sketch nicht (Arduino verkettet alle `.ino`-Dateien zu einer Übersetzungseinheit). Die Funktion diente dazu, kurze Aufblitzer während der PWM-Off-Phase im (jetzt entfallenen) Kathoden-Blanking zu verhindern; das ist überflüssig, da jetzt die Anodenspannung selbst die Helligkeit/Dunkelheit bestimmt — ein Schreiben der echten Ziffern hat keine sichtbare Wirkung, solange die Anode per `hvDimmerSetDuty()` abgeschaltet ist. Den Block

```cpp
static void nixieWriteSafe() {
  if (nightState == NIGHT_DARK) return;
  if (nightState == NIGHT_DIM && !nixiePwmOn) return;
  nixieWrite(displayDigits);
}
```

ersetzen durch:

```cpp
static void nixieWriteSafe() {
  nixieWrite(displayDigits);
}
```

- [ ] **Step 7: Strukturprüfung (grep)**

Run: `grep -rn "nixiePwmOn\|lastPwmToggle\|hvDimmerSetDuty\|hvDimPct\|NIGHT_DIM_DUTY_PCT\|NIGHT_DIM_PWM_PERIOD" --include=*.ino .`

Expected:
- `nixiePwmOn`, `lastPwmToggle`, `NIGHT_DIM_DUTY_PCT`, `NIGHT_DIM_PWM_PERIOD` liefern **keine** Treffer mehr in irgendeiner `.ino`-Datei (nicht nur `NixieClockUltra.ino`).
- `hvDimmerSetDuty` erscheint dreimal in `NixieClockUltra.ino` (DARK/DIM/NORMAL-Fälle).
- `hvDimPct` erscheint dreimal in `NixieClockUltra.ino` (Global-Deklaration, NVS-Laden, `NIGHT_DIM`-Case).

- [ ] **Step 8: Manueller Kompilier-Check (durch User)**

Wie Task 1, Step 6 — zusätzlich prüfen, dass `nixieWrite`/`displayDigits` weiterhin nur dort referenziert werden, wo sie schon vorher standen (kein verwaistes Vorkommen mehr im entfernten Block).

- [ ] **Step 9: Commit**

```bash
git add NixieClockUltra.ino display.ino
git commit -m "feat: Nacht-Modus-Dimmung über hv_dimmer.ino statt Kathoden-Blanking"
```

---

### Task 3: Web-UI-Slider „Dimm-Helligkeit“

**Files:**
- Modify: `web_server.ino:125-129` (HTML, Nacht-Modus-Karte), `web_server.ino:319-329` (JS `refreshNightMode()`), `web_server.ino:331-341` (JS `saveNightMode()`)

**Interfaces:**
- Consumes: nichts aus vorherigen Tasks (reines Frontend).
- Produces: HTML-Element `id="hvDimPct"` (Slider) + `id="hvDimPctVal"` (Anzeige), JS-Feld `hvDimPct` im `/api/nightmode`-Request-Body — konsumiert von Task 4.

- [ ] **Step 1: Slider-HTML einfügen**

In `web_server.ino`, nach Zeile 129 (schließendes `</select></div>` des `ntMode`-Dropdowns) und vor Zeile 130 (`Lichtsensor aktiv`-Zeile) einfügen:

```html
  <div class="row"><label>Dimm-Helligkeit</label>
    <input type="range" id="hvDimPct" min="5" max="95" value="25"
      oninput="document.getElementById('hvDimPctVal').textContent=this.value">
    <span id="hvDimPctVal">25</span>%</div>
```

- [ ] **Step 2: `refreshNightMode()` erweitern**

In `web_server.ino`, nach der Zeile `document.getElementById('ntMode').value  = d.ntMode;` ergänzen:

```js
  document.getElementById('hvDimPct').value = d.hvDimPct;
  document.getElementById('hvDimPctVal').textContent = d.hvDimPct;
```

- [ ] **Step 3: `saveNightMode()` erweitern**

In `web_server.ino`, im `api('/api/nightmode', {...})`-Objekt, nach der Zeile `ntMode:parseInt(document.getElementById('ntMode').value),` ergänzen:

```js
    hvDimPct:parseInt(document.getElementById('hvDimPct').value),
```

- [ ] **Step 4: Strukturprüfung (grep)**

Run: `grep -n "hvDimPct" web_server.ino`

Expected: mindestens 5 Treffer — HTML-Slider (`id="hvDimPct"` + `id="hvDimPctVal"`), `refreshNightMode()` (2 Zeilen), `saveNightMode()` (1 Zeile).

- [ ] **Step 5: Manueller Browser-Check (durch User)**

Nach Task 4 (API-Anbindung) im Web-UI: Seite laden, Nacht-Modus-Karte zeigt Slider zwischen „Modus“ und „Lichtsensor aktiv“ mit Wert aus `/api/nightmode`; Slider bewegen aktualisiert die %-Anzeige sofort.

- [ ] **Step 6: Commit**

```bash
git add web_server.ino
git commit -m "feat: Web-UI-Slider für Nacht-Modus-Dimm-Helligkeit (hvDimPct)"
```

---

### Task 4: `/api/nightmode` um `hvDimPct` erweitern

**Files:**
- Modify: `web_server.ino:645-657` (GET-Handler), `web_server.ino:660-682` (POST-Handler)

**Interfaces:**
- Consumes: `uint8_t hvDimPct` (Global, aus Task 2), `void hvDimmerSetDuty(uint8_t)` (aus Task 1), JS-Feld `hvDimPct` aus dem Request-Body (aus Task 3).
- Produces: JSON-Feld `"hvDimPct"` in GET/POST-Response — konsumiert vom Browser (Task 3, bereits verdrahtet).

- [ ] **Step 1: GET-Handler erweitern**

In `web_server.ino`, nach der Zeile `doc["ntMode"] = nightTimeMode;` ergänzen:

```cpp
    doc["hvDimPct"] = hvDimPct;
```

- [ ] **Step 2: POST-Handler erweitern**

In `web_server.ino`, nach der Zeile `nightTimeMode    = constrain((int)doc["ntMode"], 0, 1);` ergänzen:

```cpp
        hvDimPct         = (uint8_t)constrain((int)doc["hvDimPct"], 5, 95);
```

Nach der Zeile `prefs.putUChar("ntMode",  nightTimeMode);` ergänzen:

```cpp
        prefs.putUChar("hvDimPct", hvDimPct);
```

Nach der Zeile `prefs.putUShort("ldrThr", ldrThreshold);` (letzte NVS-Schreibzeile vor `req->send(...)`) ergänzen, damit ein während `NIGHT_DIM` geänderter Slider sofort wirkt:

```cpp
        if (nightState == NIGHT_DIM) hvDimmerSetDuty(hvDimPct * 255 / 100);
```

- [ ] **Step 3: Strukturprüfung (grep)**

Run: `grep -n "hvDimPct" web_server.ino`

Expected: zusätzlich zu den Treffern aus Task 3 nun auch die GET-Zeile (`doc["hvDimPct"]`), die POST-Zuweisung, `prefs.putUChar("hvDimPct"...)` und der `hvDimmerSetDuty(...)`-Aufruf — insgesamt 9 Treffer.

- [ ] **Step 4: Manueller Kompilier-Check (durch User)**

Wie Task 1, Step 6.

- [ ] **Step 5: Commit**

```bash
git add web_server.ino
git commit -m "feat: /api/nightmode GET+POST um hvDimPct erweitert, sofortige Anwendung im Dimm-Modus"
```

---

### Task 5: Hardware-Verifikation (manuell, durch User)

**Files:** keine Code-Änderungen — Abnahme-Checkliste am realen Gerät.

**Interfaces:**
- Consumes: fertig geflashte Firmware aus Task 1–4, aufgebaute TLP627-Schaltung an GPIO7.

- [ ] **Step 1: Isolierte Signalprüfung (ohne HV)**

GPIO7 mit Oszilloskop/Logic-Analyzer prüfen: 200 Hz, korrekter Duty bei jedem `nightState`-Übergang (0 / `hvDimPct` / 255) und bei Slider-Änderung im Web-UI.

- [ ] **Step 2: TLP627-Stufe separat prüfen**

LED-Vorwärtsstrom und sauberes Schalten der Anodenspannung am TLP627-Ausgang messen, bevor die Röhren dranhängen.

- [ ] **Step 3: Funktionstest am fertigen Gerät**

Nacht-Modus-Zeitfenster testweise aktivieren, `hvDimPct`-Slider 5–95% durchfahren → sichtbares, flackerfreies Dimmen. Dunkel-Modus prüfen → Röhren komplett aus, kein Nachglühen.

- [ ] **Step 4: Regressionscheck**

`NIGHT_NORMAL` weiterhin volle, stabile Helligkeit; Taster-/IR-Bedienung unverändert; NeoPixel-Nachtdimmung weiterhin korrekt (unverändert im Code, aber am Gerät gegenprüfen).

- [ ] **Step 5: Ergebnis dokumentieren**

Kurze Rückmeldung, ob alle vier Punkte bestanden wurden, damit der Branch ggf. nach `master` gemerged werden kann.

---

## Nicht im Scope (aus Spec übernommen)

- Mehrstufige/stufenlose Röhren-Helligkeit außerhalb des Nacht-Modus.
- Änderungen an der NeoPixel-Dimmung.
- Dimensionierung des LED-Vorwiderstands am GPIO7/TLP627 sowie Prüfung des Anodenstroms gegen die 120mA-Collector-Current-Grenze.
- Nicht-blockierendes WiFi-Connect.
