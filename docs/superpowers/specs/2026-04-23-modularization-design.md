# Modularisierung NixieClockUltra – Design-Spec

**Projekt:** NixieClockUltra  
**Datum:** 2026-04-23  
**Status:** Freigegeben

---

## Ziel

Die monolithische `NixieClockUltra.ino` (~1527 Zeilen) wird in 8 thematisch abgegrenzte `.ino`-Dateien aufgeteilt. Der Code wird dabei nicht inhaltlich verändert – es ist ein reines Refactoring zur Verbesserung der Übersichtlichkeit und Wartbarkeit.

---

## Architektur

Arduino IDE fügt alle `.ino`-Dateien eines Sketch-Ordners alphabetisch hinter der Haupt-`.ino`-Datei zu einer einzigen Übersetzungseinheit zusammen. Damit gilt:

- Alle globalen Variablen und Funktionen sind in allen Dateien direkt sichtbar.
- `#include`-Direktiven und `#define`-Makros stehen ausschließlich in `NixieClockUltra.ino`.
- Keine `extern`-Deklarationen nötig.
- Alphabetische Reihenfolge der Dateien ist unkritisch, da Arduino IDE für `.ino`-Dateien automatisch Forward-Deklarationen erzeugt.

---

## Dateistruktur

| Datei | Verantwortlichkeit | Enthält |
|---|---|---|
| `NixieClockUltra.ino` | Konfiguration, Globals, Einstiegspunkt | `#include`, Pin-Defines, Konstanten, alle globalen Variablen, `setup()`, `loop()` |
| `buttons.ino` | Tasteneingabe und Einstellmodus | `Button`-Struct, `updateButton()`, `handleEditMode()`, `handleBrightness()`, `handlePowerSave()` |
| `display.ino` | Anzeige-Logik | `setDisplayTime()`, `startSlotAnimation()`, `updateSlotAnimation()`, Fade-In-Logik |
| `ir_remote.ino` | IR-Fernbedienung | `IrAction`-Enum, IR-Globals (`irCodes[]`, `irLearnTarget`, `irMux` etc.), `executeAction()`, `dispatchIRAction()`, `handleIR()` |
| `mux_timer.ino` | Nixie-Multiplexing | `onMuxTimer()` ISR (IRAM_ATTR), Timer-Init-Hilfsfunktion |
| `neo_animation.ino` | NeoPixel-Beleuchtung | `updateNeoPixel()` |
| `rtc.ino` | Echtzeituhr | `readRTC()`, `writeRTC()` |
| `web_server.ino` | Web-Interface | `WEB_PAGE` PROGMEM-String, `setupWifi()`, `setupWebServer()` mit allen API-Endpunkten |

---

## Globale Variablen (bleiben in `NixieClockUltra.ino`)

Alle globalen Variablen verbleiben in der Haupt-Datei, damit ihre Definition eindeutig lokalisiert ist:

- `displayDigits[]`, `fadeBrightness[]`, `muxIndex`, `inBlank`
- `brightLevel`, `neoHue`, `neoSat`, `neoBright`, `animMode`
- `editState`, `curHour`, `curMin`, `curSec`
- `lastInteractionMs`, `powerSaveActive`, `powerSaveEnabled`
- `wifiStaConnected`, `ntpSynced`, `slotActive`, `startFadeIn`
- `muxTimer`, `timerMux` (portMUX_TYPE)
- `irCodes[]`, `irLearnTarget`, `irLearnStartMs`, `irMux` (portMUX_TYPE)
- Objekte: `rtcWire`, `Rtc`, `strip`, `server`, `prefs`, `irrecv`, `irResults`
- Button-Instanzen: `btnSet`, `btnUp`, `btnDown`, `btnLight`

---

## Randbedingungen

- **Kein inhaltlicher Code-Change:** Nur Verschiebung von Funktionen und Typdefinitionen zwischen Dateien. Kein Umschreiben, keine neuen Abstraktionen.
- **`IrAction`-Enum** wandert zusammen mit den IR-Globals nach `ir_remote.ino`, da es nur dort benötigt wird. Da die Arduino IDE alle Dateien zusammenfügt, ist der Typ trotzdem in der gesamten Sketch-Einheit sichtbar.
- **`Button`-Struct** wandert nach `buttons.ino`; die Button-Instanzen (`btnSet` etc.) bleiben als globale Variablen in `NixieClockUltra.ino`.
- **PROGMEM-String `WEB_PAGE`** gehört vollständig nach `web_server.ino`.
- Die Haupt-Datei `NixieClockUltra.ino` enthält nach dem Refactoring nur noch Includes, Defines, Globals, `setup()` und `loop()`.
