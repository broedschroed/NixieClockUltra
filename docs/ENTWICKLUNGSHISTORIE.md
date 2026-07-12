# NixieClockUltra – Entwicklungshistorie

Chronologische Übersicht aller Entwicklungsschritte, Architekturentscheidungen und Problemlösungen.

---

## Phase 1 – Initiales Firmware-Grundgerüst (2026-04-22)

**Ausgangspunkt:** Neue Hardware auf Basis ESP32-S3 mit 6 Nixie-Röhren, WS2812B NeoPixel, DS1302 RTC und IR-Empfänger.

**Was entstand:**
- Vollständige Basis-Firmware in einer einzigen `.ino`-Datei
- Nixie-Ansteuerung via Multiplex-ISR (Hardware-Timer)
- NeoPixel-Animationen (Rainbow, Static, Pulse)
- Web-Interface (WiFi AP, AsyncWebServer)
- IR-Fernbedienung mit web-basiertem Anlernmodus (7 Funktionen)
- Power-Save-Toggle (später wieder entfernt)
- DS1302 RTC-Integration

---

## Phase 2 – Modularisierung (2026-04-23)

**Problem:** Die einzige `.ino`-Datei war unübersichtlich geworden.

**Lösung:** Code aufgeteilt in separate Module:

| Datei | Inhalt |
|---|---|
| `mux_timer.ino` | Multiplex-ISR (Hardware-Timer) |
| `rtc.ino` | DS1302 readRTC / writeRTC |
| `display.ino` | setDisplayTime, Slot-Animation |
| `neo_animation.ino` | updateNeoPixel, Animationen |
| `buttons.ino` | Taster-Entprellung, Einstellmodus |
| `ir_remote.ino` | IR-Empfang, Anlernmodus |
| `web_server.ino` | HTML, WiFi-Setup, API-Endpoints |

**Problem dabei:** Der Arduino-Präprozessor erkennt Forward Declarations für `IRAM_ATTR`-Funktionen und Web-Server-Callbacks nicht automatisch → Forward Declarations manuell in `NixieClockUltra.ino` ergänzt.

---

## Phase 3 – Hardware-Inbetriebnahme & Fixes (2026-05-03)

### NeoPixel RGB-Swap
**Problem:** Die 4 Trennpunkt-LEDs (THT WS2812B) zeigten falsche Farben.  
**Ursache:** THT-WS2812B verwenden RGB-Bytereihenfolge statt GRB wie die SMD-Variante.  
**Lösung:** `rgbSwap` für Pixel 6–9 in `neo_animation.ino`.

### Kathoden-Mapping
**Problem:** Ziffern wurden auf falschen Nixie-Röhren angezeigt.  
**Lösung:** Kathoden-Mapping-Tabelle in ISR korrigiert.

### Trennpunkte dauerhaft an (`colonAlwaysOn`)
**Feature:** Neue Option, Trennpunkte permanent zu beleuchten statt sekündlich zu blinken. Steuerbar per langem Tastendruck (BTN_LIGHT) und per IR.

### Getrennte Helligkeit für Hintergrund und Trennpunkte
**Feature:** `neoBright` (Pixel 0–5) und `colonBright` (Pixel 6–9) separat einstellbar, da THT- und SMD-LEDs unterschiedliche Helligkeiten haben.

### Power-Save entfernt
**Entscheidung:** Power-Save-Funktion (`b1a83dd`) vollständig entfernt – Funktion zu wenig genutzt, verkomplizierte den Code.

### Anti-Ghosting – erste Versuche
**Problem:** Nixie-Röhren zeigten Geisterziffern (Ghosting) durch den Multiplex-Betrieb.  
**Versuch 1** (`ad6eb0b`): Anode vor Kathode schalten (dV/dt-Transiente) → kein Erfolg, revertiert (`1bcd7ff`).  
**Versuch 2** (`6502fd1`): Blank-Phase im Multiplex-Zyklus verlängert → reduziert, aber nicht beseitigt.

---

## Phase 4 – Anti-Ghosting Redesign: MCP23017 Direct Drive (2026-05-09)

**Problem:** Multiplex-Ghosting war strukturell nicht lösbar – Ursache sind gemeinsame Kathoden beim Umschalten.

**Architekturentscheidung:** Kompletter Umbau der Nixie-Ansteuerung:
- **Alt:** 1× 74HC595 Schieberegister + Transistor-Matrix, Multiplex-ISR
- **Neu:** 4× MCP23017 I2C Port-Expander (Adressen 0x20–0x23), SMBTA42-Transistoren schalten Kathoden direkt, Anoden permanent auf HV

**Implementierung:**
- `nixie_driver.ino` (neu): MCP23017-Treiber via `Wire.h`, Shadow-Register, FreeRTOS-Mutex für I2C-Zugriff aus verschiedenen Tasks
- `mux_timer.ino` (gelöscht): ISR und Hardware-Timer nicht mehr benötigt
- Taster-Pins angepasst: SET=13, UP=14, DOWN=15, LIGHT=16

**Fix:** `nixieInit()` muss vor `setDisplayTime()` in `setup()` aufgerufen werden (Wire.begin muss vor I2C-Zugriffen stehen).

**Ergebnis (getestet 2026-06-15):** Ghosting vollständig beseitigt.

---

## Phase 5 – Dokumentation (2026-06-15)

### Steampunk-Bedienungsanleitung
Vollständige Bedienungsanleitung im Steampunk-Design (HTML → PDF):
- Kapitel I–IX: Übersicht, Inbetriebnahme, Tasten, Zeiteinstellung, Beleuchtung, IR, WLAN, Persistenz, Technische Daten
- SVG-Ornamente, Nixie-Illustration, gold/schwarz Farbschema

### Systemdokumentation
Technische Dokumentation (ODT) mit eingebetteten Schaltplänen:
- Hardware: PCBs, Pins, Inter-Board-Verbindungen
- Firmware: Module, API-Endpoints, NVS-Persistenz
- Bibliotheken: ArduinoJson v6.x (v7 inkompatibel wegen API-Änderungen)

**Fix:** IR-API-Label `COLTOGGLE` → `COLON_TOGGLE` in Dokumentation und Firmware korrigiert.

---

## Phase 6 – Slot-Intervall & Feinschliff (2026-06-17)

### Slot-Intervall-Redesign
**Problem:** Slot-Machine-Animation war Teil des `AnimMode`-Enums, was zu Konflikten führte (Slot lässt sich nicht als dauerhafte Animation verwenden).

**Lösung:** Slot-Intervall als eigene Einstellung (`SlotInterval`-Enum) ausgelagert:
- `SLOT_OFF`, `SLOT_10S`, `SLOT_1MIN`, `SLOT_15MIN`, `SLOT_1HR`
- Slot-Effekt als **Override-Schicht** über die eigentliche Animation (nicht als eigener AnimMode)
- `ANIM_SLOTS` aus `AnimMode` entfernt
- NVS-Schlüssel: `slotIval`
- Neuer API-Endpoint: `POST /api/slotinterval`

### IR-Empfang auf ESP32-S3
**Problem:** IR-Codes wurden nach `strip.show()` nicht mehr empfangen.  
**Ursache:** `irrecv.pause()` / `irrecv.resume()` um `strip.show()` resettete den RMT-Empfänger alle 20 ms und unterbrach den Empfang dauerhaft.  
**Lösung:** `pause()`/`resume()` vollständig entfernt. Auf dem ESP32-S3 sind RMT-TX (NeoPixel) und RMT-RX (IR) unabhängige Kanäle – kein Konflikt.

### colonStatic + colonAlwaysOn
**Problem:** `colonStatic` (Warmweiß-Modus für Trennpunkte) ignorierte `colonAlwaysOn` – Trennpunkte blinkten trotz `colonAlwaysOn=true`.  
**Lösung:** `colonStatic`-Logik prüft nun zusätzlich `colonAlwaysOn`.  
**Weiterer Fix:** `colonStatic` wird während der Slot-Animation korrekt übersprungen.

### Taster-Entprellung
**Problem:** `pressed` wurde nie `true`, obwohl Taster gedrückt war.  
**Ursache:** Logikfehler in der Button-FSM: Das `debounced`-Flag wurde nie gesetzt, weil die Bedingung `lastState == HIGH` innerhalb des Debounce-Fensters niemals zutraf.  
**Lösung:** `Button`-Struct um `debounced`-Flag (bool) erweitert; `pressed` feuert einmalig nach Ablauf der Entprellzeit, `held` feuert alle `repeatMs` danach.

### Taster-Pins korrigiert
**Problem:** UP- und DOWN-Taster reagierten nicht.  
**Ursache:** Pins aus der MCP23017-Umbauphase waren noch auf alten Werten (14/15).  
**Lösung:** UP=IO12, DOWN=IO11, LIGHT=IO10 (SET=IO13 war korrekt).

### Farbabgleich Warmweiß
- `BG_WARM_R/G/B`-Defines für Hintergrund-Warmweiß eingeführt (statt Magic Numbers)
- `COLON_WARM_R/G/B`-Defines für Trennpunkt-Warmweiß
- Grünanteil Hintergrund: 180 → 130 nach visuellem Abgleich an der Hardware

---

## Phase 7 – WiFi-Fixes & mDNS (2026-06-20)

### WiFi-Scan funktionierte nicht
**Problem:** Scan lieferte keine Netzwerke.  
**Ursachen:**
1. `WiFi.mode(WIFI_AP)` beim ersten Start → STA-Radio deaktiviert → `scanNetworks()` funktionslos
2. `WiFi.scanNetworks()` (synchroner Aufruf) im ESPAsyncWebServer-Handler blockiert den LWIP/WiFi-Task → Response wird nie gesendet

**Lösung:** Scan-Funktion vollständig entfernt (Scan-Button, Select-Element, JS-Funktion, API-Endpoint). SSID wird manuell eingegeben.

### STA-Verbindung blieb nach Neustart im AP-Modus
**Ursachen:**
1. `WIFI_AP`-Modus beim ersten Start → nach Speichern der Credentials und Neustart manchmal kein Verbindungsaufbau
2. STA-Timeout von 10 Sekunden zu kurz (DHCP, Channel-Wechsel)

**Lösungen:**
- `WiFi.mode()` immer auf `WIFI_AP_STA` setzen (auch ohne gespeicherte Credentials)
- STA-Timeout: 10 s → 20 s

### mDNS-Hostname
**Feature:** Nach erfolgreicher STA-Verbindung meldet sich die Uhr per mDNS und DHCP unter einem konfigurierbaren Namen.  
**Implementierung:**
- `#define WIFI_HOSTNAME "nixieclockcs"` in `NixieClockUltra.ino`
- `WiFi.setHostname(WIFI_HOSTNAME)` vor `WiFi.mode()` → DHCP-Eintrag im Router
- `MDNS.begin(WIFI_HOSTNAME)` nach erfolgreicher Verbindung → `http://nixieclockcs.local` (funktioniert auf iOS, macOS, Windows 10+; nicht auf Android)

---

## Phase 8 – HV-Anodendimmung (2026-07-07)

**Problem:** Der Nacht-Modus dimmte die Röhren per Software-PWM auf den Kathoden (`nixiePwmOn`-Gate in `nixieWriteSafe()`, 20-ms-Rhythmus) – funktionierte, war aber unnötig kompliziert und erforderte einen Blitzschutz bei Sekundenwechsel.

**Architekturentscheidung:** Dimmung direkt auf der Anodenspannung statt auf den Kathoden:
- **Neu:** TLP627-Optokoppler schaltet die ~170-V-Anodenspannung per LEDC-Hardware-PWM (~200 Hz) auf GPIO7 (`HV_SWITCH_PIN`)
- `hv_dimmer.ino` (neu): `hvDimmerInit()`, `hvDimmerSetDuty()`
- `nixieWriteSafe()` dadurch auf reinen Passthrough zu `nixieWrite()` reduziert – kein Blitzschutz mehr nötig
- TLP627 aktuell handverdrahtet ergänzt, noch nicht mit eigener Referenz im KiCad-Schaltplan

**Web-UI-Feinschliff:**
- Neuer Regler „Dimm-Helligkeit“ im Nacht-Modus, stufenlos 2–60 % (`hvDimPct`, NVS-Key `hvDimPct`)
- Nacht-Modus-Karte komplett auf AJAX umgestellt (kein „Übernehmen“-Button mehr, alle Felder speichern sofort wie im Abschnitt „Helligkeit & Animation“)
- Redundantes „Nixie-Helligkeit“-Dropdown entfernt (Stufen werden weiterhin von Tastern/IR genutzt); dafür `/api/bright`-Endpoint als totes Backend entfernt
- `BRIGHTNESS_LEVELS` für Taster/IR von `{10, 30, 50, 80}` auf `{10, 40, 80, 200}` angepasst

### Beobachtung: Web-UI-Verzögerung nach dem Umbau
**Beobachtung:** Nach dem Umbau wirkte die Web-UI sowohl im AP- als auch im WLAN-Modus spürbar verzögert.
**Erste Hypothese:** Das Schalten der vollen Anodenlast mit 200 Hz koppelt Störungen in den WLAN-Funkteil ein.
**Einwand (berechtigt):** Sowohl beim alten Kathoden-Software-PWM als auch beim neuen Anoden-Hardware-PWM wird pro leuchtender Ziffer dieselbe Strommenge geschaltet – die Störquelle wäre also nicht grundsätzlich neu.
**Status:** Bei einem erneuten Test trat die Verzögerung nicht mehr auf. Mangels Reproduzierbarkeit bleibt die genaue Ursache ungeklärt.

**Dokumentation:** Bedienungsanleitung, Systemdokumentation (`hardware.md`, `firmware.md`, `gen_sysdoc.py`) und Website (`features.html`, `aufbau.html` inkl. Spannungsfluss-Diagramm) auf den HV-Dimmer-Umbau aktualisiert.

---

## Phase 9 – Weicher Ziffernwechsel & Slot-Geschwindigkeit (2026-07-11)

**Ziel:** Ziffernwechsel sollen nicht mehr schlagartig, sondern per sanftem Crossfade erfolgen – getrennt zuschaltbar für den Sekundentakt und für den Übergang zwischen Uhrzeit- und Datumsanzeige. Zusätzlich sollte die Slot-Machine-Rollgeschwindigkeit einstellbar werden.

**Umsetzung (Branch `feature/soft-digit-transitions`, gemerged nach `master`):**
- `digit_fade_math.h` (neu): reine Interpolationsmathematik (`fadeDutyForStep()`), host-testbar ohne Arduino-Framework (`test/digit_fade_math_test.cpp`)
- `digit_fade.ino` (neu): `startDigitFade()`/`updateDigitFade()`/`cancelDigitFade()` – non-blocking State-Machine, dimmt über den vorhandenen HV-Dimmer-Duty ab (`DIGIT_FADE_MIN_DUTY`, ≈5 %), schreibt die Zielziffern bei Minimalhelligkeit um und blendet wieder auf
- `display.ino`: `commitDigits()` als zentrale Stelle für alle Ziffernänderungen (ersetzt das bisherige direkte `nixieWrite()`), `setDisplayTimeSoft()`/`setDisplayDateSoft()` neu
- `NixieClockUltra.ino`: weicher Wechsel im Sekundentakt und beim Zeit/Datum-Übergang verdrahtet; `displayDigits`-Sentinel gegen dunkles Display bei RTC-Kaltstart ergänzt
- Web-UI-Karte „Weicher Ziffernwechsel“ mit zwei Toggles (Sekundentakt/Datum), `/api/softfade` GET+POST
- Slot-Machine-Geschwindigkeit skalierbar gemacht (`slotSpeedPct`, 20–100 %) mit eigenem Web-UI-Regler, `/api/slotspeed` POST
- `cancelDigitFade()` vor `startSlotAnimation()` und beim Eintritt in den Edit-Modus ergänzt (Absicherung gegen Race zwischen laufendem Fade und neuer Display-Aktivität, aus Code-Review)

**Feinschliff nach Hardware-Test:** Fade-Dauer von ursprünglich 100 ms (Sekundentakt) bzw. 200 ms (Datum-Übergang) einheitlich auf 400 ms erhöht.

**Dokumentation:** Bedienungsanleitung, Systemdokumentation (`firmware.md`, `gen_sysdoc.py`, `SUMMARY.md`), Marketing (`gen_werbetext.py`) und Website (`features.html`, `geschichte.html` Phase 9) aktualisiert. Dabei auch einen veralteten Verweis auf das inzwischen entfernte `nixieWriteSafe()` in der Systemdokumentation korrigiert.

---

## Aktueller Stand (2026-07-12)

| Komponente | Status |
|---|---|
| Nixie-Anzeige (6 Röhren, MCP23017) | ✓ funktionsfähig, kein Ghosting |
| DS1302 RTC | ✓ funktionsfähig |
| NeoPixel (10×, Hintergrund + Trennpunkte) | ✓ funktionsfähig |
| Animationen (Rainbow, Static, Pulse) | ✓ funktionsfähig |
| Slot-Machine-Animation + Intervall, Geschwindigkeit einstellbar | ✓ funktionsfähig |
| Weicher Ziffernwechsel (Sekundentakt/Datum-Übergang) | ✓ funktionsfähig, auf Hardware getestet |
| Taster (SET, UP, DOWN, LIGHT) | ✓ funktionsfähig |
| IR-Fernbedienung (7 Funktionen) | ✓ funktionsfähig |
| Web-Interface | ✓ funktionsfähig |
| WiFi STA + AP | ✓ funktionsfähig |
| NTP-Synchronisation | ✓ funktionsfähig |
| mDNS (`nixieclockcs.local`) | ✓ funktionsfähig |
| WiFi-Scan | – entfernt |
| Nacht-Modus HV-Anodendimmung (TLP627, `hv_dimmer.ino`) | ✓ funktionsfähig, auf Hardware getestet |
