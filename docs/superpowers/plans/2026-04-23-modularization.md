# Modularisierung NixieClockUltra Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Die monolithische `NixieClockUltra.ino` (1613 Zeilen) in 8 thematisch abgegrenzte `.ino`-Dateien aufteilen, ohne eine einzige Zeile Logik zu ändern.

**Architecture:** Mehrere `.ino`-Dateien im selben Sketch-Ordner. Arduino IDE fügt sie alphabetisch hinter der Haupt-Datei zusammen – alle Globals bleiben sichtbar, keine `extern`-Deklarationen nötig. Reines Move-Refactoring: Code wird verschoben, nicht verändert.

**Tech Stack:** Arduino IDE (ESP32-S3), C++, kein Build-Tool vorhanden – Compile-Test via Arduino IDE: Sketch → Verify (Ctrl+R / Cmd+R). Erwartete Ausgabe: `Compilation complete.` (ohne Fehler/Warnungen).

---

## Übersicht Dateistruktur (Zielbild)

| Datei | Verantwortlichkeit | Zeilen aus Original |
|---|---|---|
| `NixieClockUltra.ino` | Includes, Defines, Globals, setup(), loop() | 1–227, 278–284, 1197–1613 |
| `mux_timer.ino` | Nixie-Multiplex-ISR | 229–276 |
| `rtc.ino` | RTC-Hilfsfunktionen | 314–328 |
| `display.ino` | Display-Update + Slot-Animation | 330–376 |
| `neo_animation.ino` | NeoPixel-Animation | 378–461 |
| `buttons.ino` | Taster-Entprellung + Edit-Modus + Helligkeit + PowerSave | 286–312, 463–559 |
| `ir_remote.ino` | IR-Empfang + Aktionsausführung | 561–661 |
| `web_server.ino` | WEB_PAGE PROGMEM + setupWifi() + setupWebServer() | 663–1196 |

**Wichtig:** Button-Struct (`struct Button`, Z. 215–227) und Button-Instanzen (`btnSet`, `btnUp`, `btnDown`, `btnLight`, Z. 281–284) bleiben in `NixieClockUltra.ino`. Alle IR-Globals (IrAction-Enum, `irrecv`, `irCodes[]` etc., Z. 178–213) bleiben ebenfalls in `NixieClockUltra.ino`.

---

## Task 1: `mux_timer.ino` – Multiplex-ISR

**Files:**
- Create: `NixieClockUltra/mux_timer.ino`
- Modify: `NixieClockUltra/NixieClockUltra.ino` (Zeilen 229–276 löschen)

- [ ] **Step 1: Neue Datei `mux_timer.ino` erstellen**

Inhalt exakt:

```cpp
// ═══════════════════════════════════════════════════════════
//  TIMER-ISR – MULTIPLEX
// ═══════════════════════════════════════════════════════════
void IRAM_ATTR onMuxTimer() {
  portENTER_CRITICAL_ISR(&timerMux);

  if (inBlank) {
    // --- BLANK-Phase beendet: Röhre aktivieren ---
    inBlank = false;

    uint8_t digit = displayDigits[muxIndex];

    if (digit > 9) {
      // Blink-Modus: Röhre komplett aus lassen (keine Anode, keine Kathode)
      // Nächsten Alarm trotzdem setzen
      timerAlarm(muxTimer, MUX_DIGIT_US, false, 0);
      portEXIT_CRITICAL_ISR(&timerMux);
      return;
    }

    // Anode einschalten
    digitalWrite(ANODE_PIN[muxIndex], HIGH);
    // Kathode einschalten
    digitalWrite(CATHODE_PIN[digit], HIGH);

    // Nächsten Alarm: Anzeigezeit
    timerAlarm(muxTimer, MUX_DIGIT_US, false, 0);

  } else {
    // --- DIGIT-Phase beendet: Alles aus (Anti-Ghosting) ---
    // Anode aus
    digitalWrite(ANODE_PIN[muxIndex], LOW);
    // Kathode aus (nur wenn digit gültig)
    uint8_t digit = displayDigits[muxIndex];
    if (digit <= 9) {
      digitalWrite(CATHODE_PIN[digit], LOW);
    }

    // Nächste Röhre
    muxIndex = (muxIndex + 1) % 6;

    inBlank = true;
    // Nächsten Alarm: Blanking-Zeit
    timerAlarm(muxTimer, MUX_BLANK_US, false, 0);
  }

  portEXIT_CRITICAL_ISR(&timerMux);
}
```

- [ ] **Step 2: Zeilen 229–276 aus `NixieClockUltra.ino` löschen**

Das sind der Abschnitts-Header `// ═══ TIMER-ISR – MULTIPLEX ═══` (2 Zeilen) und die gesamte `onMuxTimer()`-Funktion. Nach dem Löschen folgt auf den IR-Globals-Block direkt der Taster-Bereich.

- [ ] **Step 3: Compile-Test**

Arduino IDE öffnen → `NixieClockUltra.ino` laden → **Sketch → Verify/Compile** (Ctrl+R).  
Erwartete Ausgabe: `Compilation complete.` – kein Fehler.

- [ ] **Step 4: Commit**

```bash
git add NixieClockUltra/mux_timer.ino NixieClockUltra/NixieClockUltra.ino
git commit -m "refactor: extract onMuxTimer ISR into mux_timer.ino"
```

---

## Task 2: `rtc.ino` – RTC-Hilfsfunktionen

**Files:**
- Create: `NixieClockUltra/rtc.ino`
- Modify: `NixieClockUltra/NixieClockUltra.ino` (Zeilen 314–328 löschen)

- [ ] **Step 1: Neue Datei `rtc.ino` erstellen**

```cpp
// ═══════════════════════════════════════════════════════════
//  RTC HILFSFUNKTIONEN
// ═══════════════════════════════════════════════════════════
void readRTC() {
  if (!Rtc.IsDateTimeValid()) return;
  RtcDateTime now = Rtc.GetDateTime();
  curHour = now.Hour();
  curMin  = now.Minute();
  curSec  = now.Second();
}

void writeRTC() {
  RtcDateTime dt(2024, 1, 1, curHour, curMin, curSec);
  Rtc.SetDateTime(dt);
}
```

- [ ] **Step 2: Zeilen 314–328 aus `NixieClockUltra.ino` löschen**

Der Abschnitts-Header `// ═══ RTC HILFSFUNKTIONEN ═══` (2 Zeilen) plus `readRTC()` und `writeRTC()`.

- [ ] **Step 3: Compile-Test**

Arduino IDE → **Sketch → Verify/Compile** (Ctrl+R).  
Erwartete Ausgabe: `Compilation complete.`

- [ ] **Step 4: Commit**

```bash
git add NixieClockUltra/rtc.ino NixieClockUltra/NixieClockUltra.ino
git commit -m "refactor: extract readRTC/writeRTC into rtc.ino"
```

---

## Task 3: `display.ino` – Display-Update und Slot-Animation

**Files:**
- Create: `NixieClockUltra/display.ino`
- Modify: `NixieClockUltra/NixieClockUltra.ino` (Zeilen 330–376 löschen)

- [ ] **Step 1: Neue Datei `display.ino` erstellen**

```cpp
// ═══════════════════════════════════════════════════════════
//  DISPLAY-UPDATE (aus displayDigits befüllen)
// ═══════════════════════════════════════════════════════════
void setDisplayTime(uint8_t h, uint8_t m, uint8_t s) {
  portENTER_CRITICAL(&timerMux);
  displayDigits[0] = h / 10;
  displayDigits[1] = h % 10;
  displayDigits[2] = m / 10;
  displayDigits[3] = m % 10;
  displayDigits[4] = s / 10;
  displayDigits[5] = s % 10;
  portEXIT_CRITICAL(&timerMux);
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
}
```

- [ ] **Step 2: Zeilen 330–376 aus `NixieClockUltra.ino` löschen**

Zwei Abschnitts-Header plus `setDisplayTime()`, `startSlotAnimation()` und `updateSlotAnimation()`.

- [ ] **Step 3: Compile-Test**

Arduino IDE → **Sketch → Verify/Compile** (Ctrl+R).  
Erwartete Ausgabe: `Compilation complete.`

- [ ] **Step 4: Commit**

```bash
git add NixieClockUltra/display.ino NixieClockUltra/NixieClockUltra.ino
git commit -m "refactor: extract display + slot animation into display.ino"
```

---

## Task 4: `neo_animation.ino` – NeoPixel-Animation

**Files:**
- Create: `NixieClockUltra/neo_animation.ino`
- Modify: `NixieClockUltra/NixieClockUltra.ino` (Zeilen 378–461 löschen)

- [ ] **Step 1: Neue Datei `neo_animation.ino` erstellen**

```cpp
// ═══════════════════════════════════════════════════════════
//  NEOPIXEL ANIMATIONEN
// ═══════════════════════════════════════════════════════════

// Hintergrund: Pixel 0–5 (unter den Röhren)
// Doppelpunkte: Pixel 6–9

void updateNeoPixel() {
  unsigned long now = millis();
  if (now - lastNeoUpdate < 20) return;
  lastNeoUpdate = now;

  uint8_t effectBright = powerSaveActive ? (neoBright / 4) : neoBright;
  strip.setBrightness(effectBright);

  switch (animMode) {

    case ANIM_RAINBOW: {
      // Farbverlauf über alle Hintergrund-Pixel
      for (int i = 0; i < 6; i++) {
        uint8_t hue = neoHue + i * 40;
        strip.setPixelColor(i, strip.ColorHSV(hue * 256, neoSat, 255));
      }
      neoHue++;
      // Doppelpunkte: synchron blinkend zum Sekundentakt
      bool colonOn = (curSec % 2 == 0);
      uint32_t colonColor = colonOn
        ? strip.ColorHSV((neoHue + 128) * 256, 200, 255)
        : strip.Color(0, 0, 0);
      for (int i = 6; i < 10; i++) strip.setPixelColor(i, colonColor);
      break;
    }

    case ANIM_STATIC: {
      // Statische Farbe (warmweiß)
      uint32_t warm = strip.Color(255, 180, 80);
      for (int i = 0; i < 6; i++) strip.setPixelColor(i, warm);
      bool colonOn = (curSec % 2 == 0);
      uint32_t colonColor = colonOn ? strip.Color(255, 100, 0) : strip.Color(0, 0, 0);
      for (int i = 6; i < 10; i++) strip.setPixelColor(i, colonColor);
      break;
    }

    case ANIM_PULSE: {
      // Puls-Effekt (sinusförmig)
      float phase = (millis() % 2000) / 2000.0f;
      uint8_t val = (uint8_t)(127.5f + 127.5f * sinf(phase * 2 * M_PI));
      uint32_t col = strip.ColorHSV(neoHue * 256, neoSat, val);
      for (int i = 0; i < 6; i++) strip.setPixelColor(i, col);
      neoHue++;
      bool colonOn = (curSec % 2 == 0);
      for (int i = 6; i < 10; i++)
        strip.setPixelColor(i, colonOn ? col : strip.Color(0,0,0));
      break;
    }

    case ANIM_SLOTS: {
      // Schnelles Farbwechseln während Slot-Animation
      if (slotActive) {
        uint32_t col = strip.ColorHSV((millis() / 5) % 65536, 255, 255);
        for (int i = 0; i < 10; i++) strip.setPixelColor(i, col);
      } else {
        // Fallback auf Rainbow
        for (int i = 0; i < 6; i++) {
          uint8_t hue = neoHue + i * 40;
          strip.setPixelColor(i, strip.ColorHSV(hue * 256, neoSat, 255));
        }
        neoHue++;
        bool colonOn = (curSec % 2 == 0);
        uint32_t colonColor = colonOn
          ? strip.ColorHSV((neoHue + 128) * 256, 200, 255)
          : strip.Color(0, 0, 0);
        for (int i = 6; i < 10; i++) strip.setPixelColor(i, colonColor);
      }
      break;
    }

    default: break;
  }

  irrecv.pause();
  strip.show();
  irrecv.resume();
}
```

- [ ] **Step 2: Zeilen 378–461 aus `NixieClockUltra.ino` löschen**

Abschnitts-Header `// ═══ NEOPIXEL ANIMATIONEN ═══` plus die gesamte `updateNeoPixel()`-Funktion inklusive der abschließenden `irrecv.pause()` / `strip.show()` / `irrecv.resume()`-Zeilen.

- [ ] **Step 3: Compile-Test**

Arduino IDE → **Sketch → Verify/Compile** (Ctrl+R).  
Erwartete Ausgabe: `Compilation complete.`

- [ ] **Step 4: Commit**

```bash
git add NixieClockUltra/neo_animation.ino NixieClockUltra/NixieClockUltra.ino
git commit -m "refactor: extract updateNeoPixel into neo_animation.ino"
```

---

## Task 5: `buttons.ino` – Taster-Entprellung und UI-Logik

**Files:**
- Create: `NixieClockUltra/buttons.ino`
- Modify: `NixieClockUltra/NixieClockUltra.ino` (zwei Blöcke löschen: Z. 278–312 ohne Z. 281–284, und Z. 463–559)

**Wichtig:** `Button btnSet`, `btnUp`, `btnDown`, `btnLight` (Zeilen 281–284) bleiben als globale Variablen in `NixieClockUltra.ino`. Nur die Sektion `// ═══ TASTER-ENTPRELLUNG ═══` (Kommentar-Header) und die Funktion `updateButton()` (Zeilen 278–312 abzüglich 281–284) wandern raus.

- [ ] **Step 1: Neue Datei `buttons.ino` erstellen**

```cpp
// ═══════════════════════════════════════════════════════════
//  TASTER-ENTPRELLUNG
// ═══════════════════════════════════════════════════════════
void updateButton(Button &b) {
  b.pressed = false;
  b.held    = false;
  bool state = digitalRead(b.pin);

  if (state == LOW && b.lastState == HIGH) {
    // Flanke fallend
    b.pressTime  = millis();
    b.lastRepeat = millis();
  }

  if (state == LOW) {
    unsigned long held = millis() - b.pressTime;
    if (held >= b.debounceMs) {
      if (b.lastState == HIGH) {
        b.pressed = true;  // Erstes Drücken
        lastInteractionMs = millis();
      }
      if (millis() - b.lastRepeat >= b.repeatMs) {
        b.held = true;
        b.lastRepeat = millis();
        lastInteractionMs = millis();
      }
    }
  }
  b.lastState = state;
}

// ═══════════════════════════════════════════════════════════
//  EINSTELLMODUS (FSM)
// ═══════════════════════════════════════════════════════════
void handleEditMode() {
  // SET drücken wechselt den Edit-State
  if (btnSet.pressed) {
    if (editState == EDIT_NONE) {
      editState    = EDIT_HOUR;
      editEnterTime = millis();
    } else if (editState == EDIT_HOUR) {
      editState = EDIT_MIN;
      editEnterTime = millis();
    } else if (editState == EDIT_MIN) {
      editState = EDIT_SEC;
      editEnterTime = millis();
    } else {
      // Speichern & beenden
      writeRTC();
      editState = EDIT_NONE;
    }
  }

  // Timeout
  if (editState != EDIT_NONE && (millis() - editEnterTime > EDIT_TIMEOUT_MS)) {
    writeRTC();
    editState = EDIT_NONE;
  }

  if (editState == EDIT_NONE) return;

  // UP / DOWN (mit Auto-Repeat)
  int delta = 0;
  if (btnUp.pressed   || btnUp.held)   delta = +1;
  if (btnDown.pressed || btnDown.held) delta = -1;

  if (delta != 0) {
    if (editState == EDIT_HOUR) {
      curHour = (curHour + 24 + delta) % 24;
    } else if (editState == EDIT_MIN) {
      curMin = (curMin + 60 + delta) % 60;
    } else if (editState == EDIT_SEC) {
      curSec = (curSec + 60 + delta) % 60;
    }
    editEnterTime = millis();
  }

  // Blinken der aktiven Stelle (250ms)
  bool blinkOn = ((millis() / 250) % 2 == 0);

  uint8_t dH = curHour, dM = curMin, dS = curSec;

  // Aktive Stelle ausblenden (Blink-Effekt)
  if (!blinkOn) {
    if      (editState == EDIT_HOUR) { dH = 99; }  // 99 = keine Anzeige
    else if (editState == EDIT_MIN)  { dM = 99; }
    else if (editState == EDIT_SEC)  { dS = 99; }
  }

  // Sonderfall 99: Blanke Röhren
  portENTER_CRITICAL(&timerMux);
  displayDigits[0] = (dH == 99) ? 10 : dH / 10;  // 10 = keine Kathode aktiv
  displayDigits[1] = (dH == 99) ? 10 : dH % 10;
  displayDigits[2] = (dM == 99) ? 10 : dM / 10;
  displayDigits[3] = (dM == 99) ? 10 : dM % 10;
  displayDigits[4] = (dS == 99) ? 10 : dS / 10;
  displayDigits[5] = (dS == 99) ? 10 : dS % 10;
  portEXIT_CRITICAL(&timerMux);
}

// ═══════════════════════════════════════════════════════════
//  HELLIGKEITSSTEUERUNG
// ═══════════════════════════════════════════════════════════
void handleBrightness() {
  if (btnLight.pressed) {
    brightLevel = (brightLevel + 1) % 4;
    neoBright   = BRIGHTNESS_LEVELS[brightLevel];
    // Speichern
    prefs.putUChar("bright", brightLevel);
    prefs.putUChar("neoBright", neoBright);
  }
}

// ═══════════════════════════════════════════════════════════
//  POWER-SAVE
// ═══════════════════════════════════════════════════════════
void handlePowerSave() {
  if (!powerSaveEnabled) {
    if (powerSaveActive) powerSaveActive = false;
    return;
  }
  unsigned long idleSec = (millis() - lastInteractionMs) / 1000;
  if (idleSec >= POWER_SAVE_TIMEOUT_S && !powerSaveActive) {
    powerSaveActive = true;
  } else if (idleSec < POWER_SAVE_TIMEOUT_S && powerSaveActive) {
    powerSaveActive = false;
  }
}
```

- [ ] **Step 2: Aus `NixieClockUltra.ino` löschen**

**Block A** (Zeilen 278–280 und 285–312): Den Abschnitts-Header `// ═══ TASTER-ENTPRELLUNG ═══` (2 Zeilen) sowie `void updateButton(...)` (ab Zeile 286). Die 4 Button-Instanzzeilen 281–284 (`Button btnSet = {...}` usw.) **bleiben stehen**.

**Block B** (Zeilen 463–559): Den Abschnitts-Header `// ═══ EINSTELLMODUS ═══` und alle drei Funktionen `handleEditMode()`, `handleBrightness()`, `handlePowerSave()` inklusive ihrer Abschnitts-Header.

- [ ] **Step 3: Compile-Test**

Arduino IDE → **Sketch → Verify/Compile** (Ctrl+R).  
Erwartete Ausgabe: `Compilation complete.`

- [ ] **Step 4: Commit**

```bash
git add NixieClockUltra/buttons.ino NixieClockUltra/NixieClockUltra.ino
git commit -m "refactor: extract button handling + UI logic into buttons.ino"
```

---

## Task 6: `ir_remote.ino` – IR-Fernbedienung

**Files:**
- Create: `NixieClockUltra/ir_remote.ino`
- Modify: `NixieClockUltra/NixieClockUltra.ino` (Zeilen 561–661 löschen)

**Wichtig:** Die IR-Globals (IrAction-Enum, `IR_ACTION_KEYS[]`, `IR_ACTION_LABELS[]`, `IRrecv irrecv`, `decode_results irResults`, `irCodes[]`, `irLearnTarget`, `irLearnStartMs`, `IR_LEARN_TIMEOUT_MS`, `irMux`, `powerSaveEnabled`) bleiben in `NixieClockUltra.ino` (Zeilen 178–213). Nur die drei Funktionen wandern in `ir_remote.ino`.

- [ ] **Step 1: Neue Datei `ir_remote.ino` erstellen**

```cpp
// ═══════════════════════════════════════════════════════════
//  IR-AKTIONEN
// ═══════════════════════════════════════════════════════════
void executeAction(IrAction action) {
  switch (action) {
    case IR_ACTION_SET:
      if (editState == EDIT_NONE) {
        editState = EDIT_HOUR;
        editEnterTime = millis();
      } else if (editState == EDIT_HOUR) {
        editState = EDIT_MIN;
        editEnterTime = millis();
      } else if (editState == EDIT_MIN) {
        editState = EDIT_SEC;
        editEnterTime = millis();
      } else {
        writeRTC();
        editState = EDIT_NONE;
      }
      break;

    case IR_ACTION_UP:
      if (editState == EDIT_HOUR) { curHour = (curHour + 1) % 24; editEnterTime = millis(); }
      else if (editState == EDIT_MIN) { curMin = (curMin + 1) % 60; editEnterTime = millis(); }
      else if (editState == EDIT_SEC) { curSec = (curSec + 1) % 60; editEnterTime = millis(); }
      break;

    case IR_ACTION_DOWN:
      if (editState == EDIT_HOUR) { curHour = (curHour + 23) % 24; editEnterTime = millis(); }
      else if (editState == EDIT_MIN) { curMin = (curMin + 59) % 60; editEnterTime = millis(); }
      else if (editState == EDIT_SEC) { curSec = (curSec + 59) % 60; editEnterTime = millis(); }
      break;

    case IR_ACTION_BRIGHTNESS:
      brightLevel = (brightLevel + 1) % 4;
      neoBright   = BRIGHTNESS_LEVELS[brightLevel];
      prefs.putUChar("bright", brightLevel);
      prefs.putUChar("neoBright", neoBright);
      break;

    case IR_ACTION_ANIM_NEXT:
      animMode = (AnimMode)((int(animMode) + 1) % ANIM_COUNT);
      prefs.putUChar("animMode", (uint8_t)animMode);
      break;

    case IR_ACTION_SLOT:
      startSlotAnimation(curHour, curMin, curSec);
      break;

    case IR_ACTION_POWER_SAVE_TOGGLE:
      powerSaveEnabled = !powerSaveEnabled;
      prefs.putBool("psEnabled", powerSaveEnabled);
      if (!powerSaveEnabled) powerSaveActive = false;
      break;

    default: break;
  }
}

void dispatchIRAction(uint64_t code) {
  for (int i = 0; i < IR_ACTION_COUNT; i++) {
    portENTER_CRITICAL(&irMux);
    bool match = (irCodes[i] != 0 && irCodes[i] == code);
    portEXIT_CRITICAL(&irMux);
    if (match) {
      executeAction((IrAction)i);
      lastInteractionMs = millis();
      return;
    }
  }
}

void handleIR() {
  // Lernmodus-Timeout
  if (irLearnTarget != IR_LEARN_NONE &&
      millis() - irLearnStartMs > IR_LEARN_TIMEOUT_MS) {
    irLearnTarget = IR_LEARN_NONE;
  }

  if (!irrecv.decode(&irResults)) return;

  uint64_t code = irResults.value;
  irrecv.resume();

  // Repeat-Codes ignorieren (NEC sendet 0xFFFFFFFF als Repeat)
  if (code == 0xFFFFFFFFULL || code == 0xFFFFFFFFFFFFFFFFULL) return;

  portENTER_CRITICAL(&irMux);
  if (irLearnTarget != IR_LEARN_NONE) {
    int target = (int)irLearnTarget;
    irCodes[target] = code;
    irLearnTarget = IR_LEARN_NONE;
    portEXIT_CRITICAL(&irMux);
    prefs.putULong64(IR_ACTION_KEYS[target], code);
    Serial.printf("[IR] Gelernt: %s = 0x%016llX\n",
      IR_ACTION_LABELS[target], (unsigned long long)code);
  } else {
    portEXIT_CRITICAL(&irMux);
    dispatchIRAction(code);
  }
}
```

- [ ] **Step 2: Zeilen 561–661 aus `NixieClockUltra.ino` löschen**

Abschnitts-Header `// ═══ IR-AKTIONEN ═══` plus `executeAction()`, `dispatchIRAction()` und `handleIR()`.

- [ ] **Step 3: Compile-Test**

Arduino IDE → **Sketch → Verify/Compile** (Ctrl+R).  
Erwartete Ausgabe: `Compilation complete.`

- [ ] **Step 4: Commit**

```bash
git add NixieClockUltra/ir_remote.ino NixieClockUltra/NixieClockUltra.ino
git commit -m "refactor: extract IR remote functions into ir_remote.ino"
```

---

## Task 7: `web_server.ino` – Web-Interface

**Files:**
- Create: `NixieClockUltra/web_server.ino`
- Modify: `NixieClockUltra/NixieClockUltra.ino` (Zeilen 663–1196 löschen)

Die zu extrahierenden Zeilen umfassen:
- `// ═══ WEB-INTERFACE ═══` Abschnitts-Header (Z. 663–665)
- `const char WEB_PAGE[] PROGMEM = R"rawliteral(...)rawliteral;` (Z. 667–933) – das gesamte HTML/CSS/JS
- Langer Dokumentationskommentar über WiFi-Verhalten (Z. 935–937 Abschnitts-Header + Z. 938 Kommentar-Block bis ca. Z. 964)
- `void setupWifi()` (Z. 938–964 nach Kommentar)
- `void setupWebServer()` (Z. 965–1196)

- [ ] **Step 1: Neue Datei `web_server.ino` erstellen**

Inhalt: Kopiere exakt die Zeilen 663–1196 aus `NixieClockUltra.ino` in die neue Datei. Das sind der WEB_PAGE-PROGMEM-String, `setupWifi()` und `setupWebServer()` mit allen API-Endpunkten.

Vorgehen: In Arduino IDE oder einem Texteditor die Zeilen 663–1196 aus `NixieClockUltra.ino` ausschneiden (Cut) und in die neue Datei `web_server.ino` einfügen (Paste). Dabei muss die erste Zeile der neuen Datei `// ═══════════...  WEB-INTERFACE ═══` sein und die letzte Zeile die schließende `}` von `setupWebServer()`.

Zur Orientierung: Die Zeile 1197 in der Original-Datei ist `// ═══════════...  SETUP ═══` – das ist der erste Inhalt, der in `NixieClockUltra.ino` verbleibt.

- [ ] **Step 2: Zeilen 663–1196 aus `NixieClockUltra.ino` löschen**

Nach dem Löschen folgt in `NixieClockUltra.ino` direkt nach den IR-Remote-Zeilen der `// ═══ SETUP ═══`-Header (ehemals Z. 1197).

- [ ] **Step 3: Compile-Test**

Arduino IDE → **Sketch → Verify/Compile** (Ctrl+R).  
Erwartete Ausgabe: `Compilation complete.`

- [ ] **Step 4: Commit**

```bash
git add NixieClockUltra/web_server.ino NixieClockUltra/NixieClockUltra.ino
git commit -m "refactor: extract web page + wifi + web server into web_server.ino"
```

---

## Task 8: Abschluss-Cleanup `NixieClockUltra.ino`

**Files:**
- Modify: `NixieClockUltra/NixieClockUltra.ino`

Nach allen Extraktionen enthält `NixieClockUltra.ino` noch:
- Header-Kommentar (Z. 1–26)
- Includes + Pin-Defines + Konstanten (Z. 27–100)
- Objekte + Globale Variablen (Z. 102–177)
- IR-Globals + `struct Button` + Button-Instanzen (Z. 178–284)
- Eventuell verbliebene Leerzeilen und Section-Header-Reste zwischen Z. 285 und Z. 313
- `setup()` + `loop()` (ehemalige Z. 1200–1356)
- Hardware-Hinweis-Kommentar (ehemalige Z. 1358–1613)

- [ ] **Step 1: Übrig gebliebene Leerzeilen und isolierte Section-Header bereinigen**

Überprüfe die Bereiche zwischen den Button-Instanzen und `setup()`:
- Doppelte Leerzeilen auf eine reduzieren
- Einzelne stehen gebliebene `// ═══ ... ═══` Abschnitts-Header-Paare ohne zugehörigen Code entfernen (falls vorhanden)

- [ ] **Step 2: Compile-Test (final)**

Arduino IDE → **Sketch → Verify/Compile** (Ctrl+R).  
Erwartete Ausgabe: `Compilation complete.`

Überprüfe zusätzlich in Arduino IDE, dass alle 8 Tabs erscheinen:
`NixieClockUltra | buttons | display | ir_remote | mux_timer | neo_animation | rtc | web_server`

- [ ] **Step 3: Finaler Commit**

```bash
git add NixieClockUltra/NixieClockUltra.ino
git commit -m "refactor: final cleanup of NixieClockUltra.ino after modularization"
```
