# NixieClockUltra – Hardware-Dimmung der Anodenspannung (TLP627)

**Datum:** 2026-07-07
**Status:** Genehmigt
**Branch:** `feature/hv-anode-dimming` (abgezweigt von `master` nach Commit `a50de45`)

---

## Überblick

Ersatz der bisherigen Software-PWM-Dimmung der Nixie-Röhren (zyklisches
Kathoden-Blanking über die MCP23017, nur im Nacht-Modus aktiv) durch
echtes Hardware-PWM auf der Anodenspannung. Ein neuer GPIO steuert über
einen TLP627-Optokoppler direkt die Anodenspannung – der
Photodarlington-Ausgang des TLP627 ist laut Datenblatt
(„Recommended Operating Conditions“, Supply Voltage max. 200V) für die
~170V-Anodenspannung ausreichend spannungsfest, ein zusätzlicher
HV-Schalter ist nicht nötig.

**Scope:** Ausschließlich Ersatz von `NIGHT_DIM`/`NIGHT_DARK` für die
Nixie-Röhren. Volle Helligkeit (`NIGHT_NORMAL`) bleibt fest an. Die
NeoPixel-Dimmung im Nacht-Modus (`NIGHT_DIM_NEO_PCT`, per-Pixel
`scaleColor()`) ist unverändert und nicht Teil dieser Änderung.

Bereits in der Nacht-Modus-Spec vom 2026-06-21 als Option B vermerkt
(„Hardware-PWM für Nixie – kann später nachgerüstet werden“).

---

## 1. Hardware-Interface

### 1.1 GPIO

```cpp
#define HV_SWITCH_PIN  7   // unbenutzt, kein Strapping-Pin, neben LDR (GPIO6)
```

### 1.2 Signal-Vertrag

- **Aktiv-Pegel:** HIGH-Duty = Anode an (nicht invertiert).
- **PWM:** ESP32-S3 LEDC-Hardware-Peripherie, 8-Bit-Auflösung (Duty 0–255).
- **Frequenz:** 200 Hz (`#define HV_PWM_FREQ_HZ 200`) – deutlich über der
  Flimmerschwelle, aber im Bereich, den ein TLP627-getriebener
  HV-Schalter (Photodarlington-Schaltzeiten typisch einige zehn µs)
  sauber nachbilden kann. Bei Bedarf nach Hardware-Aufbau per
  Oszilloskop verifizieren und `#define` anpassen.

### 1.3 Downstream-Hinweis (nicht Teil dieser Firmware-Änderung)

Der TLP627-Photodarlington-Ausgang liegt direkt in Reihe mit der
Anodenspannung (kein zusätzlicher HV-Schalter nötig). Zwei Punkte, die
bei der analogen Dimensionierung zu beachten sind (Hardware-Sache des
Users, nicht Gegenstand dieser Spec):
- **Collector Current max. 120mA** – Summe des Anodenstroms aller 6
  Röhren muss darunter bleiben.
- **Forward Current typ. 16mA** – bestimmt den Vorwiderstand von
  GPIO7 zur TLP627-LED.

---

## 2. Neues Modul `hv_dimmer.ino`

```cpp
void hvDimmerInit();                      // ledcAttach(HV_SWITCH_PIN, HV_PWM_FREQ_HZ, 8) + volle Helligkeit
void hvDimmerSetDuty(uint8_t duty0to255); // ledcWrite(HV_SWITCH_PIN, duty0to255)
```

Kapselt die LEDC-Details (Core-3.x-API: `ledcAttach(pin, freq, resolution)`
statt der alten `ledcSetup()`/`ledcAttachPin()`-Kombination) an einer
Stelle.

`hvDimmerInit()` wird in `setup()` nach `strip.begin()`-Block aufgerufen
und setzt initial volle Helligkeit (Duty 255).

---

## 3. Integration in Nacht-Modus (`NixieClockUltra.ino`)

### 3.1 Entfernter Code

Der komplette bisherige Software-PWM-Block in `loop()` entfällt:
- `nixiePwmOn` (Variable + Logik)
- `lastPwmToggle`
- zyklisches `nixieWrite(blank)` / `nixieWrite(displayDigits)`-Umschalten
- `#define NIGHT_DIM_PWM_PERIOD` und `#define NIGHT_DIM_DUTY_PCT`

Die Röhren zeigen im Nacht-Modus durchgehend die echten Ziffern
(`nixieWrite(displayDigits)` wie tagsüber) – ohne Anodenspannung
leuchtet nichts, ein Blank-Schreiben ist überflüssig.

### 3.2 Neue Logik beim Zustandswechsel

```cpp
if (nightState != prevNightState) {
  prevNightState = nightState;
  switch (nightState) {
    case NIGHT_DARK:   hvDimmerSetDuty(0);                     break;
    case NIGHT_DIM:    hvDimmerSetDuty(hvDimPct * 255 / 100);  break;
    case NIGHT_NORMAL: hvDimmerSetDuty(255);                   break;
  }
}
```

### 3.3 Neue Konfigurationsvariable

```cpp
uint8_t hvDimPct = 25;   // Dimm-Helligkeit in %, NVS-Key "hvDimPct", Range 5–95
```

Ersetzt die feste `NIGHT_DIM_DUTY_PCT`-Konstante. Wird zusätzlich neu
angewendet, wenn der Slider im Web-UI während `nightState == NIGHT_DIM`
verändert wird (kein Zustandswechsel nötig).

---

## 4. Web-UI (`web_server.ino`)

### 4.1 Karte „🌙 Nacht-Modus“ – neues Element

Neuer Slider zwischen „Modus“-Dropdown und „Lichtsensor aktiv“:

```html
<div class="row"><label>Dimm-Helligkeit</label>
  <input type="range" id="hvDimPct" min="5" max="95" value="25"
    oninput="document.getElementById('hvDimPctVal').textContent=this.value">
  <span id="hvDimPctVal">25</span>%</div>
```

Wird wie die übrigen Nacht-Modus-Felder beim Klick auf „Übernehmen“ per
`saveNightMode()` mitgeschickt.

### 4.2 API-Erweiterung

**`GET /api/nightmode`** – zusätzliches Feld `"hvDimPct": 25`.

**`POST /api/nightmode`** – akzeptiert zusätzlich `hvDimPct` (uint8,
5–95, `constrain()`), persistiert via `prefs.putUChar("hvDimPct", ...)`.

### 4.3 NVS-Keys (Namespace `"nixie"`, Ergänzung zur bestehenden Tabelle)

| Key       | Typ   | Bedeutung                          |
|-----------|-------|-------------------------------------|
| `hvDimPct`| uint8 | Röhren-Dimm-Helligkeit in % (5–95)  |

---

## 5. Dateien-Übersicht

| Datei                 | Änderung                                                        |
|------------------------|------------------------------------------------------------------|
| `hv_dimmer.ino`        | **Neu** – LEDC-Kapselung (`hvDimmerInit()`, `hvDimmerSetDuty()`) |
| `NixieClockUltra.ino`  | `HV_SWITCH_PIN`/`HV_PWM_FREQ_HZ` #defines, `hvDimPct` Global + NVS-Laden, `hvDimmerInit()` in `setup()`, Software-PWM-Block in `loop()` entfernt und durch `hvDimmerSetDuty()`-Aufrufe ersetzt |
| `web_server.ino`       | Neuer Slider in Nacht-Modus-Karte, `/api/nightmode` GET+POST um `hvDimPct` erweitert |

---

## 6. Verifikation

Kein Compiler/Hardware-Zugriff in dieser Umgebung verfügbar – die
Implementierung ist code-vollständig, aber ungetestet abzunehmen.
Manueller Testablauf nach Hardware-Aufbau:

1. **Isoliert (ohne HV):** GPIO7-Signal mit Oszilloskop/Logic-Analyzer
   prüfen – 200 Hz, korrekter Duty bei jedem `nightState`-Übergang
   (0 / `hvDimPct` / 255) und bei Slider-Änderung.
2. **TLP627-Stufe separat:** LED-Vorwärtsstrom und sauberes Schalten
   der Anodenspannung am TLP627-Ausgang messen, bevor die Röhren
   dranhängen.
3. **Am fertigen Gerät:** Nacht-Modus-Zeitfenster testweise aktivieren,
   `hvDimPct`-Slider 5–95% durchfahren → sichtbares, flackerfreies
   Dimmen. Dunkel-Modus prüfen → Röhren komplett aus, kein Nachglühen.
4. **Regressionscheck:** `NIGHT_NORMAL` weiterhin volle, stabile
   Helligkeit; Taster/IR-Bedienung unverändert; NeoPixel-Nachtdimmung
   (unverändert im Code) weiterhin korrekt.

---

## 7. Nicht im Scope

- Mehrstufige/stufenlose Röhren-Helligkeit außerhalb des Nacht-Modus
  (der Web-UI-„Nixie-Helligkeit“-Dropdown bleibt wirkungslos für die
  Röhren, wie bisher).
- Änderungen an der NeoPixel-Dimmung.
- Dimensionierung des LED-Vorwiderstands am GPIO7/TLP627 sowie Prüfung
  des Anodenstroms gegen die 120mA-Collector-Current-Grenze – reine
  Hardware-Aufgabe des Users.
- Nicht-blockierendes WiFi-Connect (separates, bereits besprochenes Thema).
