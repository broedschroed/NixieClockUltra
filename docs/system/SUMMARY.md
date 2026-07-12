# Systemdokumentation – Übersicht

## README.md

Einstieg ins System: 4-Satz-Beschreibung der Uhr, Tabelle der zwei Platinen
(`nixieclocklogic_V2` / `nixieclockin12_V2`), Beschreibung der Inter-Board-Steckverbinder
J3↔J1 und J4↔J2, Mermaid-Blockschaltbild mit allen Subsystemen (Spannungsversorgung,
ESP32-S3, MCP23017-Kette, NeoPixel, RTC, IR, WiFi/NTP/Browser), Bibliothekstabelle
(6 Einträge, ArduinoJson auf v6.x festgelegt), Links zu den anderen zwei Dateien und
zur Bedienungsanleitung.

## hardware.md

PCB-Details: **PCB 1 (Logic Board)** mit Komponententabelle (U1–U22, Taster,
Steckverbinder, CR2032-Batterie), Versorgungsschienen (+3,3V / HV ~170V / HVgnd).
**PCB 2 (Nixie Display Board)** mit 4× MCP23017 inkl. I²C-Adresstabelle (0x20–0x23
mit A2/A1/A0-Belegung und Röhrenzuordnung), 60× SMBTA42, 6× IN-12A,
WS2812B-Pixel-Konfiguration (Pixel 0–5 GRB Hintergrund, Pixel 6–9 RGB Trennpunkte).
Inter-Board-Signaltabellen für J3↔J1 und J4↔J2. Vollständige ESP32-S3 Pin-Belegung
(11 GPIOs).

## firmware.md

Firmware-Architektur: 11-Modul-Tabelle mit Zeilenanzahl (inkl. `digit_fade.ino` und
`hv_dimmer.ino`), korrekter 11-Schritt-Setup-Ablauf, alle GPIO-Defines und Konstanten als
Codeblock, 32 globale State-Variablen, alle 3 Enumerationen (`AnimMode`, `EditState`,
`IrAction` mit `IR_ACTION_`-Präfix), Erklärung der MCP23017-Direktansteuerung (Mutex,
Shadow-Register, Lookup-Tabelle), der HV-Anodendimmung (TLP627/LEDC) und des weichen
Ziffernwechsels (`digit_fade.ino`-Crossfade-State-Machine, Slot-Machine-Geschwindigkeit),
vollständige Web-API-Tabelle (19 Endpunkte mit verifizierten JSON-Feldnamen),
NVS-Persistenz-Tabelle (27 Schlüssel mit korrekten Namen wie `bright`, `colonOn`,
`wifiSsid`, `sfSecEn`, `sfDateEn`, `slotSpeed`, `ir_SET`…`ir_COLTOGGLE`).
