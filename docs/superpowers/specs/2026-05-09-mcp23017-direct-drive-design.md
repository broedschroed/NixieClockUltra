# Design: MCP23017 Direct Drive (Nixie ohne Multiplexing)

**Datum:** 2026-05-09  
**Projekt:** NixieClockUltra  
**Status:** Genehmigt

---

## Ausgangssituation

Die bisherige Firmware steuert 6 Nixie-Röhren per Multiplexing über einen Hardware-Timer-ISR. Alle Röhren teilen 10 gemeinsame Kathoden-Pins am ESP32 — das führt zu unlösbarem Ghosting. Der Hardware-Umbau ersetzt dieses Konzept durch direkte Ansteuerung: Jede Röhre bekommt eigene Kathoden-Transistoren (SMBTA42), die von 4 MCP23017 Port-Expandern geschaltet werden. Die Anoden sind permanent mit HV verbunden.

---

## Hardware

### I2C
- SDA: GPIO 8  
- SCL: GPIO 9  
- Adressen: 0x20, 0x21, 0x22, 0x23

### Taster (neu)
- SET: GPIO 13  
- UP:  GPIO 14  
- DOWN: GPIO 15  
- LIGHT: GPIO 16

### MCP23017 Pin-Belegung

Jeder MCP23017-Ausgang schaltet den Basis-Eingang eines SMBTA42, der die jeweilige Nixie-Kathode auf GND zieht (LOW-aktiv).  
Digit-Reihenfolge im User-Spec: erst 1–8 (oder 1–6/1–4), dann 9, dann 0.

| MCP   | Port | Bit  | Tube | Digit |
|-------|------|------|------|-------|
| 0x20  | GPA  | 0–7  | HZ   | 1–8   |
| 0x20  | GPB  | 0    | HZ   | 9     |
| 0x20  | GPB  | 1    | HZ   | 0     |
| 0x20  | GPB  | 2–7  | HE   | 1–6   |
| 0x21  | GPA  | 0–2  | HE   | 7–9   |
| 0x21  | GPA  | 3    | HE   | 0     |
| 0x21  | GPA  | 4–7  | MZ   | 1–4   |
| 0x21  | GPB  | 0–4  | MZ   | 5–9   |
| 0x21  | GPB  | 5    | MZ   | 0     |
| 0x22  | GPA  | 0–7  | ME   | 1–8   |
| 0x22  | GPB  | 0    | ME   | 9     |
| 0x22  | GPB  | 1    | ME   | 0     |
| 0x22  | GPB  | 2–7  | SZ   | 1–6   |
| 0x23  | GPA  | 0–2  | SZ   | 7–9   |
| 0x23  | GPA  | 3    | SZ   | 0     |
| 0x23  | GPA  | 4–7  | SE   | 1–4   |
| 0x23  | GPB  | 0–4  | SE   | 5–9   |
| 0x23  | GPB  | 5    | SE   | 0     |

Nicht belegte Bits (MCP 0x21 GPB6–7, MCP 0x23 GPB6–7): dauerhaft LOW (Output, 0).

---

## Software-Architektur

### Dateien

| Datei | Aktion | Inhalt |
|---|---|---|
| `mux_timer.ino` | **löschen** | ISR, Hardware-Timer — entfällt komplett |
| `nixie_driver.ino` | **neu** | MCP23017-Init, Shadow-Register, nixieWrite() |
| `NixieClockUltra.ino` | **anpassen** | Pins, Globals, setup() |
| `display.ino` | **anpassen** | nixieWrite() statt displayDigits[]+timerMux |
| `buttons.ino` | unverändert | — |
| `rtc.ino` | unverändert | — |
| `neo_animation.ino` | unverändert | — |
| `ir_remote.ino` | unverändert | — |
| `web_server.ino` | unverändert | — |

### nixie_driver.ino

```
nixieInit()
  - Wire.begin(8, 9)
  - Für jede MCP-Adresse: IODIRA=0x00, IODIRB=0x00 (alle Pins Output)
  - mcpState[4] = {0,0,0,0}
  - Alle Outputs LOW schreiben

nixieWrite(uint8_t digits[6])
  - Neuen mcpState aus digitPin-Lookup berechnen:
    Für jeden Tube i (0–5):
      Tube-Maske löschen (alle Bits dieses Tubes in betroffenen MCPs auf 0)
      Wenn digits[i] <= 9: Bit für (tube, digit) setzen
      (digits[i] == 10 → Röhre blank)
  - Für jeden MCP 0–3: wenn neuer Wert != mcpState[j], OLATA+OLATB schreiben
  - mcpState aktualisieren
```

**Lookup-Tabelle:**  
`DigitPin digitPin[6][10]` mit `{uint8_t mcp, uint8_t bit}` — kompilierzeit-konstant im Flash.

**Tube-Masken** (welche 16-Bit-Felder pro Tube zu löschen sind):

| Tube | MCP-Index | Maske (16 Bit) |
|------|-----------|----------------|
| HZ   | 0         | 0x03FF         |
| HE   | 0, 1      | 0xFC00, 0x000F |
| MZ   | 1         | 0x3FF0         |
| ME   | 2         | 0x03FF         |
| SZ   | 2, 3      | 0xFC00, 0x000F |
| SE   | 3         | 0x3FF0         |

### Änderungen NixieClockUltra.ino

**Entfernt:**
- `CATHODE_PIN[]`, `ANODE_PIN[]`
- `DRAM_ATTR`, `driver/gpio.h`
- `volatile displayDigits[]`, `fadeBrightness[]`, `muxIndex`, `inBlank`
- `hw_timer_t *muxTimer`, `portMUX_TYPE timerMux`
- Forward-Deklaration `void IRAM_ATTR onMuxTimer()`
- Timer-Setup in `setup()`

**Geändert:**
- `BTN_SET=13`, `BTN_UP=14`, `BTN_DOWN=15`, `BTN_LIGHT=16`
- `#include <Wire.h>` hinzufügen
- `setup()`: `nixieInit()` aufrufen (nach WiFi, analog bisherigem Timer-Start)
- `displayDigits[]` bleibt als nicht-volatile uint8_t[6] als Staging-Buffer

### Änderungen display.ino

- `setDisplayTime()`: schreibt in `displayDigits[]`, ruft `nixieWrite(displayDigits)` auf
- `updateSlotAnimation()`: schreibt `displayDigits[i]` direkt, ruft `nixieWrite(displayDigits)` am Ende auf
- `handleEditMode()`: `portENTER_CRITICAL`/`portEXIT_CRITICAL` entfernen, `nixieWrite(displayDigits)` am Ende

---

## Nixie-Helligkeit

Die bisherige `brightLevel`-Stufenschaltung (Taster LIGHT, Web-UI) wirkt ausschließlich auf NeoPixel — bleibt unverändert. Nixie-Röhren laufen bei direkter Ansteuerung ohne PWM immer bei voller Helligkeit.

---

## Nicht geändert

- NeoPixel-Animationen, Farben, Helligkeit
- RTC-Lesen/Schreiben
- Einstellmodus (FSM), Timeout
- Slot-Machine-Animation (Logik bleibt, nur Ausgabe via nixieWrite)
- IR-Fernbedienung und Lernmodus
- Web-Interface (alle API-Endpunkte)
- WiFi AP + STA, NTP-Sync
- Preferences / NVS-Persistenz
- Power-Save (NeoPixel dimmen)
- Trennpunkt-LEDs (colonAlwaysOn, colonStatic)
