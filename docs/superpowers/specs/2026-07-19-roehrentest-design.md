# NixieClockUltra – Röhrentest

**Datum:** 2026-07-19
**Status:** Genehmigt
**Branch:** neu anzulegen von `master`

---

## Überblick

Diagnose-Feature für die Nixie-Röhren: Ein Web-UI-Button zeigt auf
allen 6 Röhren gleichzeitig nacheinander jede Ziffer 0–9 für je 2
Sekunden an (20s Gesamtlaufzeit). Zweck: optisch prüfen, ob jede
Ziffer in jeder Röhre sauber leuchtet (defekte Kathode, kalte
Lötstelle an Transistor/MCP23017) – die bestehende Slot-Machine-
Animation reicht dafür nicht, da sie pro Röhre nur kurz durchrollt
und nie alle Röhren synchron auf derselben Ziffer stehen bleiben.

Kein neues physisches Bedienelement, keine IR-Anbindung – ausschließlich
über die Web-UI, ergänzt in der bestehenden Karte "Slot-Animation".

---

## 1. State-Machine (neues Modul `tube_test.ino`)

Analog zu `updateSlotAnimation()` / `updateDigitFade()`: non-blocking,
angetrieben von `updateTubeTest()` in `loop()`.

```cpp
bool     tubeTestActive    = false;
uint8_t  tubeTestDigit     = 0;      // 0–9, aktuell angezeigte Ziffer
uint32_t tubeTestStepStart = 0;
#define  TUBE_TEST_STEP_MS 2000

void startTubeTest() {
  cancelDigitFade();
  slotActive     = false;   // laufende Slot-Animation abbrechen
  dateShowActive = false;   // laufende Datumsanzeige abbrechen
  editState      = EDIT_NONE;   // Einstellmodus verlassen

  tubeTestActive    = true;
  tubeTestDigit     = 0;
  tubeTestStepStart = millis();

  hvDimmerSetDuty(255);   // volle Helligkeit erzwingen, unabhängig von Nacht-Modus

  uint8_t d[6] = {0,0,0,0,0,0};
  memcpy(displayDigits, d, 6);
  nixieWrite(displayDigits);   // sofort hart, kein Fade
}

void updateTubeTest() {
  if (!tubeTestActive) return;
  if (millis() - tubeTestStepStart < TUBE_TEST_STEP_MS) return;

  tubeTestDigit++;
  if (tubeTestDigit > 9) {
    stopTubeTest();
    return;
  }
  tubeTestStepStart = millis();
  uint8_t d[6] = {tubeTestDigit, tubeTestDigit, tubeTestDigit,
                  tubeTestDigit, tubeTestDigit, tubeTestDigit};
  memcpy(displayDigits, d, 6);
  nixieWrite(displayDigits);
}

void stopTubeTest() {
  if (!tubeTestActive) return;
  tubeTestActive = false;

  // HV-Duty passend zum aktuellen Nacht-Modus-Zustand wiederherstellen
  switch (nightState) {
    case NIGHT_DARK:   hvDimmerSetDuty(0);                    break;
    case NIGHT_DIM:    hvDimmerSetDuty(hvDimPct * 255 / 100); break;
    case NIGHT_NORMAL: hvDimmerSetDuty(255);                  break;
  }
  prevNightState = nightState;   // verhindert doppelte Duty-Anwendung im nächsten loop()

  setDisplayTime(curHour, curMin, curSec);   // Uhrzeit sofort wieder anzeigen
}
```

**Re-Start-Verhalten:** Aufruf von `startTubeTest()` während der Test
bereits läuft ist erlaubt und setzt ihn einfach auf Ziffer 0 zurück
(kein Fehlerfall). `stopTubeTest()` ohne aktiven Test ist ein No-op.

**Doppelte Nacht-Modus-Duty-Anwendung:** `NixieClockUltra.ino::loop()`
erkennt Nacht-Modus-Wechsel über `nightState != prevNightState` und
setzt dann die Duty. `stopTubeTest()` setzt `prevNightState` explizit
mit, damit dieser Vergleich direkt danach nicht erneut anschlägt und
die gerade gesetzte Duty nicht überschreibt.

---

## 2. Integration `NixieClockUltra.ino::loop()`

Bestehende Blöcke werden um `!tubeTestActive`-Guards ergänzt, damit
die normale Uhr während des Tests nicht dazwischenschreibt:

```cpp
// RTC-Lese-Block: Sekundentakt-Update / Slot-Trigger nur außerhalb Test
if (editState == EDIT_NONE && millis() - lastRtcRead >= 500) {
  lastRtcRead = millis();
  readRTC();
  if (curSec != lastSec) {
    lastSec = curSec;
    if (!tubeTestActive) {
      // ... bestehende Slot-Trigger-/setDisplayTimeSoft-Logik unverändert
    }
  }
}

// Slot-Animation-Update: unverändert, da slotActive bereits durch
// startTubeTest() auf false gesetzt wird

// Einstellmodus: während des Tests pausiert, damit ein SET-Tastendruck
// nicht parallel den Editiermodus startet
if (!tubeTestActive) handleEditMode();

updateTubeTest();   // neu, an der Stelle von updateDigitFade()
```

Der bestehende `nightState != prevNightState`-Block (HV-Duty bei
Nacht-Modus-Wechsel) bleibt unverändert – er greift während des Tests
nicht, weil `stopTubeTest()` `prevNightState` synchron mitführt (siehe
oben); ein Nacht-Modus-Wechsel *während* eines laufenden Tests würde
aber weiterhin die Duty umschalten (Testabbruch durch Nacht-Modus ist
nicht vorgesehen – vernachlässigbarer Edge-Case, Test läuft nur 20s
und wird typischerweise manuell direkt am Gerät ausgelöst).

---

## 3. Web-UI

### 3.1 Karte (bestehende "🎰 Slot-Animation", umbenannt)

```html
<div class="card">
  <h2>🎰 Slot-Animation &amp; Röhrentest</h2>
  <!-- ... bestehende Slot-Intervall/Geschwindigkeit/Trigger-Reihen unverändert ... -->

  <div class="row">
    <button id="tubeTestBtn" onclick="toggleTubeTest()">Röhrentest starten</button>
    <span class="badge" id="tubeTestStatus">bereit</span>
  </div>
</div>
```

### 3.2 JavaScript

```js
let tubeTestRunning = false;

async function toggleTubeTest(){
  if (tubeTestRunning) await api('/api/tubetest/stop', {});
  else                 await api('/api/tubetest/start', {});
}
```

`refreshClock()` (bestehendes 1s-Polling von `/api/status`) wird
erweitert:

```js
if (d.tubeTest !== undefined) {
  tubeTestRunning = d.tubeTest;
  document.getElementById('tubeTestBtn').textContent =
    tubeTestRunning ? 'Abbrechen' : 'Röhrentest starten';
  document.getElementById('tubeTestStatus').textContent =
    tubeTestRunning ? `Ziffer ${d.tubeTestDigit}/9` : 'bereit';
}
```

### 3.3 Neue Endpunkte (`web_server.ino`)

```cpp
server.on("/api/tubetest/start", HTTP_POST, [](AsyncWebServerRequest *req){
  startTubeTest();
  req->send(200, "application/json", "{\"ok\":true}");
});

server.on("/api/tubetest/stop", HTTP_POST, [](AsyncWebServerRequest *req){
  stopTubeTest();
  req->send(200, "application/json", "{\"ok\":true}");
});
```

### 3.4 `/api/status` Erweiterung

```cpp
doc["tubeTest"]      = tubeTestActive;
doc["tubeTestDigit"] = tubeTestDigit;
```

`StaticJsonDocument<384>` auf `448` erhöhen (zwei zusätzliche Felder).

---

## 4. Nicht Teil dieser Änderung

- Kein physischer Tasten- oder IR-Trigger (nur Web-UI).
- Keine Konfigurierbarkeit der Anzeigedauer (fest 2s) oder Ziffernfolge
  über die Web-UI.
- Kein automatischer Abbruch bei Nacht-Modus-Wechsel während eines
  laufenden Tests (siehe Edge-Case-Hinweis in Abschnitt 2).
- Keine Änderung an NeoPixel-Verhalten während des Tests.

---

## 5. Testplan (manuell, auf Hardware)

- **Normalablauf:** Röhrentest starten → alle 6 Röhren zeigen
  synchron 0,1,2…9 für je ~2s, danach automatische Rückkehr zur
  aktuellen Uhrzeit. Button-Beschriftung/Badge wechseln währenddessen
  korrekt zwischen "Röhrentest starten"/"Abbrechen" und
  "bereit"/"Ziffer n/9".
- **Abbruch:** Während des Tests auf "Abbrechen" klicken → sofortige
  Rückkehr zur Uhrzeit, kein Hängenbleiben.
- **Neustart während laufendem Test:** Start-Endpoint erneut aufrufen
  → Test beginnt sichtbar wieder bei Ziffer 0.
- **Interaktion Nacht-Modus:** Test im `NIGHT_DIM`/`NIGHT_DARK`-Zustand
  starten → Röhren leuchten während des Tests mit voller Helligkeit;
  nach Testende kehrt die Dimmung korrekt zum vorherigen Zustand
  zurück (kein dauerhaftes "hängenbleiben" bei 100%).
- **Interaktion Einstellmodus/Slot:** Test während laufender
  Slot-Animation bzw. während SET-Taste gedrückt/Editiermodus aktiv
  auslösen → beide werden sauber abgebrochen, kein doppeltes
  Beschreiben der Röhren.
