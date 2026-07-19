# Röhrentest Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a Web-UI-triggered diagnostic test that shows the same digit (0–9, 2s each) on all 6 Nixie tubes simultaneously, so a bad cathode or a cold solder joint on a transistor/MCP23017 becomes visible.

**Architecture:** A small non-blocking state machine (`tube_test.ino`, modeled on the existing `updateSlotAnimation()`/`updateDigitFade()` pattern) drives 6 tubes in lock-step. It forces full HV-anode brightness for the duration, is guarded against interference from the normal clock/slot/edit-mode logic in `loop()`, and is exposed via two new POST endpoints plus two new `/api/status` fields. The digit-fill/finished logic is pure and lives in a header (`tube_test_math.h`) so it can be host-unit-tested with g++, matching the existing `digit_fade_math.h` convention.

**Tech Stack:** Arduino/ESP32-S3 (`.ino` files, concatenated by the Arduino build system), ESPAsyncWebServer + ArduinoJson for the HTTP API, vanilla JS/HTML for the Web-UI (single `WEB_PAGE` PROGMEM string in `web_server.ino`), g++ for host-side unit tests of pure-math headers.

## Global Constraints

- Digit display duration: fixed **2000ms** per digit, not configurable via UI (per spec §4).
- Sequence is always **0→9**, no configurability (per spec §4).
- Web-UI only — no physical button, no IR action for this feature (per spec §4 and overview).
- Test forces HV-dimmer duty to **255** (full brightness) for its duration, then restores the duty matching whatever `nightState` is current when it stops (per spec §1).
- Starting the test while already running restarts it at digit 0 (no error). Stopping while not running is a no-op that still returns `{"ok":true}` (per spec §1).
- The feature lives in the existing "🎰 Slot-Animation" card, renamed to "🎰 Slot-Animation & Röhrentest" (per user decision in brainstorming).
- No automated firmware-level test harness exists in this repo (Arduino/ESP32 target) — verification of `tube_test.ino` itself and the Web-UI is manual, on real hardware, following spec §5. Only the pure-math header gets an automated (host g++) test, matching the `digit_fade_math.h` precedent.

---

### Task 1: Pure digit/completion math (`tube_test_math.h`) + host test

**Files:**
- Create: `tube_test_math.h`
- Create: `test/tube_test_math_test.cpp`

**Interfaces:**
- Produces: `void tubeTestFillDigits(uint8_t digits[6], uint8_t testDigit)` — fills all 6 slots with `testDigit`. Used by Task 2.
- Produces: `bool tubeTestIsFinished(uint8_t testDigit)` — returns `true` once `testDigit` has advanced past 9 (i.e. `testDigit > 9`). Used by Task 2.

- [ ] **Step 1: Write the failing test**

Create `test/tube_test_math_test.cpp`:

```cpp
// Host-seitiger Unit-Test für die reine Röhrentest-Logik (kein Arduino-Framework nötig).
// Kompilieren & ausführen:
//   g++ -std=c++17 -Wall -o /tmp/tube_test_math_test test/tube_test_math_test.cpp && /tmp/tube_test_math_test
#include "../tube_test_math.h"
#include <cassert>
#include <cstdio>
#include <cstring>

int main() {
  // tubeTestFillDigits: alle 6 Slots erhalten denselben Wert
  uint8_t digits[6];
  tubeTestFillDigits(digits, 0);
  for (int i = 0; i < 6; i++) assert(digits[i] == 0);

  tubeTestFillDigits(digits, 7);
  for (int i = 0; i < 6; i++) assert(digits[i] == 7);

  tubeTestFillDigits(digits, 9);
  for (int i = 0; i < 6; i++) assert(digits[i] == 9);

  // tubeTestIsFinished: 0..9 laufen weiter, erst >9 ist Ende
  for (uint8_t d = 0; d <= 9; d++) assert(!tubeTestIsFinished(d));
  assert(tubeTestIsFinished(10));
  assert(tubeTestIsFinished(255));

  printf("tube_test_math_test: alle Assertions OK\n");
  return 0;
}
```

- [ ] **Step 2: Run test to verify it fails (header doesn't exist yet)**

Run: `g++ -std=c++17 -Wall -o /tmp/tube_test_math_test test/tube_test_math_test.cpp`
Expected: FAIL — `fatal error: ../tube_test_math.h: No such file or directory`

- [ ] **Step 3: Write minimal implementation**

Create `tube_test_math.h`:

```cpp
#pragma once
#include <stdint.h>

// Reine Logik für den Röhrentest (kein Arduino-Bezug), damit sie mit
// einem Host-Compiler (g++) unit-testbar ist — siehe
// test/tube_test_math_test.cpp.

// Füllt alle 6 Röhren-Slots mit derselben Testziffer.
inline void tubeTestFillDigits(uint8_t digits[6], uint8_t testDigit) {
  for (uint8_t i = 0; i < 6; i++) digits[i] = testDigit;
}

// true, sobald die Ziffernfolge 0-9 durchlaufen ist (testDigit > 9).
inline bool tubeTestIsFinished(uint8_t testDigit) {
  return testDigit > 9;
}
```

- [ ] **Step 4: Run test to verify it passes**

Run: `g++ -std=c++17 -Wall -o /tmp/tube_test_math_test test/tube_test_math_test.cpp && /tmp/tube_test_math_test`
Expected: PASS — prints `tube_test_math_test: alle Assertions OK`

- [ ] **Step 5: Commit**

```bash
git add tube_test_math.h test/tube_test_math_test.cpp
git commit -m "feat: Röhrentest-Kernlogik (Ziffernfüllung/Testende) mit Host-Unit-Test"
```

---

### Task 2: State machine (`tube_test.ino`) + wiring into `NixieClockUltra.ino`

**Files:**
- Create: `tube_test.ino`
- Modify: `NixieClockUltra.ino:196-200` (add globals block)
- Modify: `NixieClockUltra.ino:271-274` (add forward declarations)
- Modify: `NixieClockUltra.ino:393-394` (guard `handleEditMode()`)
- Modify: `NixieClockUltra.ino:409` (add `updateTubeTest();` call)
- Modify: `NixieClockUltra.ino:427` (add `!tubeTestActive` guard)

**Interfaces:**
- Consumes (from existing code): `nixieWrite(uint8_t digits[6])` (`nixie_driver.ino`), `hvDimmerSetDuty(uint8_t duty0to255)` (`hv_dimmer.ino`), `cancelDigitFade()` (`digit_fade.ino`), `setDisplayTime(uint8_t h, uint8_t m, uint8_t s)` (`display.ino`), globals `nightState`, `hvDimPct`, `prevNightState`, `slotActive`, `dateShowActive`, `editState`, `curHour`, `curMin`, `curSec`, `displayDigits[6]` (all `NixieClockUltra.ino`), `tubeTestFillDigits()`/`tubeTestIsFinished()` (Task 1).
- Produces: `void startTubeTest()`, `void updateTubeTest()`, `void stopTubeTest()`, globals `bool tubeTestActive`, `uint8_t tubeTestDigit`. Used by Task 3 (Web-UI endpoints and `/api/status`).

- [ ] **Step 1: Add globals to `NixieClockUltra.ino`**

In `NixieClockUltra.ino`, insert after the existing "Datum-Anzeige nach Slot" block (currently lines 198–200, right before the "Fade-In beim Start" comment):

```cpp
// Röhrentest
bool     tubeTestActive    = false;
uint8_t  tubeTestDigit     = 0;      // 0–9, aktuell angezeigte Testziffer
uint32_t tubeTestStepStart = 0;
#define  TUBE_TEST_STEP_MS 2000
```

- [ ] **Step 2: Add forward declarations**

In `NixieClockUltra.ino`, after the existing line `void updateNightMode();` (line 274), add:

```cpp
void startTubeTest();
void updateTubeTest();
void stopTubeTest();
```

(Same reason as the existing forward decls above it — functions defined in a later-concatenated `.ino` file but called from `loop()`/`setup()` in the main file. Whole-sketch prototype auto-generation is unreliable in this project due to the raw-string-literal in `web_server.ino`, see the comment already present at `NixieClockUltra.ino:269-270`.)

- [ ] **Step 3: Create `tube_test.ino`**

```cpp
// ═══════════════════════════════════════════════════════════
//  RÖHRENTEST – alle 6 Röhren zeigen synchron dieselbe Ziffer
//  0..9 für je 2s, um defekte Kathoden/Lötstellen sichtbar zu machen
// ═══════════════════════════════════════════════════════════
#include "tube_test_math.h"
#include <string.h>

void startTubeTest() {
  cancelDigitFade();
  slotActive     = false;   // laufende Slot-Animation abbrechen
  dateShowActive = false;   // laufende Datumsanzeige abbrechen
  editState      = EDIT_NONE;   // Einstellmodus verlassen

  tubeTestActive    = true;
  tubeTestDigit     = 0;
  tubeTestStepStart = millis();

  hvDimmerSetDuty(255);   // volle Helligkeit erzwingen, unabhängig von Nacht-Modus

  tubeTestFillDigits(displayDigits, tubeTestDigit);
  nixieWrite(displayDigits);   // sofort hart, kein Fade
}

void updateTubeTest() {
  if (!tubeTestActive) return;
  if (millis() - tubeTestStepStart < TUBE_TEST_STEP_MS) return;

  tubeTestDigit++;
  if (tubeTestIsFinished(tubeTestDigit)) {
    stopTubeTest();
    return;
  }
  tubeTestStepStart = millis();
  tubeTestFillDigits(displayDigits, tubeTestDigit);
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

- [ ] **Step 4: Guard `handleEditMode()` in `loop()`**

In `NixieClockUltra.ino`, replace (currently lines 393–394):

```cpp
  // --- Einstellmodus ---
  handleEditMode();
```

with:

```cpp
  // --- Einstellmodus (pausiert während Röhrentest) ---
  if (!tubeTestActive) handleEditMode();
```

- [ ] **Step 5: Call `updateTubeTest()` in `loop()`**

In `NixieClockUltra.ino`, right after the existing line `updateDigitFade();` (line 409), add:

```cpp
  updateTubeTest();
```

- [ ] **Step 6: Guard the per-second clock/slot-trigger block**

In `NixieClockUltra.ino`, change (currently line 427):

```cpp
      if (!slotActive && !dateShowActive) {
```

to:

```cpp
      if (!slotActive && !dateShowActive && !tubeTestActive) {
```

- [ ] **Step 7: Verify the sketch still compiles**

This project has no CLI/CI compile step (`arduino --verify` hangs headless in this environment — do not attempt it). Instead, manually re-read the 3 modified `NixieClockUltra.ino` regions plus the new `tube_test.ino` and confirm:
- Every function/global used in `tube_test.ino` (`nixieWrite`, `hvDimmerSetDuty`, `cancelDigitFade`, `setDisplayTime`, `nightState`, `hvDimPct`, `prevNightState`, `slotActive`, `dateShowActive`, `editState`, `curHour/curMin/curSec`, `displayDigits`, `tubeTestFillDigits`, `tubeTestIsFinished`) is declared/defined somewhere in the sketch before use.
- No duplicate global/`#define` names were introduced (grep for `tubeTestActive`, `tubeTestDigit`, `tubeTestStepStart`, `TUBE_TEST_STEP_MS` — each should appear exactly once as a definition).
- Braces balance in every changed block.

Run: `grep -c "tubeTestActive\|tubeTestDigit\|tubeTestStepStart" NixieClockUltra.ino tube_test.ino`
Expected: each file shows the variables referenced consistently (globals defined once in `NixieClockUltra.ino`, used in `tube_test.ino`).

- [ ] **Step 8: Commit**

```bash
git add NixieClockUltra.ino tube_test.ino
git commit -m "feat: Röhrentest-Statemaschine (alle 6 Röhren synchron 0-9)"
```

---

### Task 3: Web-UI card, endpoints, and `/api/status` fields

**Files:**
- Modify: `web_server.ino:88` (card heading)
- Modify: `web_server.ino:104-107` (add button+badge row inside the same card)
- Modify: `web_server.ino:194-195` (extend `refreshClock()` with tube-test status)
- Modify: `web_server.ino` (add `toggleTubeTest()` JS function near `triggerSlot()`, currently lines 249-252)
- Modify: `web_server.ino:428-447` (`/api/status` handler — add fields + bump buffer size)
- Modify: `web_server.ino:505-511` (add two new endpoints after the existing `/api/slot` endpoint)

**Interfaces:**
- Consumes: `startTubeTest()`, `stopTubeTest()`, `tubeTestActive`, `tubeTestDigit` (Task 2).

- [ ] **Step 1: Rename the card heading**

In `web_server.ino`, change (line 88):

```html
  <h2>🎰 Slot-Animation</h2>
```

to:

```html
  <h2>🎰 Slot-Animation &amp; Röhrentest</h2>
```

- [ ] **Step 2: Add the Röhrentest row to the same card**

In `web_server.ino`, the card currently ends with (lines 104-107):

```html
  <div class="row">
    <button onclick="triggerSlot()">Slot-Machine!</button>
    <span class="badge" id="slotStatus">bereit</span>
  </div>
</div>
```

Change to:

```html
  <div class="row">
    <button onclick="triggerSlot()">Slot-Machine!</button>
    <span class="badge" id="slotStatus">bereit</span>
  </div>
  <div class="row">
    <button id="tubeTestBtn" onclick="toggleTubeTest()">Röhrentest starten</button>
    <span class="badge" id="tubeTestStatus">bereit</span>
  </div>
</div>
```

- [ ] **Step 3: Add the `toggleTubeTest()` JS function**

In `web_server.ino`, right after the existing `triggerSlot()` function (currently lines 249-252):

```js
async function triggerSlot(){
  await api('/api/slot',{});
  document.getElementById('slotStatus').textContent='läuft…';
}
```

add:

```js
let tubeTestRunning=false;
async function toggleTubeTest(){
  if(tubeTestRunning) await api('/api/tubetest/stop',{});
  else                await api('/api/tubetest/start',{});
}
```

- [ ] **Step 4: Extend `refreshClock()` with tube-test status**

In `web_server.ino`, right after the existing line (currently line 195):

```js
  if(d.slot!==undefined) document.getElementById('slotStatus').textContent=d.slot?'läuft…':'bereit';
```

add:

```js
  if(d.tubeTest!==undefined){
    tubeTestRunning=d.tubeTest;
    document.getElementById('tubeTestBtn').textContent=tubeTestRunning?'Abbrechen':'Röhrentest starten';
    document.getElementById('tubeTestStatus').textContent=tubeTestRunning?('Ziffer '+d.tubeTestDigit+'/9'):'bereit';
  }
```

- [ ] **Step 5: Extend `/api/status` with the two new fields**

In `web_server.ino`, change the handler's buffer size and add two fields (currently lines 428-447):

```cpp
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *req) {
    StaticJsonDocument<384> doc;
```

to:

```cpp
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *req) {
    StaticJsonDocument<448> doc;
```

and, right after the existing line `doc["slotSpeed"]  = slotSpeedPct;`, add:

```cpp
    doc["tubeTest"]      = tubeTestActive;
    doc["tubeTestDigit"] = tubeTestDigit;
```

- [ ] **Step 6: Add the two new endpoints**

In `web_server.ino`, right after the existing `/api/slot` endpoint block (currently lines 505-511):

```cpp
  server.on("/api/slot", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      startSlotAnimation(curHour, curMin, curSec);
      req->send(200, "application/json", "{\"ok\":true}");
    }
  );
```

add:

```cpp
  server.on("/api/tubetest/start", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      startTubeTest();
      req->send(200, "application/json", "{\"ok\":true}");
    }
  );

  server.on("/api/tubetest/stop", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      stopTubeTest();
      req->send(200, "application/json", "{\"ok\":true}");
    }
  );
```

- [ ] **Step 7: Manually re-check the HTML/JS/C++ edits**

No compile step available for this Arduino sketch in this environment (see Task 2 Step 7). Re-read the 6 changed regions in `web_server.ino` and confirm:
- The `WEB_PAGE` raw string literal (`R"rawliteral( ... )rawliteral"`) is not broken — every added `<div>`/`<button>`/`<span>` tag is properly closed and the new HTML sits *inside* the existing raw string, not accidentally after its closing `)rawliteral"`.
- `tubeTestBtn` and `tubeTestStatus` element IDs are unique (grep confirms exactly one `id="tubeTestBtn"` and one `id="tubeTestStatus"`).
- Every new `server.on(...)` lambda ends with a matching `);` and sits inside `setupWebServer()`.

Run: `grep -n "rawliteral\|tubeTestBtn\|tubeTestStatus" web_server.ino`
Expected: exactly one `R"rawliteral(` opening, one `)rawliteral"` closing, and both IDs appearing exactly once in the HTML and once in the JS reference each.

- [ ] **Step 8: Commit**

```bash
git add web_server.ino
git commit -m "feat: Röhrentest-Button in Web-UI, neue /api/tubetest-Endpunkte"
```

---

### Task 4: Manual hardware verification

**Files:** none (no code changes — this is the manual verification pass from spec §5)

This task cannot be executed by an agent; it requires flashing real hardware and observing the 6 physical Nixie tubes. Perform it yourself (or ask the user to) before considering the feature done:

- [ ] **Step 1: Flash the firmware** to the device via the Arduino IDE (USB CDC On Boot must stay **Enabled** — see the existing warning box in `aufbau.html`/the Web-UI).
- [ ] **Step 2: Normal run** — open the Web-UI, click "Röhrentest starten" in the "Slot-Animation & Röhrentest" card. Confirm: all 6 tubes show `0,1,2,…,9` synchronously, ~2s each; button label switches to "Abbrechen"; badge shows "Ziffer n/9"; after digit 9 the display automatically returns to the current time and the button/badge reset to "Röhrentest starten"/"bereit".
- [ ] **Step 3: Abort** — start the test again, click "Abbrechen" mid-run. Confirm immediate return to the time display, no stuck digits.
- [ ] **Step 4: Restart while running** — start the test, then call `POST /api/tubetest/start` again (e.g. via curl or by clicking the button twice quickly if the UI allows it). Confirm the test visibly restarts at digit 0.
- [ ] **Step 5: Night-mode interaction** — force `NIGHT_DIM` or `NIGHT_DARK` (via the Nacht-Modus card or by covering the LDR), then start the Röhrentest. Confirm tubes run at full brightness during the test, and dimming correctly resumes afterward (no stuck-at-full-brightness state).
- [ ] **Step 6: Edit-mode/slot interaction** — trigger the Slot-Machine animation, then immediately start the Röhrentest. Confirm the slot animation is cleanly aborted and the test runs correctly. Repeat by pressing/holding the physical SET button (entering edit mode) and starting the test from the Web-UI — confirm edit mode is exited and no garbled display results.

---

## Self-Review Notes

- **Spec coverage:** §1 (state machine) → Task 2; §2 (loop integration/guards) → Task 2; §3 (Web-UI, card, JS, endpoints, status fields) → Task 3; §5 (manual test plan) → Task 4. §4 ("Nicht Teil dieser Änderung") is respected by omission (no physical/IR trigger, no configurable duration, no NeoPixel changes added anywhere in this plan).
- **Placeholder scan:** no TBD/TODO; every step has complete, pasteable code.
- **Type consistency:** `tubeTestActive` (bool), `tubeTestDigit` (uint8_t), `tubeTestStepStart` (uint32_t) are defined once in `NixieClockUltra.ino` (Task 2 Step 1) and referenced with identical names/types in `tube_test.ino` (Task 2 Step 3) and `web_server.ino` (Task 3 Steps 4-5). `tubeTestFillDigits`/`tubeTestIsFinished` signatures match between their definition (Task 1) and their only call sites (Task 2 Step 3).
