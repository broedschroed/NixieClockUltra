# MCP23017 Direct Drive Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Nixie-Röhrenansteuerung von Multiplexing-ISR auf direkte MCP23017-Ansteuerung via I2C umstellen, ohne andere Funktionen (NeoPixel, RTC, Web, IR) zu verändern.

**Architecture:** Vier MCP23017 Port-Expander (0x20–0x23) an I2C (SDA=GPIO8, SCL=GPIO9) schalten die Kathoden-Transistoren aller 6 Nixie-Röhren direkt. Shadow-Register verhindern unnötige I2C-Schreibvorgänge. Der Hardware-Timer-ISR entfällt vollständig.

**Tech Stack:** Arduino/ESP32-S3, Wire.h (kein Extra-Library), MCP23017 Register-Direktzugriff

---

## Datei-Übersicht

| Datei | Aktion |
|---|---|
| `nixie_driver.ino` | NEU — MCP23017-Init + nixieWrite() |
| `mux_timer.ino` | LÖSCHEN |
| `NixieClockUltra.ino` | ÄNDERN — Pins, Globals, setup() |
| `display.ino` | ÄNDERN — nixieWrite() aufrufen |
| `buttons.ino` | ÄNDERN — timerMux aus handleEditMode() entfernen |
| `rtc.ino` | unverändert |
| `neo_animation.ino` | unverändert |
| `ir_remote.ino` | unverändert |
| `web_server.ino` | unverändert |

---

## Task 1: nixie_driver.ino erstellen

**Files:**
- Create: `NixieClockUltra/nixie_driver.ino`

- [ ] **Schritt 1: Datei erstellen**

Neue Datei `nixie_driver.ino` mit folgendem Inhalt anlegen:

```cpp
// ═══════════════════════════════════════════════════════════
//  NIXIE DIRECT DRIVE – MCP23017 via I2C
//  4× MCP23017 an I2C-Bus (SDA=GPIO8, SCL=GPIO9)
//  Adressen: 0x20, 0x21, 0x22, 0x23
// ═══════════════════════════════════════════════════════════

#include <Wire.h>

#define MCP_BASE_ADDR  0x20
#define MCP_IODIRA     0x00
#define MCP_IODIRB     0x01
#define MCP_OLATA      0x14

// Shadow-Register: aktueller Ausgangszustand der 4 MCPs
// Bit 0–7 = GPA0–7, Bit 8–15 = GPB0–7
static uint16_t mcpState[4] = {0, 0, 0, 0};

// Zuordnung (tube, digit_value) → MCP-Index und Bit (0–15)
struct DigitPin { uint8_t mcp; uint8_t bit; };

static const DigitPin digitPin[6][10] = {
  // Index: [tube][digit_value]  digit_value 0=Ziffer0, 1=Ziffer1, ..., 9=Ziffer9

  // Tube 0: Stundenzehner (HZ) – MCP 0x20, Bits 0–9
  { {0, 9}, {0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {0, 7}, {0, 8} },

  // Tube 1: Stundeneiner  (HE) – MCP 0x20 Bits 10–15, MCP 0x21 Bits 0–3
  { {1, 3}, {0,10}, {0,11}, {0,12}, {0,13}, {0,14}, {0,15}, {1, 0}, {1, 1}, {1, 2} },

  // Tube 2: Minutenzehner (MZ) – MCP 0x21, Bits 4–13
  { {1,13}, {1, 4}, {1, 5}, {1, 6}, {1, 7}, {1, 8}, {1, 9}, {1,10}, {1,11}, {1,12} },

  // Tube 3: Minuteneiner  (ME) – MCP 0x22, Bits 0–9
  { {2, 9}, {2, 0}, {2, 1}, {2, 2}, {2, 3}, {2, 4}, {2, 5}, {2, 6}, {2, 7}, {2, 8} },

  // Tube 4: Sekundenzehner(SZ) – MCP 0x22 Bits 10–15, MCP 0x23 Bits 0–3
  { {3, 3}, {2,10}, {2,11}, {2,12}, {2,13}, {2,14}, {2,15}, {3, 0}, {3, 1}, {3, 2} },

  // Tube 5: Sekundeneiner (SE) – MCP 0x23, Bits 4–13
  { {3,13}, {3, 4}, {3, 5}, {3, 6}, {3, 7}, {3, 8}, {3, 9}, {3,10}, {3,11}, {3,12} },
};

static void mcpWriteAB(uint8_t addr, uint16_t value) {
  Wire.beginTransmission(addr);
  Wire.write(MCP_OLATA);
  Wire.write((uint8_t)(value & 0xFF));         // GPA (Low-Byte)
  Wire.write((uint8_t)((value >> 8) & 0xFF));  // GPB (High-Byte)
  Wire.endTransmission();
}

void nixieInit() {
  Wire.begin(8, 9);
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t addr = MCP_BASE_ADDR + i;
    Wire.beginTransmission(addr);
    Wire.write(MCP_IODIRA);
    Wire.write(0x00);  // GPA: alle Pins als Output
    Wire.endTransmission();
    Wire.beginTransmission(addr);
    Wire.write(MCP_IODIRB);
    Wire.write(0x00);  // GPB: alle Pins als Output
    Wire.endTransmission();
    mcpState[i] = 0;
    mcpWriteAB(addr, 0x0000);
  }
  Serial.println("[Nixie] MCP23017 initialisiert.");
}

// digits[6]: Werte 0–9 = Ziffer anzeigen, 10 = Röhre blank
void nixieWrite(uint8_t digits[6]) {
  uint16_t newState[4] = {0, 0, 0, 0};
  for (uint8_t tube = 0; tube < 6; tube++) {
    uint8_t d = digits[tube];
    if (d <= 9) {
      const DigitPin &dp = digitPin[tube][d];
      newState[dp.mcp] |= (1u << dp.bit);
    }
  }
  for (uint8_t i = 0; i < 4; i++) {
    if (newState[i] != mcpState[i]) {
      mcpWriteAB(MCP_BASE_ADDR + i, newState[i]);
      mcpState[i] = newState[i];
    }
  }
}
```

- [ ] **Schritt 2: Kompilieren (noch ohne andere Änderungen)**

Arduino IDE: Sketch → Verifizieren (Strg+R).  
Erwartetes Ergebnis: Compiler-Fehler wegen `nixieInit()` / `nixieWrite()` noch nicht aus main aufgerufen — das ist OK für diesen Schritt, solange keine Syntaxfehler in der neuen Datei gemeldet werden.

---

## Task 2: NixieClockUltra.ino anpassen

**Files:**
- Modify: `NixieClockUltra/NixieClockUltra.ino`

- [ ] **Schritt 1: `#include "driver/gpio.h"` entfernen**

```cpp
// ENTFERNEN:
#include "driver/gpio.h"        // gpio_set_level() – IRAM_ATTR, sicher im ISR
```

- [ ] **Schritt 2: CATHODE_PIN und ANODE_PIN Arrays entfernen**

```cpp
// ENTFERNEN (Zeilen ~53–59):
//const uint8_t DRAM_ATTR CATHODE_PIN[10] = {47, 17, 18, 38, 39, 40, 41, 42, 45, 46};
const uint8_t DRAM_ATTR CATHODE_PIN[10] = {17, 47, 46, 45, 42, 41, 40, 39, 38, 18};

const uint8_t DRAM_ATTR ANODE_PIN[6]    = {16, 15, 14, 13, 12, 11};
//                                         HZ  HE  MZ  ME  SZ  SE
```

- [ ] **Schritt 3: Taster-Pins auf neue GPIOs ändern**

```cpp
// ALT:
#define BTN_SET      10
#define BTN_UP        9
#define BTN_DOWN      8
#define BTN_LIGHT     7

// NEU:
#define BTN_SET      13
#define BTN_UP       14
#define BTN_DOWN     15
#define BTN_LIGHT    16
```

- [ ] **Schritt 4: Multiplex-Timing-Defines entfernen**

```cpp
// ENTFERNEN:
#define MUX_DIGIT_US       3800
#define MUX_BLANK_US        300
#define MUX_BLANK_TICKS      11
```

- [ ] **Schritt 5: volatile-Qualifier und nicht mehr benötigte Globals bereinigen**

```cpp
// ALT:
volatile uint8_t displayDigits[6] = {0, 0, 0, 0, 0, 0};
volatile uint8_t fadeBrightness[6] = {255, 255, 255, 255, 255, 255};
volatile uint8_t muxIndex = 0;
volatile bool    inBlank  = false;

// NEU (nur displayDigits bleibt, ohne volatile, fadeBrightness/muxIndex/inBlank entfernen):
uint8_t displayDigits[6] = {0, 0, 0, 0, 0, 0};
```

- [ ] **Schritt 6: Timer-Globals entfernen**

```cpp
// ENTFERNEN:
hw_timer_t *muxTimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
```

- [ ] **Schritt 7: Forward-Deklaration für onMuxTimer() entfernen**

```cpp
// ENTFERNEN:
void IRAM_ATTR onMuxTimer();
```

- [ ] **Schritt 8: Kathoden/Anoden-Pin-Setup in setup() entfernen**

```cpp
// ENTFERNEN aus setup():
  // --- Pins: Kathoden ---
  for (int i = 0; i < 10; i++) {
    pinMode(CATHODE_PIN[i], OUTPUT);
    digitalWrite(CATHODE_PIN[i], LOW);
  }
  // --- Pins: Anoden ---
  for (int i = 0; i < 6; i++) {
    pinMode(ANODE_PIN[i], OUTPUT);
    digitalWrite(ANODE_PIN[i], LOW);
  }
```

- [ ] **Schritt 9: Timer-Setup in setup() durch nixieInit() ersetzen**

```cpp
// ALT:
  // --- Multiplex-Timer (Timer 0, 1 MHz Basis) ---
  muxTimer = timerBegin(1000000);
  timerAttachInterrupt(muxTimer, &onMuxTimer);
  timerAlarm(muxTimer, MUX_BLANK_US, true, 0);

// NEU:
  // --- Nixie Direct Drive via MCP23017 ---
  nixieInit();
```

- [ ] **Schritt 10: Kompilieren**

Arduino IDE: Strg+R.  
Erwartetes Ergebnis: Keine Fehler mehr wegen fehlender CATHODE_PIN/ANODE_PIN/timerMux.  
Mögliche verbleibende Fehler: `timerMux` noch in display.ino und buttons.ino referenziert — das wird in Task 3 und 4 behoben.

---

## Task 3: display.ino anpassen

**Files:**
- Modify: `NixieClockUltra/display.ino`

- [ ] **Schritt 1: setDisplayTime() — timerMux entfernen, nixieWrite() aufrufen**

```cpp
// ALT:
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

// NEU:
void setDisplayTime(uint8_t h, uint8_t m, uint8_t s) {
  displayDigits[0] = h / 10;
  displayDigits[1] = h % 10;
  displayDigits[2] = m / 10;
  displayDigits[3] = m % 10;
  displayDigits[4] = s / 10;
  displayDigits[5] = s % 10;
  nixieWrite(displayDigits);
}
```

- [ ] **Schritt 2: updateSlotAnimation() — nixieWrite() am Ende ergänzen**

```cpp
// ALT:
void updateSlotAnimation() {
  if (!slotActive) return;
  unsigned long elapsed = millis() - slotStartMs;
  bool allDone = true;
  for (int i = 0; i < 6; i++) {
    unsigned long stopTime = 600 + i * 180;
    if (elapsed < stopTime) {
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

// NEU (nixieWrite am Ende ergänzen):
void updateSlotAnimation() {
  if (!slotActive) return;
  unsigned long elapsed = millis() - slotStartMs;
  bool allDone = true;
  for (int i = 0; i < 6; i++) {
    unsigned long stopTime = 600 + i * 180;
    if (elapsed < stopTime) {
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

- [ ] **Schritt 3: Kompilieren**

Arduino IDE: Strg+R.  
Erwartetes Ergebnis: Keine Fehler in display.ino.

---

## Task 4: buttons.ino anpassen

**Files:**
- Modify: `NixieClockUltra/buttons.ino`

- [ ] **Schritt 1: handleEditMode() — portCRITICAL entfernen, nixieWrite() aufrufen**

```cpp
// ALT (letzter Block in handleEditMode()):
  portENTER_CRITICAL(&timerMux);
  displayDigits[0] = (dH == 99) ? 10 : dH / 10;
  displayDigits[1] = (dH == 99) ? 10 : dH % 10;
  displayDigits[2] = (dM == 99) ? 10 : dM / 10;
  displayDigits[3] = (dM == 99) ? 10 : dM % 10;
  displayDigits[4] = (dS == 99) ? 10 : dS / 10;
  displayDigits[5] = (dS == 99) ? 10 : dS % 10;
  portEXIT_CRITICAL(&timerMux);

// NEU:
  displayDigits[0] = (dH == 99) ? 10 : dH / 10;
  displayDigits[1] = (dH == 99) ? 10 : dH % 10;
  displayDigits[2] = (dM == 99) ? 10 : dM / 10;
  displayDigits[3] = (dM == 99) ? 10 : dM % 10;
  displayDigits[4] = (dS == 99) ? 10 : dS / 10;
  displayDigits[5] = (dS == 99) ? 10 : dS % 10;
  nixieWrite(displayDigits);
```

- [ ] **Schritt 2: Kompilieren**

Arduino IDE: Strg+R.  
Erwartetes Ergebnis: Erfolgreich — keine Fehler mehr.

---

## Task 5: mux_timer.ino löschen

**Files:**
- Delete: `NixieClockUltra/mux_timer.ino`

- [ ] **Schritt 1: Datei löschen**

```bash
rm NixieClockUltra/mux_timer.ino
```

- [ ] **Schritt 2: Kompilieren**

Arduino IDE: Strg+R.  
Erwartetes Ergebnis: Saubere Kompilierung ohne Warnungen oder Fehler.

- [ ] **Schritt 3: Commit**

```bash
git add -A
git commit -m "feat: MCP23017 direct drive, kein Multiplexing mehr"
```

---

## Task 6: Upload und Funktionstest

- [ ] **Schritt 1: Firmware hochladen**

Arduino IDE: Hochladen (Strg+U).  
Board: ESP32S3 Dev Module, Upload Speed: 921600.

- [ ] **Schritt 2: Serial Monitor öffnen (115200 Baud)**

Erwartete Ausgabe beim Booten:
```
[NixieClock] Booting...
[WiFi] AP gestartet: NixieClock  IP: 192.168.4.1
[Nixie] MCP23017 initialisiert.
[IR] Empfänger gestartet auf Pin 48
[NixieClock] Bereit.
```

Falls stattdessen `[Nixie] MCP23017 initialisiert.` fehlt oder I2C-Fehler erscheinen: I2C-Adressen (0x20–0x23) und SDA/SCL-Verdrahtung (GPIO 8/9) prüfen.

- [ ] **Schritt 3: Visuelle Grundfunktion prüfen**

Uhr sollte aktuelle Zeit anzeigen (oder 00:00:00 bei frischer RTC).  
Alle 6 Röhren sollten die korrekte Ziffer zeigen, keine Phantomziffern (Ghosting) sichtbar.

- [ ] **Schritt 4: Einstellmodus testen**

SET-Taster (GPIO 13) drücken → Stunden blinken.  
UP/DOWN (GPIO 14/15) → Wert ändert sich.  
SET nochmal → Minuten blinken.  
SET nochmal → Sekunden blinken.  
SET nochmal → Zeit gespeichert, Normalbetrieb.

- [ ] **Schritt 5: Slot-Animation testen**

Browser: http://192.168.4.1 → "Slot-Machine!" klicken.  
Erwartetes Ergebnis: Alle 6 Röhren rollen durch Ziffern, stoppen nacheinander bei aktueller Zeit.

- [ ] **Schritt 6: Abschluss-Commit**

```bash
git add -A
git commit -m "test: MCP23017 direct drive validiert"
```
