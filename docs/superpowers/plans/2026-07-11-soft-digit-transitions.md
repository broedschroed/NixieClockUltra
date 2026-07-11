# Weicher Ziffernwechsel & Slot-Machine-Geschwindigkeit Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** ESP32-S3-Nixie-Uhr um zwei unabhängige, per Web-UI schaltbare Firmware-Features erweitern: (1) weicher HV-Dimmer-Crossfade beim Ziffernwechsel (Sekundentakt und/oder Zeit↔Datum-Wechsel), (2) einstellbare Geschwindigkeit der Slot-Machine-Animation.

**Architecture:** Neues Modul `digit_fade.ino` implementiert eine non-blocking State-Machine, die den bestehenden `hv_dimmer.ino`-PWM-Kanal für einen kurzen Crossfade nutzt und beim Nulldurchgang (Minimalhelligkeit) über die bestehende `nixieWrite()` die neuen Ziffern schreibt. `display.ino` bekommt eine gemeinsame `commitDigits()`-Funktion, über die sowohl der bestehende harte Sofort-Wechsel als auch der neue weiche Wechsel laufen. Die reine Interpolations-Mathematik des Fades wird in eine Arduino-unabhängige Header-Funktion ausgelagert und mit einem echten Host-Unit-Test (g++) abgesichert. Slot-Machine-Geschwindigkeit skaliert zwei bestehende Timing-Konstanten in `updateSlotAnimation()` um einen NVS-persistenten Faktor.

**Tech Stack:** ESP32-S3 Arduino-Firmware (C++, kein RTOS-Framework außer FreeRTOS-Primitiven), `Preferences`-NVS, `ESPAsyncWebServer` + `ArduinoJson` für die Web-API, LEDC-Hardware-PWM (`ledcWrite`) für den HV-Dimmer.

**Hinweis zur Verifikation:** Dieses Projekt hat keine automatisierte Firmware-Testsuite und in dieser Umgebung ist kein `arduino-cli`/Compiler für ESP32 verfügbar — Verifikation der Arduino-Tasks erfolgt daher per manuellem Code-Trace (im jeweiligen Task ausgeschrieben) statt per Testlauf. Eine Ausnahme: Die Interpolations-Mathematik des Fades (Task 1) ist Arduino-unabhängig und wird mit einem echten, mit `g++` ausführbaren Host-Unit-Test abgesichert (`g++` ist in dieser Umgebung vorhanden, geprüft). Die eigentliche funktionale Verifikation auf echter Hardware erfolgt in Task 7 durch den Nutzer (Flashen + manueller Test gemäß Spec-Testplan).

## Global Constraints

- Fade-Dauer Sekundentakt: **100ms** fest codiert. (Spec Abschnitt 1.1)
- Fade-Dauer Zeit↔Datum-Wechsel: **200ms** fest codiert. (Spec Abschnitt 1.1)
- Minimalhelligkeit während Fade: **5% Duty** (≈13 von 255), fest codiert (`DIGIT_FADE_MIN_DUTY`). (Spec Abschnitt 1.1)
- Weicher Ziffernwechsel nur aktiv wenn `nightState == NIGHT_NORMAL`; zentral erzwungen in `commitDigits()`, nicht nur an den Aufrufstellen. (Spec Abschnitt 1.1, 1.3)
- Beide Soft-Fade-Toggles (`sfSecEn`, `sfDateEn`) und Slot-Speed (`slotSpeed`) sind NVS-persistent, Default: Toggles `false`, Slot-Speed `100`. (Spec Abschnitt 1.6, 2.2)
- Slot-Speed-Bereich: **20%–100%**, keine Beschleunigung über heutiges Verhalten hinaus. (Spec Abschnitt 2.1)
- Kein Pro-Röhre-Crossfade, keine Web-UI-Konfigurierbarkeit von Fade-Dauer/Minimalhelligkeit. (Spec "Nicht Teil dieser Änderung")

---

### Task 1: Reine Fade-Interpolations-Mathematik + Host-Unit-Test

**Files:**
- Create: `digit_fade_math.h`
- Create: `test/digit_fade_math_test.cpp`

**Interfaces:**
- Produces: `uint8_t fadeDutyForStep(bool risingUp, uint8_t stepsDone, uint8_t stepsTotal, uint8_t minDuty, uint8_t maxDuty)` — reine Funktion, keine Arduino-Abhängigkeiten. Wird von Task 2 (`digit_fade.ino`) konsumiert.

- [ ] **Step 1: Testverzeichnis anlegen und fehlschlagenden Test schreiben**

Erstelle `test/digit_fade_math_test.cpp`:

```cpp
// Host-seitiger Unit-Test für die reine Interpolations-Mathematik des
// weichen Ziffernwechsels (kein Arduino-Framework nötig).
// Kompilieren & ausführen:
//   g++ -std=c++17 -Wall -o /tmp/digit_fade_math_test test/digit_fade_math_test.cpp && /tmp/digit_fade_math_test
#include "../digit_fade_math.h"
#include <cassert>
#include <cstdio>

int main() {
  // Fade-down: Start bei maxDuty, Ende exakt bei minDuty
  assert(fadeDutyForStep(false, 0, 10, 13, 255) == 255);
  assert(fadeDutyForStep(false, 10, 10, 13, 255) == 13);

  // Fade-up: Start bei minDuty, Ende exakt bei maxDuty
  assert(fadeDutyForStep(true, 0, 10, 13, 255) == 13);
  assert(fadeDutyForStep(true, 10, 10, 13, 255) == 255);

  // Monoton fallend über alle Zwischenschritte (fade-down)
  uint8_t prev = 255;
  for (uint8_t s = 1; s <= 10; s++) {
    uint8_t d = fadeDutyForStep(false, s, 10, 13, 255);
    assert(d <= prev);
    prev = d;
  }

  // Monoton steigend über alle Zwischenschritte (fade-up)
  prev = 13;
  for (uint8_t s = 1; s <= 10; s++) {
    uint8_t d = fadeDutyForStep(true, s, 10, 13, 255);
    assert(d >= prev);
    prev = d;
  }

  // Division-Guard: stepsTotal=0 darf nicht durch Null teilen
  assert(fadeDutyForStep(false, 0, 0, 13, 255) == 255);
  assert(fadeDutyForStep(true, 0, 0, 13, 255) == 13);

  // 200ms-Fade (20 Schritte) an einer Zwischenposition
  assert(fadeDutyForStep(false, 5, 20, 13, 255) == 255 - (uint8_t)((255 - 13) * 5 / 20));

  printf("digit_fade_math_test: alle Assertions OK\n");
  return 0;
}
```

- [ ] **Step 2: Test ausführen, Fehlschlag bestätigen**

Run: `g++ -std=c++17 -Wall -o /tmp/digit_fade_math_test test/digit_fade_math_test.cpp && /tmp/digit_fade_math_test`
Expected: FAIL — Compile-Fehler `fatal error: ../digit_fade_math.h: No such file or directory` (Header existiert noch nicht).

- [ ] **Step 3: Minimalimplementierung schreiben**

Erstelle `digit_fade_math.h`:

```cpp
#pragma once
#include <stdint.h>

// Reine Interpolations-Mathematik für den HV-Dimmer-Crossfade beim
// weichen Ziffernwechsel. Keine Arduino-Abhängigkeiten, damit sie mit
// einem Host-Compiler (g++) unit-testbar ist — siehe
// test/digit_fade_math_test.cpp.
//
// risingUp=false (Fade-Down): stepsDone=0 → maxDuty, stepsDone=stepsTotal → minDuty
// risingUp=true  (Fade-Up):   stepsDone=0 → minDuty, stepsDone=stepsTotal → maxDuty
inline uint8_t fadeDutyForStep(bool risingUp, uint8_t stepsDone, uint8_t stepsTotal,
                                uint8_t minDuty, uint8_t maxDuty) {
  if (stepsTotal == 0) stepsTotal = 1;
  if (stepsDone > stepsTotal) stepsDone = stepsTotal;
  uint16_t span = (uint16_t)(maxDuty - minDuty) * stepsDone / stepsTotal;
  return risingUp ? (uint8_t)(minDuty + span) : (uint8_t)(maxDuty - span);
}
```

- [ ] **Step 4: Test erneut ausführen, Erfolg bestätigen**

Run: `g++ -std=c++17 -Wall -o /tmp/digit_fade_math_test test/digit_fade_math_test.cpp && /tmp/digit_fade_math_test`
Expected: PASS — Ausgabe `digit_fade_math_test: alle Assertions OK`, Exit-Code 0.

- [ ] **Step 5: Commit**

```bash
git add digit_fade_math.h test/digit_fade_math_test.cpp
git commit -m "feat: reine Interpolations-Mathematik für Ziffern-Crossfade + Host-Unit-Test"
```

---

### Task 2: `digit_fade.ino` — Non-blocking Fade-State-Machine

**Files:**
- Create: `digit_fade.ino`

**Interfaces:**
- Consumes: `fadeDutyForStep(bool, uint8_t, uint8_t, uint8_t, uint8_t) -> uint8_t` (Task 1); bestehendes `void nixieWrite(uint8_t digits[6])` (`nixie_driver.ino`); bestehendes `void hvDimmerSetDuty(uint8_t duty0to255)` (`hv_dimmer.ino`); bestehendes globales `NightState nightState` (`NixieClockUltra.ino`, wird hier nicht gelesen, nur von Task 3 aus geprüft).
- Produces: `void startDigitFade(uint8_t newDigits[6], uint16_t fadeMs)`, `void updateDigitFade()`, `void cancelDigitFade()` — werden von Task 3 (`display.ino`, `NixieClockUltra.ino`) konsumiert.

- [ ] **Step 1: Modul schreiben**

Erstelle `digit_fade.ino`:

```cpp
// ═══════════════════════════════════════════════════════════
//  WEICHER ZIFFERNWECHSEL – HV-Dimmer-Crossfade
//  Non-blocking State-Machine, angetrieben von updateDigitFade()
//  in loop(). Nutzt hv_dimmer.ino (Duty) + nixie_driver.ino
//  (Ziffern schreiben). Reine Interpolations-Mathematik in
//  digit_fade_math.h (siehe Task 1, host-testbar).
// ═══════════════════════════════════════════════════════════

#include <string.h>
#include "digit_fade_math.h"

#define DIGIT_FADE_MIN_DUTY   13   // ~5% von 255
#define DIGIT_FADE_STEP_MS    5    // Schrittintervall

enum FadeDir { FADE_DOWN, FADE_UP };

static bool     fadeRunning    = false;
static FadeDir  fadeDir        = FADE_DOWN;
static uint8_t  fadeTargetDigits[6];
static uint32_t fadeLastStepMs = 0;
static uint8_t  fadeStepsTotal = 1;
static uint8_t  fadeStepsDone  = 0;

// Schließt einen laufenden Fade sofort ab: schreibt die Zielziffern
// und setzt volle Helligkeit. Wird intern verwendet, wenn ein neuer
// Fade angefordert wird während einer noch läuft.
static void fadeFinishImmediately() {
  if (!fadeRunning) return;
  nixieWrite(fadeTargetDigits);
  hvDimmerSetDuty(255);
  fadeRunning = false;
}

// Schließt einen laufenden Fade sofort ab, OHNE die Helligkeit
// anzufassen (der Aufrufer setzt sie danach selbst, z.B. beim
// Nacht-Modus-Wechsel). No-op wenn kein Fade läuft.
void cancelDigitFade() {
  if (!fadeRunning) return;
  nixieWrite(fadeTargetDigits);
  fadeRunning = false;
}

// Startet einen neuen Fade zu newDigits über fadeMs (aufgeteilt in
// fadeMs/2 Abblenden + fadeMs/2 Aufblenden). fadeMs muss > 0 sein
// (Aufrufer in display.ino prüft das bereits vor dem Aufruf).
void startDigitFade(uint8_t newDigits[6], uint16_t fadeMs) {
  fadeFinishImmediately();
  memcpy(fadeTargetDigits, newDigits, 6);
  fadeStepsTotal = (uint8_t)((fadeMs / 2) / DIGIT_FADE_STEP_MS);
  if (fadeStepsTotal < 1) fadeStepsTotal = 1;
  fadeStepsDone  = 0;
  fadeDir        = FADE_DOWN;
  fadeRunning    = true;
  fadeLastStepMs = millis();
}

// In loop() bei jedem Durchlauf aufrufen. No-op wenn kein Fade läuft.
void updateDigitFade() {
  if (!fadeRunning) return;
  if (millis() - fadeLastStepMs < DIGIT_FADE_STEP_MS) return;
  fadeLastStepMs = millis();
  fadeStepsDone++;

  uint8_t duty = fadeDutyForStep(fadeDir == FADE_UP, fadeStepsDone, fadeStepsTotal,
                                  DIGIT_FADE_MIN_DUTY, 255);
  hvDimmerSetDuty(duty);

  if (fadeStepsDone >= fadeStepsTotal) {
    if (fadeDir == FADE_DOWN) {
      nixieWrite(fadeTargetDigits);   // Ziffern bei Minimalhelligkeit umschalten
      fadeDir       = FADE_UP;
      fadeStepsDone = 0;
    } else {
      fadeRunning = false;            // Fade komplett, Duty ist bereits 255
    }
  }
}
```

- [ ] **Step 2: Manueller Trace zur Verifikation (kein Compiler in dieser Umgebung verfügbar)**

Für einen Aufruf `startDigitFade(newDigits, 100)`: `fadeStepsTotal = (100/2)/5 = 10`.
Trace der ersten, mittleren und letzten `updateDigitFade()`-Aufrufe (jeweils ≥5ms nach dem letzten Schritt):

| Aufruf # | fadeDir vorher | fadeStepsDone (nach ++) | duty (`fadeDutyForStep`) | Aktion |
|---|---|---|---|---|
| 1 | DOWN | 1 | 255-(242·1/10)=231 | `hvDimmerSetDuty(231)` |
| 5 | DOWN | 5 | 255-(242·5/10)=134 | `hvDimmerSetDuty(134)` |
| 10 | DOWN | 10 | 13 | `hvDimmerSetDuty(13)`, dann `nixieWrite(fadeTargetDigits)`, `fadeDir=UP`, `fadeStepsDone=0` |
| 11 | UP | 1 | 13+(242·1/10)=37 | `hvDimmerSetDuty(37)` |
| 20 | UP | 10 | 255 | `hvDimmerSetDuty(255)`, `fadeRunning=false` |

Stimmt mit Task 1's getesteter `fadeDutyForStep`-Logik überein (dieselbe Funktion, hier nur die State-Machine-Verdrahtung geprüft) — kein eigenständiger Fehlerpfad in diesem Trace. Bestätige das per Lesen des Codes gegen diese Tabelle, bevor du fortfährst.

- [ ] **Step 3: Commit**

```bash
git add digit_fade.ino
git commit -m "feat: non-blocking Fade-State-Machine für weichen Ziffernwechsel"
```

---

### Task 3: Verdrahtung in `display.ino` + `NixieClockUltra.ino`

**Files:**
- Modify: `display.ino` (komplett, aktuell 62 Zeilen)
- Modify: `NixieClockUltra.ino:162` (neue Globals), `NixieClockUltra.ino:316` (NVS-Laden in `setup()`), `NixieClockUltra.ino:392-399` (Nacht-Modus-Block), `NixieClockUltra.ino:419` (Sekundentakt), `NixieClockUltra.ino:430` (Zeit→Datum), `NixieClockUltra.ino:436` (Datum→Zeit)

**Interfaces:**
- Consumes: `startDigitFade`, `updateDigitFade`, `cancelDigitFade` (Task 2); bestehendes `nixieWrite`; globales `nightState`.
- Produces: `void setDisplayTimeSoft(uint8_t h, uint8_t m, uint8_t s, uint16_t fadeMs)`, `void setDisplayDateSoft(uint16_t fadeMs)` (`display.ino`); globale `bool softFadeSecondEnabled`, `bool softFadeDateEnabled` (`NixieClockUltra.ino`) — werden von Task 4 (Web-UI) konsumiert.

- [ ] **Step 1: `display.ino` komplett ersetzen**

```cpp
// ═══════════════════════════════════════════════════════════
//  DISPLAY-UPDATE (displayDigits befüllen)
// ═══════════════════════════════════════════════════════════
#include <string.h>

// Schreibt neue Zielziffern. Bei fadeMs>0 und nightState==NIGHT_NORMAL
// läuft ein weicher HV-Dimmer-Crossfade (digit_fade.ino), sonst sofort
// hart über nixieWrite(). Kein Effekt, wenn sich nichts ändert.
static void commitDigits(uint8_t newDigits[6], uint16_t fadeMs) {
  bool changed = memcmp(newDigits, displayDigits, 6) != 0;
  memcpy(displayDigits, newDigits, 6);
  if (!changed) return;
  if (fadeMs > 0 && nightState == NIGHT_NORMAL) {
    startDigitFade(newDigits, fadeMs);
  } else {
    nixieWrite(displayDigits);
  }
}

void setDisplayTime(uint8_t h, uint8_t m, uint8_t s) {
  uint8_t d[6] = { (uint8_t)(h / 10), (uint8_t)(h % 10),
                    (uint8_t)(m / 10), (uint8_t)(m % 10),
                    (uint8_t)(s / 10), (uint8_t)(s % 10) };
  commitDigits(d, 0);
}

void setDisplayTimeSoft(uint8_t h, uint8_t m, uint8_t s, uint16_t fadeMs) {
  uint8_t d[6] = { (uint8_t)(h / 10), (uint8_t)(h % 10),
                    (uint8_t)(m / 10), (uint8_t)(m % 10),
                    (uint8_t)(s / 10), (uint8_t)(s % 10) };
  commitDigits(d, fadeMs);
}

void setDisplayDate() {
  uint8_t d[6] = { (uint8_t)(curDay / 10),   (uint8_t)(curDay % 10),
                    (uint8_t)(curMonth / 10), (uint8_t)(curMonth % 10),
                    (uint8_t)(curYear / 10),  (uint8_t)(curYear % 10) };
  commitDigits(d, 0);
}

void setDisplayDateSoft(uint16_t fadeMs) {
  uint8_t d[6] = { (uint8_t)(curDay / 10),   (uint8_t)(curDay % 10),
                    (uint8_t)(curMonth / 10), (uint8_t)(curMonth % 10),
                    (uint8_t)(curYear / 10),  (uint8_t)(curYear % 10) };
  commitDigits(d, fadeMs);
}

// ═══════════════════════════════════════════════════════════
//  SLOT-MACHINE ANIMATION
// ═══════════════════════════════════════════════════════════
void startSlotAnimation(uint8_t h, uint8_t m, uint8_t s) {
  slotTarget[0] = h / 10; slotTarget[1] = h % 10;
  slotTarget[2] = m / 10; slotTarget[3] = m % 10;
  slotTarget[4] = s / 10; slotTarget[5] = s % 10;
  for (int i = 0; i < 6; i++) slotCurrent[i] = random(10);
  slotActive   = true;
  slotStartMs  = millis();
}

void updateSlotAnimation() {
  if (!slotActive) return;
  unsigned long elapsed = millis() - slotStartMs;

  // Jede Röhre stoppt zu einem anderen Zeitpunkt
  bool allDone = true;
  for (int i = 0; i < 6; i++) {
    unsigned long stopTime = 600 + i * 180; // ms
    if (elapsed < stopTime) {
      // Noch rollend
      if ((millis() % 60) < 30) {
        slotCurrent[i] = (slotCurrent[i] + 1) % 10;
      }
      displayDigits[i] = slotCurrent[i];
      allDone = false;
    } else {
      displayDigits[i] = slotTarget[i];
    }
  }
  if (allDone) slotActive = false;
  nixieWrite(displayDigits);
}
```

(`startSlotAnimation`/`updateSlotAnimation` hier unverändert gegenüber dem aktuellen Stand belassen — Slot-Speed-Skalierung ist Task 5. `nixieWriteSafe()` entfällt, da `commitDigits()` deren Rolle übernimmt und `updateSlotAnimation()` bereits direkt `nixieWrite()` rief.)

- [ ] **Step 2: `NixieClockUltra.ino` — neue Globals**

Nach `NixieClockUltra.ino:162` (`uint8_t hvDimPct = 25; ...`) einfügen:

```cpp
bool softFadeSecondEnabled  = false;   // NVS-Key "sfSecEn"
bool softFadeDateEnabled    = false;   // NVS-Key "sfDateEn"
```

- [ ] **Step 3: `NixieClockUltra.ino` — NVS beim Boot laden**

Nach `NixieClockUltra.ino:316` (`hvDimPct = (uint8_t)constrain(...);`) einfügen:

```cpp
softFadeSecondEnabled = prefs.getBool("sfSecEn",  false);
softFadeDateEnabled   = prefs.getBool("sfDateEn", false);
```

- [ ] **Step 4: `NixieClockUltra.ino` — Nacht-Modus-Block um `cancelDigitFade()` und `updateDigitFade()` erweitern**

Ersetze (aktuell Zeilen 391–399):

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

durch:

```cpp
  // --- Nacht-Modus: Röhren-Helligkeit per HV-Dimmer steuern ---
  if (nightState != prevNightState) {
    prevNightState = nightState;
    cancelDigitFade();   // laufenden Ziffern-Fade nicht mit Duty-Wechsel kollidieren lassen
    switch (nightState) {
      case NIGHT_DARK:   hvDimmerSetDuty(0);                        break;
      case NIGHT_DIM:    hvDimmerSetDuty(hvDimPct * 255 / 100);     break;
      case NIGHT_NORMAL: hvDimmerSetDuty(255);                      break;
    }
  }
  updateDigitFade();
```

- [ ] **Step 5: `NixieClockUltra.ino` — Sekundentakt-Aufruf**

Ersetze (aktuell Zeile 417–420):

```cpp
      if (!slotActive && !dateShowActive) {
        if (triggerSlot) startSlotAnimation(curHour, curMin, curSec);
        else             setDisplayTime(curHour, curMin, curSec);
      }
```

durch:

```cpp
      if (!slotActive && !dateShowActive) {
        if (triggerSlot) startSlotAnimation(curHour, curMin, curSec);
        else             setDisplayTimeSoft(curHour, curMin, curSec, softFadeSecondEnabled ? 100 : 0);
      }
```

- [ ] **Step 6: `NixieClockUltra.ino` — Zeit→Datum-Übergang**

Ersetze (aktuell Zeile 427–431):

```cpp
  if (wasSlotActive && !slotActive) {
    dateShowActive = true;
    dateShowStart  = millis();
    setDisplayDate();
  }
```

durch:

```cpp
  if (wasSlotActive && !slotActive) {
    dateShowActive = true;
    dateShowStart  = millis();
    setDisplayDateSoft(softFadeDateEnabled ? 200 : 0);
  }
```

- [ ] **Step 7: `NixieClockUltra.ino` — Datum→Zeit-Übergang**

Ersetze (aktuell Zeile 434–437):

```cpp
  if (dateShowActive && millis() - dateShowStart >= DATE_SHOW_MS) {
    dateShowActive = false;
    setDisplayTime(curHour, curMin, curSec);
  }
```

durch:

```cpp
  if (dateShowActive && millis() - dateShowStart >= DATE_SHOW_MS) {
    dateShowActive = false;
    setDisplayTimeSoft(curHour, curMin, curSec, softFadeDateEnabled ? 200 : 0);
  }
```

- [ ] **Step 8: Manueller Trace zur Verifikation**

Mit Default-Werten (`softFadeSecondEnabled=false`, `softFadeDateEnabled=false`) rufen alle drei Stellen `setDisplayTimeSoft`/`setDisplayDateSoft` mit `fadeMs=0` auf → `commitDigits()` nimmt den `else`-Zweig (`nixieWrite(displayDigits)`) → **identisch zum bisherigen Verhalten vor dieser Änderung**. Das ist die Regressionsgarantie für den Default-Zustand. Bestätige das durch Lesen von `commitDigits()` gegen diesen Fall, bevor du fortfährst (echte Verifikation mit `softFadeSecondEnabled=true` erfolgt auf Hardware in Task 7, da hier `nightState`/`millis()` zur Laufzeit nötig sind).

- [ ] **Step 9: Commit**

```bash
git add display.ino NixieClockUltra.ino
git commit -m "feat: weichen Ziffernwechsel in Sekundentakt und Zeit/Datum-Übergang verdrahten"
```

---

### Task 4: Web-UI für weichen Ziffernwechsel

**Files:**
- Modify: `web_server.ino` (HTML-Karte, JS, zwei neue Endpunkte)

**Interfaces:**
- Consumes: globale `bool softFadeSecondEnabled`, `bool softFadeDateEnabled` (Task 3).
- Produces: `GET/POST /api/softfade` (JSON `{sec: bool, date: bool}`).

- [ ] **Step 1: Neue HTML-Karte einfügen**

In `web_server.ino` nach der Slot-Animation-Karte (nach Zeile 105, `</div>` der `🎰 Slot-Animation`-Karte, vor `<div class="card">` der `🌙 Nacht-Modus`-Karte in Zeile 107) einfügen:

```html
<div class="card">
  <h2>✨ Weicher Ziffernwechsel</h2>
  <div class="row"><label>Bei Sekundentakt</label>
    <label class="toggle"><input type="checkbox" id="sfSec" onchange="saveSoftFade()"><span class="slider"></span></label></div>
  <div class="row"><label>Bei Zeit/Datum-Wechsel</label>
    <label class="toggle"><input type="checkbox" id="sfDate" onchange="saveSoftFade()"><span class="slider"></span></label></div>
</div>
```

- [ ] **Step 2: JS-Funktionen ergänzen**

Nach der bestehenden `refreshNightMode()`-Funktion (nach Zeile 321, vor `async function setHvDimPct(v){`) einfügen:

```js
async function refreshSoftFade(){
  let d=await get('/api/softfade');
  if(d.sec===undefined)return;
  document.getElementById('sfSec').checked  = d.sec;
  document.getElementById('sfDate').checked = d.date;
}
async function saveSoftFade(){
  let r=await api('/api/softfade',{
    sec:  document.getElementById('sfSec').checked,
    date: document.getElementById('sfDate').checked
  });
  document.getElementById('statusMsg').textContent=r.ok?'Ziffernwechsel gespeichert ✓':'Fehler ✗';
}
```

Und in der Init-Sektion am Skript-Ende (aktuell Zeile 343–347):

```js
setInterval(refreshClock,1000);
refreshClock();
refreshWifi();
refreshIR();
refreshNightMode();
```

ergänze eine Zeile:

```js
setInterval(refreshClock,1000);
refreshClock();
refreshWifi();
refreshIR();
refreshNightMode();
refreshSoftFade();
```

- [ ] **Step 3: Server-Endpunkte ergänzen**

Nach dem bestehenden `/api/nightmode` POST-Handler (nach Zeile 672, vor `server.begin();` in Zeile 674) einfügen:

```cpp
  // --- Weicher Ziffernwechsel: Status lesen ---
  server.on("/api/softfade", HTTP_GET, [](AsyncWebServerRequest *req) {
    StaticJsonDocument<64> doc;
    doc["sec"]  = softFadeSecondEnabled;
    doc["date"] = softFadeDateEnabled;
    String out; serializeJson(doc, out);
    req->send(200, "application/json", out);
  });

  // --- Weicher Ziffernwechsel: Einstellungen speichern ---
  server.on("/api/softfade", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      StaticJsonDocument<64> doc;
      if (!deserializeJson(doc, data, len)) {
        softFadeSecondEnabled = doc["sec"].as<bool>();
        softFadeDateEnabled   = doc["date"].as<bool>();
        prefs.putBool("sfSecEn",  softFadeSecondEnabled);
        prefs.putBool("sfDateEn", softFadeDateEnabled);
        req->send(200, "application/json", "{\"ok\":true}");
      } else {
        req->send(400, "application/json", "{\"ok\":false}");
      }
    }
  );
```

- [ ] **Step 4: Manueller Trace zur Verifikation**

`GET /api/softfade` mit Default-Werten liefert `{"sec":false,"date":false}`. `POST /api/softfade {"sec":true,"date":false}` setzt `softFadeSecondEnabled=true`, persistiert `sfSecEn=1` in NVS, lässt `softFadeDateEnabled` unverändert auf `false` (da `doc["date"].as<bool>()` bei `false` explizit gesetzt wird — beide Felder werden bei jedem POST vollständig überschrieben, nicht partiell wie bei `/api/nightmode`; das ist bewusst, da die UI beide Checkboxen immer zusammen sendet, siehe `saveSoftFade()`). Bestätige das durch Lesen des Codes gegen dieses Beispiel.

- [ ] **Step 5: Commit**

```bash
git add web_server.ino
git commit -m "feat: Web-UI-Karte für weichen Ziffernwechsel (Sekundentakt/Datum-Toggle)"
```

---

### Task 5: Slot-Machine-Geschwindigkeit — Skalierungslogik

**Files:**
- Modify: `NixieClockUltra.ino:148` (neues Global), `NixieClockUltra.ino:188-191` (neue Timing-Globals), `NixieClockUltra.ino:302` (NVS-Laden)
- Modify: `display.ino` (`startSlotAnimation`, `updateSlotAnimation`)

**Interfaces:**
- Produces: globale `uint8_t slotSpeedPct` (`NixieClockUltra.ino`) — wird von Task 6 (Web-UI) konsumiert. Globale `uint16_t slotRollIntervalMs`, `uint16_t slotStopMs[6]` (`NixieClockUltra.ino`) — intern von `display.ino` verwendet.

- [ ] **Step 1: `NixieClockUltra.ino` — neues Global bei `SlotInterval`**

Nach `NixieClockUltra.ino:148` (`SlotInterval slotInterval = SLOT_OFF;`) einfügen:

```cpp
uint8_t slotSpeedPct = 100;   // 20–100, NVS-Key "slotSpeed" (100 = aktuelles/unverändertes Tempo)
```

- [ ] **Step 2: `NixieClockUltra.ino` — Timing-Globals bei den Slot-Variablen**

Nach `NixieClockUltra.ino:191` (`uint8_t slotCurrent[6] = {0};`) einfügen:

```cpp
uint16_t slotRollIntervalMs   = 60;
uint16_t slotStopMs[6]        = {600, 780, 960, 1140, 1320, 1500};
```

- [ ] **Step 3: `NixieClockUltra.ino` — NVS beim Boot laden**

Nach `NixieClockUltra.ino:302` (`slotInterval = (savedSlot <= (uint8_t)SLOT_1HR) ? (SlotInterval)savedSlot : SLOT_OFF;`) einfügen:

```cpp
slotSpeedPct = (uint8_t)constrain((int)prefs.getUChar("slotSpeed", 100), 20, 100);
```

- [ ] **Step 4: `display.ino` — `startSlotAnimation()` skaliert Timing bei Animationsstart**

Ersetze die `startSlotAnimation()`-Funktion (aus Task 3, Step 1 so eingefügt) durch:

```cpp
void startSlotAnimation(uint8_t h, uint8_t m, uint8_t s) {
  slotTarget[0] = h / 10; slotTarget[1] = h % 10;
  slotTarget[2] = m / 10; slotTarget[3] = m % 10;
  slotTarget[4] = s / 10; slotTarget[5] = s % 10;
  for (int i = 0; i < 6; i++) slotCurrent[i] = random(10);
  slotRollIntervalMs = (uint16_t)(60UL * 100 / slotSpeedPct);
  for (int i = 0; i < 6; i++) {
    slotStopMs[i] = (uint16_t)((600UL + (unsigned long)i * 180UL) * 100 / slotSpeedPct);
  }
  slotActive   = true;
  slotStartMs  = millis();
}
```

- [ ] **Step 5: `display.ino` — `updateSlotAnimation()` nutzt skalierte Werte**

Ersetze die `updateSlotAnimation()`-Funktion durch:

```cpp
void updateSlotAnimation() {
  if (!slotActive) return;
  unsigned long elapsed = millis() - slotStartMs;

  // Jede Röhre stoppt zu einem anderen (skalierten) Zeitpunkt
  bool allDone = true;
  for (int i = 0; i < 6; i++) {
    unsigned long stopTime = slotStopMs[i];
    if (elapsed < stopTime) {
      // Noch rollend
      if ((millis() % slotRollIntervalMs) < (slotRollIntervalMs / 2)) {
        slotCurrent[i] = (slotCurrent[i] + 1) % 10;
      }
      displayDigits[i] = slotCurrent[i];
      allDone = false;
    } else {
      displayDigits[i] = slotTarget[i];
    }
  }
  if (allDone) slotActive = false;
  nixieWrite(displayDigits);
}
```

- [ ] **Step 6: Manueller Trace zur Verifikation**

Bei `slotSpeedPct=100` (Default): `slotRollIntervalMs = 60*100/100 = 60`, `slotStopMs = {600,780,960,1140,1320,1500}` — exakt identisch zu den bisherigen fest codierten Werten (Regressionsgarantie für Default).
Bei `slotSpeedPct=20`: `slotRollIntervalMs = 60*100/20 = 300`, `slotStopMs[5] = (600+5*180)*100/20 = 1500*5 = 7500` — 5× langsamer, wie in der Spec (Abschnitt 2.3) gefordert. Bestätige beide Rechnungen durch Nachvollziehen der Formel im Code.

- [ ] **Step 7: Commit**

```bash
git add NixieClockUltra.ino display.ino
git commit -m "feat: Slot-Machine-Geschwindigkeit skalierbar machen (slotSpeedPct)"
```

---

### Task 6: Web-UI für Slot-Machine-Geschwindigkeit

**Files:**
- Modify: `web_server.ino` (HTML-Regler in bestehender Slot-Karte, `/api/status` erweitern, neuer Endpunkt, JS)

**Interfaces:**
- Consumes: globale `uint8_t slotSpeedPct` (Task 5).
- Produces: `POST /api/slotspeed` (JSON `{val: int}`); `doc["slotSpeed"]` in `GET /api/status`.

- [ ] **Step 1: Regler in bestehende Slot-Animation-Karte einfügen**

In `web_server.ino`, in der `🎰 Slot-Animation`-Karte (Zeile 89–105), zwischen dem `<select id="slotIval">`-Block (endet Zeile 100) und dem Trigger-Button-Block (beginnt Zeile 101) einfügen:

```html
  <div class="row">
    <label>Geschwindigkeit</label>
    <input type="range" id="slotSpeed" min="20" max="100" value="100" oninput="setSlotSpeed(this.value)">
    <span id="slotSpeedVal">100</span>%
  </div>
```

- [ ] **Step 2: `/api/status` um `slotSpeed` erweitern**

In der `/api/status`-Handler-Funktion (aktuell Zeile 396–415), nach `doc["slotIval"] = (int)slotInterval;` (Zeile 404) einfügen:

```cpp
    doc["slotSpeed"] = slotSpeedPct;
```

- [ ] **Step 3: `refreshClock()` um Regler-Initialisierung erweitern**

In `refreshClock()` (aktuell Zeile 172–193), nach der Zeile `if(d.slotIval!==undefined) document.getElementById('slotIval').value=d.slotIval;` (Zeile 177) einfügen:

```js
  if(d.slotSpeed!==undefined){document.getElementById('slotSpeed').value=d.slotSpeed;document.getElementById('slotSpeedVal').textContent=d.slotSpeed;}
```

- [ ] **Step 4: JS-Funktion `setSlotSpeed()` ergänzen**

Nach der bestehenden `setSlotInterval()`-Funktion (aktuell Zeile 223–226) einfügen:

```js
async function setSlotSpeed(v){
  document.getElementById('slotSpeedVal').textContent=v;
  await api('/api/slotspeed',{val:parseInt(v)});
}
```

- [ ] **Step 5: Server-Endpunkt ergänzen**

Nach dem bestehenden `/api/slotinterval`-Handler (nach Zeile 493, vor dem `// --- WiFi Status ---`-Kommentar in Zeile 495) einfügen:

```cpp
  server.on("/api/slotspeed", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      StaticJsonDocument<64> doc;
      if (!deserializeJson(doc, data, len)) {
        slotSpeedPct = (uint8_t)constrain((int)doc["val"], 20, 100);
        prefs.putUChar("slotSpeed", slotSpeedPct);
        req->send(200, "application/json", "{\"ok\":true}");
      } else {
        req->send(400, "application/json", "{\"ok\":false}");
      }
    }
  );
```

- [ ] **Step 6: Manueller Trace zur Verifikation**

`POST /api/slotspeed {"val": 15}` → `constrain(15, 20, 100) = 20` (unter Minimum geklemmt) → `slotSpeedPct=20`, persistiert. `GET /api/status` enthält danach `"slotSpeed":20`. Bestätige durch Lesen des Codes gegen dieses Beispiel.

- [ ] **Step 7: Commit**

```bash
git add web_server.ino
git commit -m "feat: Web-UI-Regler für Slot-Machine-Geschwindigkeit"
```

---

### Task 7: Manueller End-to-End-Test auf echter Hardware

**Files:** keine Code-Änderungen — reine Verifikation.

**Interfaces:** keine.

Dieser Task kann nur vom Nutzer auf der physischen Uhr ausgeführt werden (Flash-Zugriff auf ESP32-S3-Hardware ist von dieser Umgebung aus nicht möglich). Checkliste gemäß Spec Abschnitt 3 (Testplan):

- [ ] **Firmware kompilieren und flashen** (Arduino IDE, `USB CDC On Boot: Enabled` beibehalten — siehe Projekt-Historie zur USB-Falle).
- [ ] **Weicher Ziffernwechsel Sekundentakt:** Toggle "Bei Sekundentakt" in Web-UI aktivieren, `nightState`/Tageslicht sicherstellen (`NIGHT_NORMAL`). Jede Sekunde muss ein kurzes Abdunkeln/Aufhellen aller Röhren sichtbar sein (~100ms). Toggle deaktivieren → sofortiger Wechsel wie vor dieser Änderung.
- [ ] **Weicher Ziffernwechsel Zeit/Datum:** Toggle "Bei Zeit/Datum-Wechsel" aktivieren, Slot-Intervall auf "10 Sek" stellen, warten bis Slot-Animation endet und Datumsanzeige startet → 200ms-Fade beobachten; nach 5s Rückwechsel zur Zeit → ebenfalls 200ms-Fade.
- [ ] **Interaktion mit Nacht-Modus:** Während eines Sekundentakt-Fades in den Dimm-Zustand wechseln (LDR abdecken oder Zeitbereich triggern) → kein hängender Zwischenzustand, Duty landet korrekt auf Dimm-Helligkeit (kein "stecken bleiben" bei Fade-Minimalhelligkeit).
- [ ] **Slot-Speed:** Regler auf 20% stellen, Slot-Machine-Button drücken → deutlich langsameres Rollen, letzte Röhre stoppt nach ~7,5s. Regler zurück auf 100% → Verhalten wie vor dieser Änderung.
- [ ] **NVS-Persistenz:** Nach jedem obigen Test einen Reboot durchführen (Power-Cycle oder `ESP.restart()` via Neustart-Trigger) — alle vier neuen Einstellungen (`sfSecEn`, `sfDateEn`, `slotSpeed`, Regler-Position in der UI nach Reload) müssen erhalten bleiben.
- [ ] Nach erfolgreichem Test: Branch `feature/soft-digit-transitions` ist bereit für `finishing-a-development-branch`.

---

## Self-Review

**Spec-Abdeckung:** Abschnitt 1 (weicher Ziffernwechsel, Scope/Architektur/Timing/Nacht-Modus-Interaktion/Web-UI-NVS) → Task 1–4. Abschnitt 2 (Slot-Speed, Variable/Persistenz/Anwendung/Web-UI-NVS) → Task 5–6. Abschnitt 3 (Testplan) → Task 7. "Nicht Teil dieser Änderung" (Pro-Röhre-Crossfade, konfigurierbare Fade-Dauer/Minimalhelligkeit, Geschwindigkeiten >100%) wird von keinem Task versehentlich umgesetzt — geprüft.

**Platzhalter-Scan:** Keine TBD/TODO, jeder Code-Schritt enthält vollständigen Code, jede Verifikation ist entweder ein echter Testlauf (Task 1) oder ein konkret ausgerechneter manueller Trace mit Zahlenwerten (Task 2–6), kein "Tests wie oben schreiben"-Verweis.

**Typkonsistenz:** `setDisplayTimeSoft(uint8_t,uint8_t,uint8_t,uint16_t)` / `setDisplayDateSoft(uint16_t)` (Task 3 produziert, Task 3 selbst konsumiert in `NixieClockUltra.ino` — keine späteren Tasks rufen sie erneut auf). `startDigitFade(uint8_t[6], uint16_t)` / `updateDigitFade()` / `cancelDigitFade()` (Task 2 produziert, Task 3 konsumiert exakt diese Signaturen). `fadeDutyForStep(bool,uint8_t,uint8_t,uint8_t,uint8_t)` (Task 1 produziert, Task 2 konsumiert exakt diese Signatur). `slotSpeedPct`/`slotRollIntervalMs`/`slotStopMs[6]` (Task 5 produziert, Task 6 konsumiert nur `slotSpeedPct`, `display.ino` intern nutzt alle drei) — konsistent über alle Tasks geprüft.
