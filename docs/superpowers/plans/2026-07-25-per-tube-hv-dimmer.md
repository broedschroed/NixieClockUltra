# Pro-Röhre-HV-Dimmer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Beim weichen Ziffernwechsel soll nur die Röhre abblenden, deren Ziffer sich tatsächlich ändert (statt aller 6 gemeinsam), wenn die neue optionale Pro-Röhre-HV-Schalter-Hardware (6× TLP627) bestückt und per Compile-Switch aktiviert ist. Ohne aktivierten Switch bleibt das heutige Verhalten (ein gemeinsamer Schalter, alle dimmen zusammen) exakt erhalten.

**Architecture:** `hv_dimmer.ino` bekommt zwei API-Funktionen (`hvDimmerSetDutyAll()`, `hvDimmerSetDutyTube()`), die je nach `#ifdef HV_PER_TUBE_DIMMER` entweder 6 unabhängige LEDC-Kanäle oder (Fallback) den einen bestehenden `HV_SWITCH_PIN` ansteuern — im Fallback ignoriert `hvDimmerSetDutyTube()` den Röhrenindex und dimmt einfach global, wodurch das `#ifdef` komplett in dieser einen Datei isoliert bleibt. `digit_fade.ino` ermittelt beim Fade-Start per Bitmaske (`computeChangedMask()`, neue reine Funktion in `digit_fade_math.h`), welche Röhren betroffen sind, und steuert nur diese über `hvDimmerSetDutyTube()` an.

**Tech Stack:** Arduino-ESP32 Core 3.x (`ledcAttach`/`ledcWrite`), C++17 Host-Unit-Tests (g++, kein Arduino-Framework nötig) für die reine Logik in `digit_fade_math.h`.

## Global Constraints

- Ohne `HV_PER_TUBE_DIMMER` (Standard, auskommentiert) muss sich das Verhalten gegenüber dem aktuellen Stand nicht ändern — Rename-only, keine Logikänderung.
- Bit-Reihenfolge der Röhren-Maske: Bit 0 = Tube-Index 0 (Stundenzehner/HZ) … Bit 5 = Tube-Index 5 (Sekundeneiner/SE) — identisch zur bestehenden `displayDigits[6]`-Reihenfolge (`NixieClockUltra.ino:131`) und `digitPin[6][10]` in `nixie_driver.ino`.
- Neue GPIOs: `HV_TUBE_PIN_0..5 = 38, 47, 15, 16, 17, 18` für HZ, HE, MZ, ME, SZ, SE.
- Weicher Ziffernwechsel läuft künftig auch im `NIGHT_DIM`-Zustand (Ziel-Helligkeit `hvDimPct`-skaliert statt 100 %), weiterhin nicht in `NIGHT_DARK`.
- Kein Arduino-Compile-Test verfügbar (kein `arduino-cli` installiert) — Verifikation von `.ino`-Änderungen erfolgt per Code-Review und späterem Hardware-Test durch den Nutzer, nicht per automatisiertem Build in diesem Repo.

---

### Task 1: Pin-Definitionen und Compile-Switch

**Files:**
- Modify: `NixieClockUltra.ino:70-71`

**Interfaces:**
- Produces: `HV_PER_TUBE_DIMMER` (auskommentiertes Macro), `HV_TUBE_PIN_0`..`HV_TUBE_PIN_5` (Konstanten) — von Task 3 (`hv_dimmer.ino`) konsumiert.

- [ ] **Step 1: Defines einfügen**

In `NixieClockUltra.ino`, Zeile 70–71 ersetzen:

```cpp
// HV-Dimmer (TLP627 → Anodenspannung)
#define HV_SWITCH_PIN  7
```

durch:

```cpp
// HV-Dimmer (TLP627 → Anodenspannung)
#define HV_SWITCH_PIN  7   // Gemeinsamer Schalter (ohne HV_PER_TUBE_DIMMER)

// Bei bestücktem Pro-Röhre-HV-Schalter (6× TLP627 auf dem Nixie Display Board)
// einkommentieren. Ermöglicht, dass beim weichen Ziffernwechsel nur die Röhre
// abblendet, deren Ziffer sich tatsächlich ändert (statt aller 6 gemeinsam).
// #define HV_PER_TUBE_DIMMER
#define HV_TUBE_PIN_0  38   // Stundenzehner  (HZ)
#define HV_TUBE_PIN_1  47   // Stundeneiner   (HE)
#define HV_TUBE_PIN_2  15   // Minutenzehner  (MZ)
#define HV_TUBE_PIN_3  16   // Minuteneiner   (ME)
#define HV_TUBE_PIN_4  17   // Sekundenzehner (SZ)
#define HV_TUBE_PIN_5  18   // Sekundeneiner  (SE)
```

- [ ] **Step 2: Sichtprüfung**

Datei lesen und bestätigen, dass `HV_PER_TUBE_DIMMER` auskommentiert ist (Standard = altes Verhalten) und die 6 Pin-Werte exakt `38, 47, 15, 16, 17, 18` sind — keine Kollision mit bestehenden Defines in derselben Datei (`RTC_*`, `BTN_*`, `NEO_PIN`, `IR_RECV_PIN`, `I2C_*`, `HV_SWITCH_PIN`, `LDR_PIN` in `night_mode.ino`).

- [ ] **Step 3: Commit**

```bash
git add NixieClockUltra.ino
git commit -m "feat: Pin-Defines für optionalen Pro-Röhre-HV-Dimmer ergänzen"
```

---

### Task 2: `computeChangedMask()` — reine Bitmasken-Logik (TDD)

**Files:**
- Modify: `digit_fade_math.h`
- Test: `test/digit_fade_math_test.cpp`

**Interfaces:**
- Produces: `uint8_t computeChangedMask(const uint8_t oldDigits[6], const uint8_t newDigits[6])` — von Task 4 (`digit_fade.ino`/`display.ino`) konsumiert. Bit *i* gesetzt ⇔ `oldDigits[i] != newDigits[i]`.

- [ ] **Step 1: Failing Test schreiben**

In `test/digit_fade_math_test.cpp`, direkt vor `printf("digit_fade_math_test: alle Assertions OK\n");` einfügen:

```cpp
  // computeChangedMask: keine Änderung → mask 0
  {
    uint8_t a[6] = {1,2,3,4,5,6};
    uint8_t b[6] = {1,2,3,4,5,6};
    assert(computeChangedMask(a, b) == 0);
  }

  // computeChangedMask: nur Tube-Index 0 (HZ) geändert → Bit 0
  {
    uint8_t a[6] = {1,2,3,4,5,6};
    uint8_t b[6] = {9,2,3,4,5,6};
    assert(computeChangedMask(a, b) == 0b000001);
  }

  // computeChangedMask: nur Tube-Index 5 (SE) geändert → Bit 5
  {
    uint8_t a[6] = {1,2,3,4,5,6};
    uint8_t b[6] = {1,2,3,4,5,9};
    assert(computeChangedMask(a, b) == 0b100000);
  }

  // computeChangedMask: Tube-Index 2 und 3 geändert (MZ+ME) → Bits 2+3
  {
    uint8_t a[6] = {1,2,3,4,5,6};
    uint8_t b[6] = {1,2,9,9,5,6};
    assert(computeChangedMask(a, b) == 0b001100);
  }

  // computeChangedMask: alle 6 geändert → 0b111111
  {
    uint8_t a[6] = {0,0,0,0,0,0};
    uint8_t b[6] = {1,1,1,1,1,1};
    assert(computeChangedMask(a, b) == 0b111111);
  }

  printf("computeChangedMask: alle Assertions OK\n");
```

- [ ] **Step 2: Test ausführen, Fehlschlag bestätigen**

Run: `g++ -std=c++17 -Wall -o /tmp/digit_fade_math_test test/digit_fade_math_test.cpp && /tmp/digit_fade_math_test`
Expected: Compile-Fehler `'computeChangedMask' was not declared in this scope` (Funktion existiert noch nicht).

- [ ] **Step 3: Minimale Implementierung**

In `digit_fade_math.h`, am Ende der Datei (nach `fadeDutyForStep()`) einfügen:

```cpp

// Vergleicht alte gegen neue Ziffern und liefert eine Bitmaske der Röhren,
// deren Ziffer sich geändert hat. Bit i (i=0..5) entspricht Tube-Index i
// (0=HZ, 1=HE, 2=MZ, 3=ME, 4=SZ, 5=SE — dieselbe Reihenfolge wie
// displayDigits[6] und digitPin[6][10] in nixie_driver.ino).
inline uint8_t computeChangedMask(const uint8_t oldDigits[6], const uint8_t newDigits[6]) {
  uint8_t mask = 0;
  for (uint8_t i = 0; i < 6; i++) {
    if (oldDigits[i] != newDigits[i]) mask |= (uint8_t)(1u << i);
  }
  return mask;
}
```

- [ ] **Step 4: Test ausführen, Erfolg bestätigen**

Run: `g++ -std=c++17 -Wall -o /tmp/digit_fade_math_test test/digit_fade_math_test.cpp && /tmp/digit_fade_math_test`
Expected: `digit_fade_math_test: alle Assertions OK` gefolgt von `computeChangedMask: alle Assertions OK`, Exit-Code 0.

- [ ] **Step 5: Commit**

```bash
git add digit_fade_math.h test/digit_fade_math_test.cpp
git commit -m "feat: computeChangedMask() für Pro-Röhre-Fade-Bitmaske + Tests"
```

---

### Task 3: HV-Dimmer Hardware-Abstraktion

**Files:**
- Modify: `hv_dimmer.ino` (komplett ersetzen)
- Modify: `NixieClockUltra.ino:413-415`
- Modify: `tube_test.ino:17`, `tube_test.ino:43-45`
- Modify: `web_server.ino:746`

**Interfaces:**
- Consumes: `HV_SWITCH_PIN`, `HV_PWM_FREQ_HZ`, `HV_PER_TUBE_DIMMER`, `HV_TUBE_PIN_0..5` (Task 1)
- Produces: `void hvDimmerInit()` (Signatur unverändert), `void hvDimmerSetDutyAll(uint8_t duty)`, `void hvDimmerSetDutyTube(uint8_t tube, uint8_t duty)` — von Task 4 (`digit_fade.ino`) konsumiert. Ersetzt die bisherige `hvDimmerSetDuty(uint8_t duty)`.

- [ ] **Step 1: `hv_dimmer.ino` komplett ersetzen**

Gesamten Inhalt von `hv_dimmer.ino` durch folgenden ersetzen:

```cpp
// ═══════════════════════════════════════════════════════════
//  HARDWARE-DIMMUNG DER ANODENSPANNUNG (TLP627, LEDC-PWM)
//  Ohne HV_PER_TUBE_DIMMER: ein gemeinsamer Schalter für alle 6 Anoden.
//  Mit HV_PER_TUBE_DIMMER: 6 unabhängige Schalter, einer pro Röhre.
// ═══════════════════════════════════════════════════════════

#ifdef HV_PER_TUBE_DIMMER

static const uint8_t hvTubePin[6] = {
  HV_TUBE_PIN_0, HV_TUBE_PIN_1, HV_TUBE_PIN_2,
  HV_TUBE_PIN_3, HV_TUBE_PIN_4, HV_TUBE_PIN_5
};

void hvDimmerInit() {
  for (uint8_t i = 0; i < 6; i++) {
    ledcAttach(hvTubePin[i], HV_PWM_FREQ_HZ, 8);
    ledcWrite(hvTubePin[i], 255);   // volle Helligkeit (Anode dauerhaft an)
  }
}

void hvDimmerSetDutyAll(uint8_t duty0to255) {
  for (uint8_t i = 0; i < 6; i++) ledcWrite(hvTubePin[i], duty0to255);
}

void hvDimmerSetDutyTube(uint8_t tube, uint8_t duty0to255) {
  ledcWrite(hvTubePin[tube], duty0to255);
}

#else  // heutige Hardware: ein gemeinsamer Schalter

void hvDimmerInit() {
  ledcAttach(HV_SWITCH_PIN, HV_PWM_FREQ_HZ, 8);
  ledcWrite(HV_SWITCH_PIN, 255);   // volle Helligkeit (Anode dauerhaft an)
}

void hvDimmerSetDutyAll(uint8_t duty0to255) {
  ledcWrite(HV_SWITCH_PIN, duty0to255);
}

// Ohne Pro-Röhre-Hardware gibt es nur den einen gemeinsamen Schalter — der
// Röhrenindex wird ignoriert, jeder Aufruf dimmt alle Anoden gemeinsam
// (reproduziert exakt das bisherige Verhalten).
void hvDimmerSetDutyTube(uint8_t /*tube*/, uint8_t duty0to255) {
  ledcWrite(HV_SWITCH_PIN, duty0to255);
}

#endif
```

- [ ] **Step 2: Aufrufstellen umbenennen — `NixieClockUltra.ino`**

Zeile 413–415 ersetzen:

```cpp
      case NIGHT_DARK:   hvDimmerSetDuty(0);                        break;
      case NIGHT_DIM:    hvDimmerSetDuty(hvDimPct * 255 / 100);     break;
      case NIGHT_NORMAL: hvDimmerSetDuty(255);                      break;
```

durch:

```cpp
      case NIGHT_DARK:   hvDimmerSetDutyAll(0);                        break;
      case NIGHT_DIM:    hvDimmerSetDutyAll(hvDimPct * 255 / 100);     break;
      case NIGHT_NORMAL: hvDimmerSetDutyAll(255);                      break;
```

- [ ] **Step 3: Aufrufstellen umbenennen — `tube_test.ino`**

Zeile 17 ersetzen:
```cpp
  hvDimmerSetDuty(255);   // volle Helligkeit erzwingen, unabhängig von Nacht-Modus
```
durch:
```cpp
  hvDimmerSetDutyAll(255);   // volle Helligkeit erzwingen, unabhängig von Nacht-Modus
```

Zeile 43–45 ersetzen:
```cpp
    case NIGHT_DARK:   hvDimmerSetDuty(0);                    break;
    case NIGHT_DIM:    hvDimmerSetDuty(hvDimPct * 255 / 100); break;
    case NIGHT_NORMAL: hvDimmerSetDuty(255);                  break;
```
durch:
```cpp
    case NIGHT_DARK:   hvDimmerSetDutyAll(0);                    break;
    case NIGHT_DIM:    hvDimmerSetDutyAll(hvDimPct * 255 / 100); break;
    case NIGHT_NORMAL: hvDimmerSetDutyAll(255);                  break;
```

- [ ] **Step 4: Aufrufstelle umbenennen — `web_server.ino`**

Zeile 746 ersetzen:
```cpp
        if (nightState == NIGHT_DIM) hvDimmerSetDuty(hvDimPct * 255 / 100);
```
durch:
```cpp
        if (nightState == NIGHT_DIM) hvDimmerSetDutyAll(hvDimPct * 255 / 100);
```

- [ ] **Step 5: Grep-Check — keine Restverwendung der alten Funktion**

Run: `grep -rn "hvDimmerSetDuty(" *.ino`
Expected: kein Treffer mehr (nur noch `hvDimmerSetDutyAll(` und `hvDimmerSetDutyTube(` in der Codebasis, letzteres bislang nur in `hv_dimmer.ino` selbst definiert — Aufrufe folgen in Task 4).

- [ ] **Step 6: Commit**

```bash
git add hv_dimmer.ino NixieClockUltra.ino tube_test.ino web_server.ino
git commit -m "refactor: hvDimmerSetDuty durch hvDimmerSetDutyAll/-Tube ersetzen"
```

---

### Task 4: Pro-Röhre-Fade-Logik

**Files:**
- Modify: `digit_fade.ino` (komplett ersetzen)
- Modify: `display.ino:1-20`

**Interfaces:**
- Consumes: `computeChangedMask()` (Task 2), `hvDimmerSetDutyTube()`/`hvDimmerSetDutyAll()` (Task 3), globale `nightState`/`hvDimPct` (bestehend in `NixieClockUltra.ino`)
- Produces: `void startDigitFade(uint8_t newDigits[6], uint8_t changedMask, uint16_t fadeMs)` (Signaturänderung — neuer Parameter `changedMask`), `void updateDigitFade()` (unverändert), `void cancelDigitFade()` (unverändert) — einziger Aufrufer von `startDigitFade()` ist `display.ino`, in diesem Task mit angepasst.

- [ ] **Step 1: `digit_fade.ino` komplett ersetzen**

Gesamten Inhalt von `digit_fade.ino` durch folgenden ersetzen:

```cpp
// ═══════════════════════════════════════════════════════════
//  WEICHER ZIFFERNWECHSEL – HV-Dimmer-Crossfade (pro Röhre)
//  Non-blocking State-Machine, angetrieben von updateDigitFade()
//  in loop(). Nutzt hv_dimmer.ino (Duty) + nixie_driver.ino
//  (Ziffern schreiben). Reine Interpolations-/Masken-Mathematik
//  in digit_fade_math.h (host-testbar, siehe test/).
//
//  Ohne HV_PER_TUBE_DIMMER dimmt hvDimmerSetDutyTube() ohnehin den
//  einen gemeinsamen Schalter (siehe hv_dimmer.ino) — dieser Code
//  bleibt für beide Hardware-Varianten identisch.
// ═══════════════════════════════════════════════════════════

#include <string.h>
#include "digit_fade_math.h"

#define DIGIT_FADE_MIN_DUTY   13   // ~5% von 255
#define DIGIT_FADE_STEP_MS    5    // Schrittintervall

enum FadeDir { FADE_DOWN, FADE_UP };

static bool     fadeRunning    = false;
static FadeDir  fadeDir        = FADE_DOWN;
static uint8_t  fadeTargetDigits[6];
static uint8_t  fadeMask       = 0;     // Bitmaske der Röhren, die diesen Fade durchlaufen
static uint8_t  fadeMaxDuty    = 255;   // Ziel-Helligkeit oben (255 normal, hvDimPct-skaliert im Dimm-Modus)
static uint32_t fadeLastStepMs = 0;
static uint8_t  fadeStepsTotal = 1;
static uint8_t  fadeStepsDone  = 0;

// Setzt jede Röhre aus mask auf duty.
static void applyDutyToMask(uint8_t mask, uint8_t duty) {
  for (uint8_t tube = 0; tube < 6; tube++) {
    if (mask & (uint8_t)(1u << tube)) hvDimmerSetDutyTube(tube, duty);
  }
}

// Schließt einen laufenden Fade sofort ab: schreibt die Zielziffern und setzt
// die betroffenen Röhren auf ihre Ziel-Helligkeit. Wird intern verwendet, wenn
// ein neuer Fade angefordert wird während einer noch läuft.
static void fadeFinishImmediately() {
  if (!fadeRunning) return;
  nixieWrite(fadeTargetDigits);
  applyDutyToMask(fadeMask, fadeMaxDuty);
  fadeRunning = false;
}

// Schließt einen laufenden Fade sofort ab, OHNE die Helligkeit anzufassen
// (der Aufrufer setzt sie danach selbst, z.B. beim Nacht-Modus-Wechsel).
// No-op wenn kein Fade läuft.
void cancelDigitFade() {
  if (!fadeRunning) return;
  nixieWrite(fadeTargetDigits);
  fadeRunning = false;
}

// Startet einen neuen Fade zu newDigits über fadeMs (aufgeteilt in
// fadeMs/2 Abblenden + fadeMs/2 Aufblenden). changedMask markiert die Röhren,
// deren Ziffer sich ändert (siehe computeChangedMask() in digit_fade_math.h)
// — nur diese werden während des Fades angefasst, unveränderte Röhren bleiben
// auf ihrer aktuellen Helligkeit. fadeMs muss > 0 sein (Aufrufer in
// display.ino prüft das bereits vor dem Aufruf).
void startDigitFade(uint8_t newDigits[6], uint8_t changedMask, uint16_t fadeMs) {
  fadeFinishImmediately();
  memcpy(fadeTargetDigits, newDigits, 6);
  fadeMask       = changedMask;
  fadeMaxDuty    = (nightState == NIGHT_DIM) ? (uint8_t)(hvDimPct * 255 / 100) : 255;
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
                                  DIGIT_FADE_MIN_DUTY, fadeMaxDuty);
  applyDutyToMask(fadeMask, duty);

  if (fadeStepsDone >= fadeStepsTotal) {
    if (fadeDir == FADE_DOWN) {
      nixieWrite(fadeTargetDigits);   // Ziffern bei Minimalhelligkeit umschalten
      fadeDir       = FADE_UP;
      fadeStepsDone = 0;
    } else {
      fadeRunning = false;            // Fade komplett, Duty ist bereits fadeMaxDuty
    }
  }
}
```

- [ ] **Step 2: `display.ino` — `commitDigits()` anpassen**

Zeilen 1–20 (Datei-Header + `commitDigits()`) ersetzen:

```cpp
// ═══════════════════════════════════════════════════════════
//  DISPLAY-UPDATE (displayDigits befüllen)
// ═══════════════════════════════════════════════════════════
#include <string.h>

// Schreibt neue Zielziffern. Bei fadeMs>0 und nightState!=NIGHT_DARK läuft ein
// weicher HV-Dimmer-Crossfade (digit_fade.ino) nur für die Röhren, deren
// Ziffer sich ändert; in NIGHT_DARK ist die Anodenspannung ohnehin aus, dort
// wird sofort hart über nixieWrite() geschrieben. Kein Effekt, wenn sich
// nichts ändert.
static void commitDigits(uint8_t newDigits[6], uint16_t fadeMs) {
  // Maske VOR dem Überschreiben von displayDigits berechnen (vergleicht
  // alt gegen neu) — bestimmt zugleich, ob sich überhaupt etwas ändert.
  uint8_t changedMask = computeChangedMask(displayDigits, newDigits);
  if (changedMask == 0) return;
  memcpy(displayDigits, newDigits, 6);
  if (fadeMs > 0 && nightState != NIGHT_DARK) {
    startDigitFade(newDigits, changedMask, fadeMs);
  } else {
    nixieWrite(displayDigits);
  }
}
```

- [ ] **Step 3: `display.ino` — `digit_fade_math.h` einbinden**

Direkt nach `#include <string.h>` (jetzt Zeile 4) einfügen:

```cpp
#include "digit_fade_math.h"
```

- [ ] **Step 4: Grep-Check — keine alte `startDigitFade`-Signatur mehr**

Run: `grep -n "startDigitFade(" *.ino`
Expected: genau zwei Treffer — die Definition in `digit_fade.ino` (3 Parameter: `newDigits, changedMask, fadeMs`) und der Aufruf in `display.ino` (ebenfalls 3 Argumente).

- [ ] **Step 5: Host-Tests erneut laufen lassen (Regressionscheck)**

Run: `g++ -std=c++17 -Wall -o /tmp/digit_fade_math_test test/digit_fade_math_test.cpp && /tmp/digit_fade_math_test`
Expected: weiterhin `digit_fade_math_test: alle Assertions OK` + `computeChangedMask: alle Assertions OK` (dieser Task ändert `digit_fade_math.h` nicht, reiner Regressionscheck).

- [ ] **Step 6: Commit**

```bash
git add digit_fade.ino display.ino
git commit -m "feat: weicher Ziffernwechsel dimmt nur geänderte Röhren (Pro-Röhre-Fade)"
```

---

### Task 5: System-Dokumentation (`docs/system/`)

**Files:**
- Modify: `docs/system/hardware.md`
- Modify: `docs/system/firmware.md`
- Modify: `docs/system/SUMMARY.md`

**Interfaces:**
- Keine Code-Schnittstellen — reine Dokumentationsänderung, muss mit dem Stand nach Task 1–4 übereinstimmen.

- [ ] **Step 1: `docs/system/hardware.md` — PCB2-Intro ergänzen**

Zeilen 49–53 ersetzen:

```markdown
Das Display Board enthält die eigentliche Anzeigeelektronik. Die vier MCP23017 treiben
je 16 NPN-Transistoren, die die Nixie-Kathoden individuell auf GND schalten. Die Anoden
aller Röhren liegen gemeinsam an der über J4 eingespeisten Hochspannung (~170V), die
auf dem Logic Board per TLP627-Hardware-PWM gedimmt werden kann (siehe Nacht-Modus in
[firmware.md](firmware.md)).
```

durch:

```markdown
Das Display Board enthält die eigentliche Anzeigeelektronik. Die vier MCP23017 treiben
je 16 NPN-Transistoren, die die Nixie-Kathoden individuell auf GND schalten. Die Anoden
aller Röhren liegen gemeinsam an der über J4 eingespeisten Hochspannung (~170V), die
auf dem Logic Board per TLP627-Hardware-PWM gedimmt werden kann (siehe Nacht-Modus in
[firmware.md](firmware.md)).

**Optional:** Statt des einen gemeinsamen Schalters auf dem Logic Board kann jede Anode
über einen eigenen TLP627-Schalter auf dem Display Board unabhängig gedimmt werden
(6× TLP627 statt 1×, siehe Komponententabelle unten). Das ermöglicht einen weichen
Ziffernwechsel pro Röhre, bei dem unveränderte Ziffern nicht mitblinken. Aktiviert wird
das per `HV_PER_TUBE_DIMMER`-Compile-Switch in der Firmware
([firmware.md](firmware.md#wichtige-defines)) — ohne bestückte Zusatzschalter bleibt der
gemeinsame Schalter aktiv.
```

- [ ] **Step 2: `docs/system/hardware.md` — TLP627-Zeile in PCB2-Komponententabelle ergänzen**

Nach Zeile 69 (letzte Zeile der PCB2-Hauptkomponenten-Tabelle, `| J2 | HV (4-polig) | 1 | ... |`) folgende Zeile einfügen:

```markdown
| –        | TLP627 (DIP-4)       | 6 (optional) | Pro-Röhre-Anodenschalter — ersetzt bei Bestückung den gemeinsamen Logic-Board-Schalter für unabhängiges Dimmen jeder Röhre |
```

- [ ] **Step 3: `docs/system/hardware.md` — Pin-Belegungstabelle ergänzen**

Zeile 138 (`| 13   | BTN_SET   | Taster SET (INPUT_PULLUP, aktiv LOW)   |`) — direkt danach folgende 4 Zeilen einfügen:

```markdown
| 15   | HV_TUBE_PIN_2 | *(optional)* Pro-Röhre-TLP627 Minutenzehner (nur bei `HV_PER_TUBE_DIMMER`) |
| 16   | HV_TUBE_PIN_3 | *(optional)* Pro-Röhre-TLP627 Minuteneiner (nur bei `HV_PER_TUBE_DIMMER`) |
| 17   | HV_TUBE_PIN_4 | *(optional)* Pro-Röhre-TLP627 Sekundenzehner (nur bei `HV_PER_TUBE_DIMMER`) |
| 18   | HV_TUBE_PIN_5 | *(optional)* Pro-Röhre-TLP627 Sekundeneiner (nur bei `HV_PER_TUBE_DIMMER`) |
```

Danach, direkt nach Zeile 139 (`| 21   | NEO_DATA  | ...`), folgende 2 Zeilen einfügen:

```markdown
| 38   | HV_TUBE_PIN_0 | *(optional)* Pro-Röhre-TLP627 Stundenzehner (nur bei `HV_PER_TUBE_DIMMER`) |
| 47   | HV_TUBE_PIN_1 | *(optional)* Pro-Röhre-TLP627 Stundeneiner (nur bei `HV_PER_TUBE_DIMMER`) |
```

(Ergebnis: Tabelle bleibt nach GPIO-Nummer aufsteigend sortiert — 2,4,5,6,7,8,9,10,11,12,13,**15,16,17,18**,21,**38,47**,48.)

- [ ] **Step 4: `docs/system/firmware.md` — Modulübersicht**

Zeile 17 ersetzen:

```markdown
| `display.ino`        | 88     | `setDisplayTime()`, `setDisplayDate()`, `commitDigits()` (weicher Ziffernwechsel), Slot-Animation |
```

durch:

```markdown
| `display.ino`        | 89     | `setDisplayTime()`, `setDisplayDate()`, `commitDigits()` (weicher Ziffernwechsel, Bitmaske via `computeChangedMask()`), Slot-Animation |
```

Zeile 18 ersetzen:

```markdown
| `digit_fade.ino`     | 77     | `startDigitFade()`, `updateDigitFade()`, `cancelDigitFade()` — non-blocking Crossfade über HV-Dimmer-Duty |
```

durch:

```markdown
| `digit_fade.ino`     | 95     | `startDigitFade()`, `updateDigitFade()`, `cancelDigitFade()` — non-blocking Crossfade über HV-Dimmer-Duty, pro Röhre per Bitmaske |
```

Zeile 22 ersetzen:

```markdown
| `hv_dimmer.ino`      | 12     | `hvDimmerInit()`, `hvDimmerSetDuty()` — LEDC-Hardware-PWM für TLP627 auf Anodenspannung |
```

durch:

```markdown
| `hv_dimmer.ino`      | 47     | `hvDimmerInit()`, `hvDimmerSetDutyAll()`, `hvDimmerSetDutyTube()` — LEDC-Hardware-PWM für TLP627 auf Anodenspannung; bei `HV_PER_TUBE_DIMMER` 6 unabhängige Kanäle statt einem gemeinsamen |
```

- [ ] **Step 5: `docs/system/firmware.md` — Setup-Ablauf Schritt 4**

Zeile 40–41 ersetzen:

```markdown
4. `hvDimmerInit()` — LEDC-PWM auf `HV_SWITCH_PIN` (GPIO7) anlegen, Duty zunächst 255
   (volle Anodenspannung)
```

durch:

```markdown
4. `hvDimmerInit()` — LEDC-PWM anlegen, Duty zunächst 255 (volle Anodenspannung); ohne
   `HV_PER_TUBE_DIMMER` auf `HV_SWITCH_PIN` (GPIO7), mit aktiviertem Switch auf 6
   unabhängigen Kanälen (`HV_TUBE_PIN_0`–`HV_TUBE_PIN_5`)
```

- [ ] **Step 6: `docs/system/firmware.md` — Wichtige Defines**

Zeile 95–97 ersetzen:

```cpp
// HV-Dimmer (TLP627, LEDC-Hardware-PWM auf Anodenspannung)
#define HV_SWITCH_PIN   7         // GPIO → TLP627 → Anodenspannung
#define HV_PWM_FREQ_HZ  200       // Hz, LEDC 8-Bit-Auflösung (Duty 0–255)
```

durch:

```cpp
// HV-Dimmer (TLP627, LEDC-Hardware-PWM auf Anodenspannung) {#wichtige-defines}
#define HV_SWITCH_PIN   7         // GPIO → TLP627 → Anodenspannung (ohne HV_PER_TUBE_DIMMER)
#define HV_PWM_FREQ_HZ  200       // Hz, LEDC 8-Bit-Auflösung (Duty 0–255)

// Bei bestücktem Pro-Röhre-HV-Schalter (6× TLP627 auf dem Nixie Display Board):
// #define HV_PER_TUBE_DIMMER
#define HV_TUBE_PIN_0  38         // Stundenzehner (HZ)
#define HV_TUBE_PIN_1  47         // Stundeneiner  (HE)
#define HV_TUBE_PIN_2  15         // Minutenzehner (MZ)
#define HV_TUBE_PIN_3  16         // Minuteneiner  (ME)
#define HV_TUBE_PIN_4  17         // Sekundenzehner(SZ)
#define HV_TUBE_PIN_5  18         // Sekundeneiner (SE)
```

- [ ] **Step 7: `docs/system/firmware.md` — Nacht-Modus-Logik**

Zeile 183–195 ersetzen:

```markdown
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
```

durch:

```markdown
**HV-Anodendimmung** in `loop()`: Bei einem Wechsel von `nightState` (Guard
`nightState != prevNightState`) ruft `loop()` `hvDimmerSetDutyAll()` auf (`hv_dimmer.ino`)
— setzt alle 6 Röhren gemeinsam auf dieselbe Ziel-Helligkeit, unabhängig davon, ob
`HV_PER_TUBE_DIMMER` aktiv ist:

| `nightState`   | Duty (`hvDimmerSetDutyAll()`)   |
|----------------|----------------------------------|
| `NIGHT_NORMAL` | 255 (volle Anodenspannung)       |
| `NIGHT_DIM`    | `hvDimPct * 255 / 100` (2–60 %)  |
| `NIGHT_DARK`   | 0 (Anodenspannung aus)           |

Die Dimmung erfolgt per LEDC-Hardware-PWM (~200 Hz) direkt auf der Anodenspannung über
einen TLP627-Optokoppler — ohne `HV_PER_TUBE_DIMMER` einen gemeinsamen (`HV_SWITCH_PIN`,
GPIO7), mit aktiviertem Switch 6 unabhängige (`HV_TUBE_PIN_0`–`HV_TUBE_PIN_5`) — nicht mehr
per Software-PWM auf den Kathoden. Ein separater Blitzschutz für Sekundenwechsel ist nicht
mehr nötig, da die Kathoden-Ansteuerung unabhängig von der Anodendimmung läuft.
```

- [ ] **Step 8: `docs/system/firmware.md` — Weicher Ziffernwechsel**

Zeile 197–219 ersetzen:

```markdown
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
```

durch:

```markdown
## Weicher Ziffernwechsel (`digit_fade.ino`)

`commitDigits()` in `display.ino` ist die zentrale Stelle, über die alle Ziffernänderungen
laufen (`setDisplayTime()`, `setDisplayDate()` sowie die Soft-Varianten
`setDisplayTimeSoft()`/`setDisplayDateSoft()`). Sie berechnet per `computeChangedMask()`
(`digit_fade_math.h`) eine Bitmaske der Röhren, deren Ziffer sich ändert (Bit *i* = Tube-Index
*i*, 0=HZ…5=SE) — ist die Maske 0, ändert sich nichts und der Aufruf wird ignoriert. Sonst:

- `fadeMs == 0` oder `nightState == NIGHT_DARK` → sofortiger Hart-Wechsel über `nixieWrite()`
  (in NIGHT_DARK ist die Anodenspannung ohnehin aus, ein Fade wäre unsichtbar)
- `fadeMs > 0` und `nightState != NIGHT_DARK` (also auch in `NIGHT_DIM`) → `startDigitFade()`
  in `digit_fade.ino`, mit der berechneten Maske als Parameter

`startDigitFade()`/`updateDigitFade()` bilden eine non-blocking State-Machine (angetrieben aus
`loop()`), die den HV-Dimmer-Duty **nur für die Röhren in der Fade-Maske**
(`hvDimmerSetDutyTube()`) in `DIGIT_FADE_STEP_MS`-Schritten (5 ms) erst auf
`DIGIT_FADE_MIN_DUTY` (13, ≈5 %) abblendet, bei Minimalhelligkeit die Zielziffern schreibt
(`nixieWrite()`), und wieder auf die Ziel-Helligkeit (`fadeMaxDuty`) aufblendet. `fadeMaxDuty`
wird beim Fade-Start einmalig ermittelt: 255 bei `NIGHT_NORMAL`, `hvDimPct * 255 / 100` bei
`NIGHT_DIM` — der Fade wirkt im Dimm-Modus also konsistent gedimmt statt kurz auf 100 %
aufzublitzen. Röhren außerhalb der Maske werden während des gesamten Fades nicht angefasst —
mit aktiviertem `HV_PER_TUBE_DIMMER` (`hv_dimmer.ino`) bleiben sie sichtbar unverändert hell,
ohne die Hardware-Option dimmt `hvDimmerSetDutyTube()` ohnehin den einen gemeinsamen Schalter
(siehe [hv_dimmer.ino](#hv-anodendimmung-hv_dimmerino)), sodass sich am heutigen
Erscheinungsbild (alle 6 dimmen gemeinsam) nichts ändert.

Die Dauer wird über `fadeMs` gesteuert (je zur Hälfte Ab-/Aufblenden); aktuell fest verdrahtet
auf 400 ms, siehe `NixieClockUltra.ino` (`softFadeSecondEnabled ? 400 : 0` bzw.
`softFadeDateEnabled ? 400 : 0`). Die reine Interpolationsmathematik (`fadeDutyForStep()`) und
die Masken-Berechnung (`computeChangedMask()`) liegen in `digit_fade_math.h` und sind per
Host-Unit-Test (`test/digit_fade_math_test.cpp`) ohne Arduino-Framework testbar.

`cancelDigitFade()` schließt einen laufenden Fade sofort ab (Zielziffern schreiben, Duty
unangetastet) und wird vor `startSlotAnimation()` sowie beim Eintritt in den Edit-Modus
aufgerufen — Absicherung gegen eine Race zwischen laufendem Fade und neuer Display-Aktivität.
```

- [ ] **Step 9: `docs/system/firmware.md` — Röhrentest-Abschnitt**

Zeile 246 ersetzen:

```markdown
- `startTubeTest()`: bricht konkurrierende Display-Nutzer ab (`cancelDigitFade()`,
  `slotActive = false`, `dateShowActive = false`, `editState = EDIT_NONE`),
  erzwingt `hvDimmerSetDuty(255)` unabhängig vom Nacht-Modus, schreibt Ziffer 0
  sofort hart auf alle Röhren. Erneuter Aufruf während eines laufenden Tests
  setzt ihn einfach auf Ziffer 0 zurück (kein Fehlerfall).
```

durch:

```markdown
- `startTubeTest()`: bricht konkurrierende Display-Nutzer ab (`cancelDigitFade()`,
  `slotActive = false`, `dateShowActive = false`, `editState = EDIT_NONE`),
  erzwingt `hvDimmerSetDutyAll(255)` unabhängig vom Nacht-Modus, schreibt Ziffer 0
  sofort hart auf alle Röhren. Erneuter Aufruf während eines laufenden Tests
  setzt ihn einfach auf Ziffer 0 zurück (kein Fehlerfall).
```

- [ ] **Step 10: `docs/system/firmware.md` — HV-Anodendimmung Codeblock**

Zeile 309–325 ersetzen:

```markdown
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
```

durch:

```markdown
## HV-Anodendimmung (`hv_dimmer.ino`) {#hv-anodendimmung-hv_dimmerino}

Schaltet die Anodenspannung (~170 V) über einen oder sechs TLP627-Optokoppler per
LEDC-Hardware-PWM. Der `HV_PER_TUBE_DIMMER`-Switch entscheidet, welche der beiden Varianten
kompiliert wird (siehe [Wichtige Defines](#wichtige-defines)):

```cpp
#ifdef HV_PER_TUBE_DIMMER
// 6 unabhängige Kanäle, einer pro Röhre
void hvDimmerInit() { /* ledcAttach() je HV_TUBE_PIN_0..5, Duty 255 */ }
void hvDimmerSetDutyAll(uint8_t duty0to255)              { /* alle 6 Kanäle */ }
void hvDimmerSetDutyTube(uint8_t tube, uint8_t duty0to255) { /* nur dieser Kanal */ }

#else
// Ein gemeinsamer Kanal (heutige Hardware)
void hvDimmerInit() {
  ledcAttach(HV_SWITCH_PIN, HV_PWM_FREQ_HZ, 8);
  ledcWrite(HV_SWITCH_PIN, 255);   // volle Helligkeit (Anode dauerhaft an)
}
void hvDimmerSetDutyAll(uint8_t duty0to255) { ledcWrite(HV_SWITCH_PIN, duty0to255); }
// Röhrenindex wird ignoriert — es gibt nur den einen gemeinsamen Schalter.
void hvDimmerSetDutyTube(uint8_t /*tube*/, uint8_t duty0to255) { ledcWrite(HV_SWITCH_PIN, duty0to255); }
#endif
```

`hvDimmerSetDutyAll()` wird bei einem `nightState`-Wechsel in `loop()` aufgerufen, nicht pro
Loop-Durchlauf. `hvDimmerSetDutyTube()` wird ausschließlich aus der Fade-State-Machine in
`digit_fade.ino` heraus aufgerufen (siehe [Weicher Ziffernwechsel](#weicher-ziffernwechsel-digit_fadeino)) — kein zusätzlicher CPU-Overhead im Normalbetrieb.
```

- [ ] **Step 11: `docs/system/SUMMARY.md` aktualisieren**

Zeile 19–20 ersetzen:

```markdown
Inter-Board-Signaltabellen für J3↔J1 und J4↔J2. Vollständige ESP32-S3 Pin-Belegung
(11 GPIOs).
```

durch:

```markdown
Inter-Board-Signaltabellen für J3↔J1 und J4↔J2. Vollständige ESP32-S3 Pin-Belegung
(11 Pflicht-GPIOs + 6 optionale GPIOs für den Pro-Röhre-HV-Dimmer).
```

Zeile 28 ersetzen:

```markdown
Shadow-Register, Lookup-Tabelle), der HV-Anodendimmung (TLP627/LEDC) und des weichen
```

durch:

```markdown
Shadow-Register, Lookup-Tabelle), der HV-Anodendimmung (TLP627/LEDC, optional 6 unabhängige
Pro-Röhre-Kanäle über `HV_PER_TUBE_DIMMER`) und des weichen
```

- [ ] **Step 12: Sichtprüfung**

Alle drei Dateien lesen und bestätigen, dass Pin-Nummern (38, 47, 15, 16, 17, 18),
Funktionsnamen (`hvDimmerSetDutyAll`, `hvDimmerSetDutyTube`, `computeChangedMask`) und
Verhaltensbeschreibungen exakt mit Task 1–4 übereinstimmen.

- [ ] **Step 13: Commit**

```bash
git add docs/system/hardware.md docs/system/firmware.md docs/system/SUMMARY.md
git commit -m "docs: Systemdoku um optionalen Pro-Röhre-HV-Dimmer ergänzen"
```

---

### Task 6: Nutzer-Dokumentation & Website

**Files:**
- Modify: `docs/manual/nixie-clock-ultra-bedienungsanleitung.html:601-602`, `:856`
- Modify: `docs/website/aufbau.html:42`
- Modify: `docs/website/features.html:36`, `:80`
- Modify: `docs/website/js/i18n-de.js` (Keys `aufbau.logic.p3`, `features.anzeige.li4`, `features.technik.li3`)
- Modify: `docs/website/js/i18n-en.js` (dieselben Keys, englische Übersetzung)

**Interfaces:**
- Keine Code-Schnittstellen — reine Inhaltsänderung an bestehenden `data-i18n`-Keys, keine neuen Keys nötig.

- [ ] **Step 1: Bedienungsanleitung — Kapitel „Weicher Ziffernwechsel"**

In `docs/manual/nixie-clock-ultra-bedienungsanleitung.html`, Zeile 602, den Absatz ergänzen (Bauteilnamen bleiben hier bewusst außen vor, siehe bestehender Schreibstil):

Zeile 602 ersetzen:

```html
  <p>Statt hart umzuschalten, können die Röhren beim Ziffernwechsel sanft überblenden — kurz abdunkeln und mit der neuen Ziffer wieder aufhellen. Das lässt sich getrennt für zwei Situationen im Web-Interface unter <strong>Weicher Ziffernwechsel</strong> ein- und ausschalten: für den ganz normalen <strong>Sekundentakt</strong> und für den <strong>Wechsel zwischen Uhrzeit- und Datumsanzeige</strong>.</p>
```

durch:

```html
  <p>Statt hart umzuschalten, können die Röhren beim Ziffernwechsel sanft überblenden — kurz abdunkeln und mit der neuen Ziffer wieder aufhellen. Das lässt sich getrennt für zwei Situationen im Web-Interface unter <strong>Weicher Ziffernwechsel</strong> ein- und ausschalten: für den ganz normalen <strong>Sekundentakt</strong> und für den <strong>Wechsel zwischen Uhrzeit- und Datumsanzeige</strong>.</p>
  <p>Bei Uhren mit der optionalen Pro-Röhre-Dimmoption (siehe Kapitel „Systemdaten") blendet dabei nur die Röhre ab, deren Ziffer sich tatsächlich ändert — unveränderte Ziffern bleiben durchgehend hell. Ohne diese Nachrüstung dimmen weiterhin alle Röhren gemeinsam.</p>
```

- [ ] **Step 2: Bedienungsanleitung — Systemdaten-Tabelle**

Zeile 856 ersetzen:

```html
      <tr><td><strong>Anoden-Dimmung</strong></td><td>TLP627-Optokoppler, Hardware-PWM ≈200 Hz (LEDC), GPIO7</td></tr>
```

durch:

```html
      <tr><td><strong>Anoden-Dimmung</strong></td><td>TLP627-Optokoppler, Hardware-PWM ≈200 Hz (LEDC), GPIO7 (Standard); optional 6× TLP627 für unabhängiges Pro-Röhre-Dimmen, GPIO 15/16/17/18/38/47</td></tr>
```

- [ ] **Step 3: Website — `aufbau.html`**

In `docs/website/aufbau.html`, Zeile 42 (`<p data-i18n="aufbau.logic.p3">...</p>`) den `data-i18n`-Platzhaltertext (nur als Fallback/Referenz, die tatsächliche Anzeige kommt aus den i18n-Dateien) analog zu Step 4/5 anpassen — ersetzen:

```html
          <p data-i18n="aufbau.logic.p3">Neu in Rev 2.1: Lichtsensor-Anschluss <strong>J5</strong> (LDR, GPIO6) für den automatischen Nacht-Modus. Neu ergänzt: <strong>TLP627</strong>-Optokoppler (GPIO7) dimmt die Anodenspannung per Hardware-PWM (2–60 %) — aktuell als Erweiterung handverdrahtet, noch nicht mit eigener Referenz im Schaltplan.</p>
```

durch:

```html
          <p data-i18n="aufbau.logic.p3">Neu in Rev 2.1: Lichtsensor-Anschluss <strong>J5</strong> (LDR, GPIO6) für den automatischen Nacht-Modus. Neu ergänzt: <strong>TLP627</strong>-Optokoppler (GPIO7) dimmt die Anodenspannung per Hardware-PWM (2–60 %) — aktuell als Erweiterung handverdrahtet, noch nicht mit eigener Referenz im Schaltplan. Optional lässt sich dieser eine Schalter durch 6 separate TLP627 auf dem Nixie Display Board ersetzen, einen pro Röhre, für unabhängiges Dimmen jeder Ziffer beim weichen Ziffernwechsel.</p>
```

- [ ] **Step 4: `docs/website/js/i18n-de.js` aktualisieren**

Zeile 74 ersetzen:

```javascript
  "aufbau.logic.p3": "Neu in Rev 2.1: Lichtsensor-Anschluss <strong>J5</strong> (LDR, GPIO6) für den automatischen Nacht-Modus. Neu ergänzt: <strong>TLP627</strong>-Optokoppler (GPIO7) dimmt die Anodenspannung per Hardware-PWM (2&ndash;60&nbsp;%) &mdash; aktuell als Erweiterung handverdrahtet, noch nicht mit eigener Referenz im Schaltplan.",
```

durch:

```javascript
  "aufbau.logic.p3": "Neu in Rev 2.1: Lichtsensor-Anschluss <strong>J5</strong> (LDR, GPIO6) für den automatischen Nacht-Modus. Neu ergänzt: <strong>TLP627</strong>-Optokoppler (GPIO7) dimmt die Anodenspannung per Hardware-PWM (2&ndash;60&nbsp;%) &mdash; aktuell als Erweiterung handverdrahtet, noch nicht mit eigener Referenz im Schaltplan. Optional lässt sich dieser eine Schalter durch 6 separate TLP627 auf dem Nixie Display Board ersetzen, einen pro Röhre, für unabhängiges Dimmen jeder Ziffer beim weichen Ziffernwechsel.",
```

Zeile 36 ersetzen:

```javascript
  "features.anzeige.li4": "Optional weicher Ziffernwechsel (Crossfade) &mdash; einzeln zuschaltbar für Sekundentakt und für den Wechsel zwischen Uhrzeit und Datum",
```

durch:

```javascript
  "features.anzeige.li4": "Optional weicher Ziffernwechsel (Crossfade) &mdash; einzeln zuschaltbar für Sekundentakt und für den Wechsel zwischen Uhrzeit und Datum, bei Pro-Röhre-Dimmoption pro Ziffer statt für alle 6 Röhren gemeinsam",
```

Zeile 60 ersetzen:

```javascript
  "features.technik.li3": "Hochvolt-Versorgung: 5 V USB &rarr; DC-DC-Konverter &rarr; 10 V &rarr; HV-Modul &rarr; ~170 V &rarr; TLP627-Hardware-Dimmer &rarr; Nixie-Anoden",
```

durch:

```javascript
  "features.technik.li3": "Hochvolt-Versorgung: 5 V USB &rarr; DC-DC-Konverter &rarr; 10 V &rarr; HV-Modul &rarr; ~170 V &rarr; TLP627-Hardware-Dimmer (optional 6&times; f&uuml;r unabh&auml;ngiges Pro-R&ouml;hre-Dimmen) &rarr; Nixie-Anoden",
```

- [ ] **Step 5: `docs/website/js/i18n-en.js` aktualisieren**

Zeile 74 ersetzen:

```javascript
  "aufbau.logic.p3": "New in Rev 2.1: light-sensor connector <strong>J5</strong> (LDR, GPIO6) for automatic night mode. Newly added: <strong>TLP627</strong> optocoupler (GPIO7) dims the anode voltage via hardware PWM (2&ndash;60&nbsp;%) &mdash; currently hand-wired as an add-on, not yet given its own reference designator in the schematic.",
```

durch:

```javascript
  "aufbau.logic.p3": "New in Rev 2.1: light-sensor connector <strong>J5</strong> (LDR, GPIO6) for automatic night mode. Newly added: <strong>TLP627</strong> optocoupler (GPIO7) dims the anode voltage via hardware PWM (2&ndash;60&nbsp;%) &mdash; currently hand-wired as an add-on, not yet given its own reference designator in the schematic. This single switch can optionally be replaced by 6 separate TLP627 optocouplers on the Nixie Display Board, one per tube, for independently dimming each digit during the soft digit transition.",
```

Zeile 36 ersetzen:

```javascript
  "features.anzeige.li4": "Optional soft digit transition (crossfade) &mdash; independently toggleable for the seconds tick and for switching between time and date",
```

durch:

```javascript
  "features.anzeige.li4": "Optional soft digit transition (crossfade) &mdash; independently toggleable for the seconds tick and for switching between time and date, per-digit instead of all 6 tubes together when the per-tube dimmer option is fitted",
```

Zeile 60 ersetzen:

```javascript
  "features.technik.li3": "High-voltage supply: 5 V USB &rarr; DC-DC converter &rarr; 10 V &rarr; HV module &rarr; ~170 V &rarr; TLP627 hardware dimmer &rarr; Nixie anodes",
```

durch:

```javascript
  "features.technik.li3": "High-voltage supply: 5 V USB &rarr; DC-DC converter &rarr; 10 V &rarr; HV module &rarr; ~170 V &rarr; TLP627 hardware dimmer (optionally 6&times; for independent per-tube dimming) &rarr; Nixie anodes",
```

- [ ] **Step 6: Konsistenz-Check — i18n-Keys**

Run: `grep -c '"aufbau.logic.p3"\|"features.anzeige.li4"\|"features.technik.li3"' docs/website/js/i18n-de.js docs/website/js/i18n-en.js`
Expected: jede Datei meldet `3` (alle drei Keys weiterhin genau einmal vorhanden, kein Duplikat, keine verwaiste Zeile).

- [ ] **Step 7: Commit**

```bash
git add docs/manual/nixie-clock-ultra-bedienungsanleitung.html docs/website/aufbau.html docs/website/js/i18n-de.js docs/website/js/i18n-en.js
git commit -m "docs: Bedienungsanleitung und Website um optionalen Pro-Röhre-Dimmer ergänzen"
```

---

## Nach Abschluss aller Tasks

- Alle Host-Tests laufen lassen: `g++ -std=c++17 -Wall -o /tmp/digit_fade_math_test test/digit_fade_math_test.cpp && /tmp/digit_fade_math_test && g++ -std=c++17 -Wall -o /tmp/tube_test_math_test test/tube_test_math_test.cpp && /tmp/tube_test_math_test`
- Hardware-Test steht noch aus (kein Compile-Test in diesem Repo möglich) — siehe Test-Plan im Design-Spec (`docs/superpowers/specs/2026-07-25-per-tube-hv-dimmer-design.md`): einmal mit deaktiviertem `HV_PER_TUBE_DIMMER` auf echter Hardware durchklicken (Regressionscheck), danach mit aktiviertem Switch und bestückten 6 TLP627.
