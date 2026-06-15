# Design-Spec: Systemdokumentation Nixie Clock Ultra

**Datum:** 2026-06-15
**Status:** Freigegeben

---

## Ziel

Technische Systemdokumentation für Entwickler und Dritte, die das System verstehen
oder weiterentwickeln möchten. Kein Nachbau-Guide, keine BOM, keine Fertigungsinfos.

---

## Format

- Drei Markdown-Dateien unter `docs/system/`
- Diagramme als Mermaid-Blöcke (GitHub-rendert sie automatisch)
- Tabellen für alle tabellarischen Daten (Pins, APIs, Komponenten)
- Sprache: Deutsch

---

## Dateistruktur

```
docs/system/
├── README.md      ← Einstieg, Systemübersicht, Blockschaltbild
├── hardware.md    ← PCB-Beschreibungen, Inter-Board-Verbindungen, Pin-Belegung
└── firmware.md    ← Firmware-Module, Ablauf, State, API, Persistenz
```

Das bestehende `blockschaltbild.md` im Repo-Root wird durch einen Redirect-Hinweis
auf `docs/system/README.md` ersetzt (nicht gelöscht, um Git-History zu erhalten).

---

## Datei 1: `docs/system/README.md`

### Abschnitte

#### 1.1 Systembeschreibung (3–5 Sätze)
Kurze Einführung: ESP32-S3-basierte Nixie-Röhrenuhr, Zweiplatinendesign,
direkte Ansteuerung via MCP23017, WiFi + Web-UI, IR-Fernbedienung.

#### 1.2 Zweiplatinenübersicht (Tabelle)

| Platine                | KiCAD-Projekt          | Funktion                                      |
|------------------------|------------------------|-----------------------------------------------|
| Logic Board            | nixieclocklogic_V2     | ESP32-S3, RTC, HV-Modul, Taster, IR, USB      |
| Nixie Display Board    | nixieclockin12_V2      | 4× MCP23017, 6× IN-12A, 60× NPN, WS2812B     |

#### 1.3 Systemblockschaltbild (Mermaid)

Aktualisiertes Blockschaltbild (ersetzt das veraltete `blockschaltbild.md`,
das noch Mux-Timer ISR enthielt). Zeigt:
- Spannungsversorgung (5V USB → AMS1117 3.3V + HV-MOD ~170V)
- ESP32-S3 mit WiFi-Stack (AP + STA), NVS Flash
- I²C-Bus → 4× MCP23017 → 60× SMBTA42 → 6× IN-12A Nixie
- DS1302 RTC (ThreeWire: IO=GPIO4, CLK=GPIO5, CE=GPIO2)
- 10× WS2812B NeoPixel (GPIO21)
- VS1838B IR-Empfänger (GPIO48)
- 4× Taster SET/UP/DOWN/LIGHT (GPIO13–16)
- NTP (pool.ntp.org, nur bei STA-Verbindung)
- Web-Browser → HTTP API (192.168.4.1)

#### 1.4 Bibliotheksabhängigkeiten (Tabelle)

| Bibliothek         | Quelle            | Zweck                        |
|--------------------|-------------------|------------------------------|
| Adafruit NeoPixel  | Arduino Lib Mgr   | WS2812B-Ansteuerung          |
| Rtc by Makuna      | Arduino Lib Mgr   | DS1302 RTC                   |
| AsyncTCP           | me-no-dev         | Async TCP für ESP32          |
| ESPAsyncWebServer  | me-no-dev         | HTTP-Server                  |
| ArduinoJson        | Benoit Blanchon   | JSON-Serialisierung (v6+)    |
| IRremoteESP8266    | David Conran      | IR-Empfang (NEC, Samsung …)  |

---

## Datei 2: `docs/system/hardware.md`

### Abschnitte

#### 2.1 PCB 1 – Logic Board (`nixieclocklogic_V2`, Rev 0.9, 2026-05-07)

Hauptkomponenten-Tabelle:

| Ref  | Wert / Teil         | Funktion                                     |
|------|---------------------|----------------------------------------------|
| U1   | ESP32-S3-WROOM-1    | Mikrocontroller, WiFi/BT                     |
| U2   | DS1302N+            | Batteriegepufferte Echtzeituhr (DIP-8)       |
| U3   | VS1838B             | IR-Empfänger 38 kHz                          |
| U4   | HV-MOD              | Boost-Converter, erzeugt ~170 V DC           |
| U5   | AMS1117-3.3         | LDO-Regler 5V → 3.3V                        |
| U22  | USBLC6-2SC6         | USB-ESD-Schutz                               |
| BT1  | CR2032              | RTC-Backup-Batterie                          |
| SW1–4| Taster              | SET, UP, DOWN, LIGHT                         |
| J2   | USB Micro-B         | 5V-Einspeisung und Programmierung            |
| J3   | Logic (8-pin)       | Inter-Board: I²C + Versorgung → Display      |
| J4   | HV (4-pin)          | Inter-Board: ~170V + HVgnd → Display        |

Versorgungsschienen: +3.3V (Logik), VCC (pull-ups/RTC), GND, HV (~170V), HVgnd.

#### 2.2 PCB 2 – Nixie Display Board (`nixieclockin12_V2`, Rev 2.01, 2026-04-06)

Hauptkomponenten-Tabelle:

| Ref      | Wert / Teil       | Anzahl | Funktion                                      |
|----------|-------------------|--------|-----------------------------------------------|
| U1–U4    | MCP23017-E/SO     | 4      | I²C GPIO-Expander, je 16 Ausgänge (SOIC-28)  |
| Q1–Q60   | SMBTA42           | 60     | NPN-Transistor (300V) als Kathodenschalter    |
| NX1–NX6  | IN-12A            | 6      | Nixie-Röhren (numerisch, Aufsicht)           |
| D1       | 6× WS2812B-SMD    | 6      | Hintergrundbeleuchtung (Pixel 0–5)           |
| D2–D6    | WS2812B-SMD       | 5      | Trennpunkt-LEDs (SMD)                        |
| D7–D9    | WS2812B (THT)     | 3      | Trennpunkt-LEDs (Durchsteck, YF923)          |
| J1       | Logic (8-pin)     | 1      | Inter-Board: I²C + Versorgung ← Logic Board  |
| J2       | HV (4-pin)        | 1      | Inter-Board: ~170V ← Logic Board             |

MCP23017 I²C-Adressen (A2=0, A1=0/1, A0=0/1):

| IC  | Adresse | A2 | A1 | A0 | Zuständige Röhren                      |
|-----|---------|----|----|----|-----------------------------------------|
| U1  | 0x20    | 0  | 0  | 0  | Tube 0 (HZ), Tube 1 (HE) Bits 10–15   |
| U2  | 0x21    | 0  | 0  | 1  | Tube 1 (HE) Bits 0–3, Tube 2 (MZ)     |
| U3  | 0x22    | 0  | 1  | 0  | Tube 3 (ME), Tube 4 (SZ) Bits 10–15   |
| U4  | 0x23    | 0  | 1  | 1  | Tube 4 (SZ) Bits 0–3, Tube 5 (SE)     |

Jeder MCP23017 steuert 16 NPN-Transistoren (über 3.3kΩ Basiswiderstände),
die jeweils eine Nixie-Kathode auf GND ziehen. Die Anoden aller Röhren
liegen permanent an HV (~170V). Digit-Wert 10 = alle Bits aus = Röhre blank.

#### 2.3 Inter-Board-Verbindungen

J3 (Logic Board) ↔ J1 (Nixie Display Board), 8-polig:

> Exakte Pin-Nummern aus Schaltplan-PDF `nixieclocklogic_V2_BPHL.pdf` verifizieren.

| Pin | Signal | Richtung         | Beschreibung                     |
|-----|--------|------------------|----------------------------------|
| —   | VCC    | Logic → Display  | 3.3V Logikversorgung             |
| —   | GND    | —                | Digitalmasse                     |
| —   | SDA    | ESP32 ↔ MCP23017 | I²C Daten (GPIO8)                |
| —   | SCL    | ESP32 → MCP23017 | I²C Takt (GPIO9)                 |
| —   | NEO    | ESP32 → D1       | WS2812B Datenleitung (GPIO21)    |
| —   | —      | —                | (reserviert / nicht belegt)      |

J4 (Logic Board) ↔ J2 (Nixie Display Board), 4-polig:

| Pin | Signal | Richtung        | Beschreibung             |
|-----|--------|-----------------|--------------------------|
| 1–2 | HV     | Logic → Display | ~170V Anodenversorgung   |
| 3–4 | HVgnd  | —               | HV-Rückleiter            |

#### 2.4 ESP32-S3 Pin-Belegung

| GPIO | Funktion    | Modul/Peripheral                  |
|------|-------------|-----------------------------------|
| 2    | RTC_CE      | DS1302 Chip Enable                |
| 4    | RTC_IO      | DS1302 Data                       |
| 5    | RTC_CLK     | DS1302 Clock                      |
| 8    | I2C_SDA     | MCP23017 ×4 (Daten)               |
| 9    | I2C_SCL     | MCP23017 ×4 (Takt)                |
| 13   | BTN_SET     | Taster SET (aktiv LOW)            |
| 14   | BTN_UP      | Taster UP (aktiv LOW)             |
| 15   | BTN_DOWN    | Taster DOWN (aktiv LOW)           |
| 16   | BTN_LIGHT   | Taster LIGHT (aktiv LOW)          |
| 21   | NEO_DATA    | WS2812B DIN (10 LEDs)             |
| 48   | IR_RECV     | VS1838B Ausgang                   |

---

## Datei 3: `docs/system/firmware.md`

### Abschnitte

#### 3.1 Modulübersicht

| Datei               | Zeilen | Inhalt                                                    |
|---------------------|--------|-----------------------------------------------------------|
| `NixieClockUltra.ino` | 359  | Globals, `setup()`, `loop()`, Edit-Mode-FSM, Slot-Anim  |
| `nixie_driver.ino`  | 91     | `nixieInit()`, `nixieWrite()`, MCP23017-Abstraktion      |
| `display.ino`       | 47     | Fade-In/Out, `setDisplayTime()`, Digit-Mapping           |
| `buttons.ino`       | 119    | Entprell-FSM für 4 Taster, Kurz-/Langdruck-Erkennung    |
| `rtc.ino`           | 15     | `readRTC()`, `writeRTC()` via DS1302/ThreeWire           |
| `neo_animation.ino` | 99     | Rainbow, Statisch, Puls, Slot-Machine, Trennpunkt-Blinken|
| `web_server.ino`    | 559    | HTML/JS (embedded), alle API-Handler, WiFi-Setup, NTP    |

#### 3.2 Setup-Ablauf

Nummerierte Liste der `setup()`-Schritte:
1. Serial (115200)
2. `nixieInit()` — I²C + MCP23017 initialisieren, alle Pins als Output, alle Bits 0
3. NeoPixel-Objekt initialisieren, Helligkeit setzen
4. RTC lesen (`readRTC()`), Zeit in `curHour/curMin/curSec` laden
5. Taster-Pins konfigurieren (INPUT_PULLUP)
6. IR-Empfänger starten (`IrReceiver.begin()`)
7. NVS laden (Helligkeit, Animation, WiFi-Creds, IR-Codes, colonAlwaysOn)
8. `setupWifi()` — AP starten + ggf. STA verbinden
9. `setupWebServer()` — alle Routes registrieren, server.begin()
10. NTP konfigurieren (nur wenn STA verbunden)
11. Fade-In-Sequenz starten (`startFadeIn = true`)

#### 3.3 Globale State-Variablen

| Variable         | Typ             | Beschreibung                                 |
|------------------|-----------------|----------------------------------------------|
| `displayDigits`  | `uint8_t[6]`    | Aktuell angezeigte Ziffern (0–9, 10=blank)   |
| `brightLevel`    | `uint8_t`       | Helligkeitsstufe 0–3 (Index in BRIGHTNESS_LEVELS[]) |
| `neoBright`      | `uint8_t`       | NeoPixel-Feinwert 10–255 (Hintergrund)       |
| `colonBright`    | `uint8_t`       | NeoPixel-Feinwert (Trennpunkte)              |
| `neoHue`         | `uint8_t`       | Auto-inkrement für Rainbow-Animation         |
| `animMode`       | `AnimMode`      | ANIM_RAINBOW / ANIM_STATIC / ANIM_PULSE / ANIM_SLOTS |
| `editState`      | `EditState`     | EDIT_NONE / EDIT_HOUR / EDIT_MIN / EDIT_SEC  |
| `colonAlwaysOn`  | `bool`          | Trennpunkt-LEDs dauerhaft an (kein Blinken)  |
| `irCodes[]`      | `uint64_t[7]`   | Angelernte IR-Codes, Index = IrAction-enum   |
| `wifiStaConnected`| `bool`         | STA-Verbindung aktiv                         |
| `ntpSynced`      | `bool`          | NTP-Synchronisierung erfolgt                 |

#### 3.4 Nixie-Ansteuerung (nixie_driver.ino)

Direktansteuerung ohne Multiplexing:
- `nixieInit()`: Wire.begin(SDA=8, SCL=9), alle 4 MCP23017 auf Output, alle Bits 0
- `nixieWrite(digits[6])`: Berechnet für jede Röhre das zugehörige MCP-Bit anhand
  der `digitPin[tube][digit]`-Lookup-Tabelle, schreibt nur geänderte MCPs
  (Shadow-Register `mcpState[]`), geschützt durch FreeRTOS-Mutex
- Digit-Wert 10 → kein Bit gesetzt → Röhre blank

Tube-zu-MCP-Bit-Zuordnung (kompakt):

| Tube | Funktion     | MCP  | Bits (GPA/GPB)        |
|------|-------------|------|-----------------------|
| 0    | Stundenzehner| 0x20 | Bits 0–9              |
| 1    | Stundeneiner | 0x20 | Bits 10–15, 0x21 Bits 0–3 |
| 2    | Minutenzehner| 0x21 | Bits 4–13             |
| 3    | Minuteneiner | 0x22 | Bits 0–9              |
| 4    | Sekundenzehner| 0x22 | Bits 10–15, 0x23 Bits 0–3 |
| 5    | Sekundeneiner | 0x23 | Bits 4–13             |

#### 3.5 Web-API-Endpunkte

| Pfad              | Methode | Body / Parameter                        | Funktion                          |
|-------------------|---------|-----------------------------------------|-----------------------------------|
| `/`               | GET     | —                                       | Liefert eingebettetes HTML/JS/CSS |
| `/api/status`     | GET     | —                                       | Zeit, Helligkeit, Anim, WiFi, NTP |
| `/api/settime`    | POST    | `{h, m, s}`                             | Zeit in RTC schreiben             |
| `/api/bright`     | POST    | `{level}` (0–3)                         | Helligkeitsstufe setzen           |
| `/api/neobright`  | POST    | `{value}` (10–255)                      | NeoPixel-Feinwert setzen          |
| `/api/anim`       | POST    | `{mode}` (0–3)                          | Animationsmodus setzen            |
| `/api/colonbright`| POST    | `{value}`                               | Trennpunkt-Helligkeit             |
| `/api/colonon`    | POST    | `{on}` (bool)                           | Trennpunkte dauerhaft an/aus      |
| `/api/colonstatic`| POST    | `{static}` (bool)                       | Trennpunkte statisch (kein Blinken)|
| `/api/slot`       | POST    | —                                       | Slot-Machine-Animation auslösen   |
| `/api/wifi`       | GET     | —                                       | WiFi-Status (AP+STA IP, NTP)      |
| `/api/wifi`       | POST    | `{ssid, password}`                      | STA-Verbindung einrichten         |
| `/api/wifi/scan`  | GET     | —                                       | WLAN-Netzwerke scannen            |
| `/api/ir/status`  | GET     | —                                       | Aktuelle IR-Code-Belegung         |
| `/api/ir/learn`   | POST    | `{action}` (0–6)                        | IR-Lernmodus starten              |
| `/api/ir/clear`   | POST    | `{action}` (0–6)                        | IR-Code löschen                   |

#### 3.6 NVS-Persistenz (Preferences)

Alle Einstellungen werden bei Änderung in den ESP32-NVS-Flash geschrieben
und beim Start automatisch geladen:

| Schlüssel     | Typ    | Inhalt                              |
|---------------|--------|-------------------------------------|
| `brightLevel` | uint8  | Helligkeitsstufe (0–3)              |
| `neoBright`   | uint8  | NeoPixel-Feinwert (10–255)          |
| `animMode`    | uint8  | Animationsmodus (0–3)               |
| `colonAlwaysOn`| bool  | Trennpunkte dauerhaft an            |
| `colonStatic` | bool   | Kein Blinken der Trennpunkte        |
| `wifiSSID`    | String | Heimnetz SSID                       |
| `wifiPass`    | String | Heimnetz Passwort                   |
| `irCode_0`…`irCode_6` | uint64 | Angelernte IR-Codes je Funktion |

---

## Ausgabedateien

- `docs/system/README.md`
- `docs/system/hardware.md`
- `docs/system/firmware.md`
- `blockschaltbild.md` → Hinweis auf `docs/system/README.md` (kein Delete)
