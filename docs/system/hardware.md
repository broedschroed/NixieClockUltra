# Hardware-Dokumentation

## PCB 1 – Logic Board

**KiCAD-Projekt:** `nixieclocklogic_V2` · Rev 0.9 · 2026-05-07

Das Logic Board beherbergt den Mikrocontroller, die Echtzeituhr, die Hochspannungserzeugung
und alle Nutzerschnittstellen (Taster, IR, USB). Es versorgt das Nixie Display Board
über zwei dedizierte Steckverbinder.

### Hauptkomponenten

| Ref  | Bauteil             | Funktion                                        |
|------|---------------------|-------------------------------------------------|
| U1   | ESP32-S3-WROOM-1    | Mikrocontroller, WiFi/BT, 4 MB Flash            |
| U2   | DS1302N+ (DIP-8)    | Batteriegepufferte Echtzeituhr                  |
| U3   | VS1838B             | IR-Empfänger/-Demodulator 38 kHz                |
| U4   | HV-MOD              | Boost-Converter, erzeugt ~170 V DC für Nixie-Anoden |
| U5   | AMS1117-3.3 (SOT-223)| LDO-Linearregler 5V → 3,3V                    |
| U22  | USBLC6-2SC6 (SOT-23-6)| USB-ESD-/TVS-Schutz                          |
| BT1  | CR2032              | RTC-Backup-Batterie                             |
| Y1   | 32,768 kHz Quarz    | RTC-Taktquelle                                  |
| SW1  | SET                 | Taster: Einstellmodus starten / Zeit speichern  |
| SW2  | UP                  | Taster: Wert erhöhen                            |
| SW3  | DOWN                | Taster: Wert verringern                         |
| SW4  | LIGHT               | Taster: Helligkeit / Trennpunkte                |
| SW5  | BOOT                | ESP32-Bootmodus (Programmierung)                |
| J2   | USB Micro-B         | 5V-Einspeisung und Firmware-Upload              |
| J3   | Logic (8-polig)     | Inter-Board: Logik-Signale → Nixie Display      |
| J4   | HV (4-polig)        | Inter-Board: ~170V Anodenversorgung → Display   |
| J5   | LDR (2-polig)       | Optionaler Helligkeitssensor (nicht bestückt)   |

### Versorgungsschienen

| Schiene | Spannung  | Quelle   | Verbraucher                          |
|---------|-----------|----------|--------------------------------------|
| VCC     | 5V        | J2 USB   | U4 HV-MOD, U5 AMS1117               |
| +3,3V   | 3,3V      | U5       | ESP32-S3, DS1302, VS1838B, Pull-ups  |
| HV      | ~170V DC  | U4       | Nixie-Anoden (via J4)                |
| HVgnd   | GND (HV)  | U4       | HV-Rückleiter (galvanisch getrennt)  |

---

## PCB 2 – Nixie Display Board

**KiCAD-Projekt:** `nixieclockin12_V2` · Rev 2.01 · 2026-04-06

Das Display Board enthält die eigentliche Anzeigeelektronik. Die vier MCP23017 treiben
je 16 NPN-Transistoren, die die Nixie-Kathoden individuell auf GND schalten. Die Anoden
aller Röhren liegen permanent an ~170V.

### Hauptkomponenten

| Ref      | Bauteil              | Anzahl | Funktion                                         |
|----------|----------------------|--------|--------------------------------------------------|
| U1–U4    | MCP23017-E/SO (SOIC-28) | 4   | 16-bit I²C GPIO-Expander; je 16 Ausgänge         |
| Q1–Q60   | SMBTA42 (TSOT-23)    | 60     | NPN-Transistor 300V als Kathoden-Schalter        |
| NX1–NX6  | IN-12A               | 6      | Numerische Nixie-Röhre (Aufsicht, 10 Kathoden)  |
| D1       | WS2812B-SMD (×6)     | 6      | Hintergrundbeleuchtung (Pixel 0–5, GRB)         |
| D2–D6    | WS2812B-SMD          | 5      | Zusätzliche LEDs / Trennpunkte (SMD)             |
| D7–D9    | WS2812B-THT (YF923)  | 3      | Trennpunkt-LEDs (Durchsteck, RGB)                |
| R1–R64   | 3,3 kΩ (0805)        | 64     | Basis-Vorwiderstände für Q1–Q60                  |
| R65–R70  | 10 kΩ (THT, axial)   | 6      | I²C-Pull-ups und MCP-Adress-Pull-ups             |
| C1–C4    | 100 nF (0805)        | 4      | Abblockkondensatoren je MCP23017                 |
| J1       | Logic (8-polig)      | 1      | Inter-Board: Logik-Signale ← Logic Board         |
| J2       | HV (4-polig)         | 1      | Inter-Board: ~170V ← Logic Board                 |

### MCP23017 I²C-Adressen und Röhrenzuordnung

Die Adresspins A2, A1, A0 der MCP23017 sind durch Pull-ups auf GND oder VCC verdrahtet:

| IC  | I²C-Adresse | A2 | A1 | A0 | Zuständige Röhre(n)                     |
|-----|-------------|----|----|----|-----------------------------------------|
| U1  | 0x20        | 0  | 0  | 0  | Tube 0 (Stundenzehner), Tube 1 Bits 10–15 |
| U2  | 0x21        | 0  | 0  | 1  | Tube 1 Bits 0–3, Tube 2 (Minutenzehner) |
| U3  | 0x22        | 0  | 1  | 0  | Tube 3 (Minuteneiner), Tube 4 Bits 10–15|
| U4  | 0x23        | 0  | 1  | 1  | Tube 4 Bits 0–3, Tube 5 (Sekundeneiner) |

Jeder MCP23017 hat 16 Ausgänge (GPA0–7, GPB0–7). Die Tube-zu-Bit-Zuordnung ist
im Firmware-Modul `nixie_driver.ino` als `digitPin[6][10]`-Lookup-Tabelle implementiert
(→ [firmware.md](firmware.md#nixie-ansteuerung)).

### WS2812B NeoPixel-Konfiguration

Die 10 LEDs sind als verkettete Kette an GPIO21 angeschlossen:

| Pixel | Typ           | Position      | Farbreihenfolge |
|-------|---------------|---------------|-----------------|
| 0–5   | WS2812B-SMD   | Röhrenhintergrund | GRB          |
| 6–9   | WS2812B-THT   | Trennpunkte   | RGB             |

> Die unterschiedliche Farbreihenfolge (GRB vs. RGB) der SMD- und THT-Varianten
> wird in der Firmware durch separate Farbberechnungen berücksichtigt.

---

## Inter-Board-Verbindungen

### J3 (Logic Board) ↔ J1 (Nixie Display Board) — 8-polig „Logic"

> Exakte Pin-Reihenfolge aus `nixieclocklogic_V2_BPHL.pdf` verifizieren.

| Signal  | Richtung          | Beschreibung                            |
|---------|-------------------|-----------------------------------------|
| VCC     | Logic → Display   | 3,3V Logikversorgung                    |
| GND     | —                 | Digitalmasse                            |
| SDA     | ESP32 ↔ MCP23017  | I²C Daten (GPIO8)                       |
| SCL     | ESP32 → MCP23017  | I²C Takt (GPIO9)                        |
| NEO     | ESP32 → D1        | WS2812B Datenleitung (GPIO21)           |
| —       | —                 | (reserviert)                            |

### J4 (Logic Board) ↔ J2 (Nixie Display Board) — 4-polig „HV"

| Signal | Richtung        | Beschreibung                          |
|--------|-----------------|---------------------------------------|
| HV     | Logic → Display | ~170V DC Anodenversorgung (×2 Pins)   |
| HVgnd  | —               | HV-Rückleiter (×2 Pins)              |

---

## ESP32-S3 Pin-Belegung (vollständig)

| GPIO | Signal    | Peripheral / Funktion                  |
|------|-----------|----------------------------------------|
| 2    | RTC_CE    | DS1302 Chip Enable (ThreeWire)         |
| 4    | RTC_IO    | DS1302 Data (ThreeWire)                |
| 5    | RTC_CLK   | DS1302 Clock (ThreeWire)               |
| 8    | I2C_SDA   | I²C Daten → 4× MCP23017               |
| 9    | I2C_SCL   | I²C Takt → 4× MCP23017                |
| 13   | BTN_SET   | Taster SET (INPUT_PULLUP, aktiv LOW)   |
| 14   | BTN_UP    | Taster UP (INPUT_PULLUP, aktiv LOW)    |
| 15   | BTN_DOWN  | Taster DOWN (INPUT_PULLUP, aktiv LOW)  |
| 16   | BTN_LIGHT | Taster LIGHT (INPUT_PULLUP, aktiv LOW) |
| 21   | NEO_DATA  | WS2812B DIN (10 LEDs in Kette)         |
| 48   | IR_RECV   | VS1838B demodulierter Ausgang          |
