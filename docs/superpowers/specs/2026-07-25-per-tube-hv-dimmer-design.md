# Design: Pro-Röhre-HV-Dimmer für weichen Ziffernwechsel

## Kontext

Die IN-12-Leiterkarte wird so umgebaut, dass jede der 6 Nixie-Anoden optional
einen eigenen TLP627-Optokoppler-Schalter bekommt, statt sich (wie heute) einen
einzigen gemeinsamen Schalter (`HV_SWITCH_PIN`, GPIO7) zu teilen. Die neue
Bestückung ist optional — bestehende, unmodifizierte Uhren müssen sich nach
dem Firmware-Update exakt wie zuvor verhalten.

**Heutiges Verhalten:** Beim weichen Ziffernwechsel (`digit_fade.ino`) wird die
gemeinsame Anodenspannung aller 6 Röhren ab- und wieder aufgeblendet — auch für
Röhren, deren Ziffer sich gar nicht ändert (z. B. blinken beim Sekundentakt
Stunden/Minuten sichtbar mit).

**Ziel:** Mit den 6 separaten Schaltern soll nur die Röhre abblenden, deren
Ziffer sich tatsächlich ändert; unveränderte Röhren bleiben durchgehend hell.
Der weiche Übergang soll außerdem auch im gedimmten Nacht-Modus laufen (aktuell
nur bei `NIGHT_NORMAL`), dort zwischen Minimal-Duty und der reduzierten
Ziel-Helligkeit (`hvDimPct`) statt bis 100 %.

Ein Compile-Time-`#define` schaltet zwischen Alt- und Neu-Verhalten, da die
Hardware optional bestückt wird.

## 1. Pin-Belegung

Neue Defines in `NixieClockUltra.ino`, nur wirksam wenn `HV_PER_TUBE_DIMMER`
aktiviert ist:

```cpp
// Bei bestücktem Pro-Röhre-HV-Schalter (6× TLP627) einkommentieren:
// #define HV_PER_TUBE_DIMMER
#define HV_TUBE_PIN_0  38   // Stundenzehner (HZ)
#define HV_TUBE_PIN_1  47   // Stundeneiner  (HE)
#define HV_TUBE_PIN_2  15   // Minutenzehner (MZ)
#define HV_TUBE_PIN_3  16   // Minuteneiner  (ME)
#define HV_TUBE_PIN_4  17   // Sekundenzehner(SZ)
#define HV_TUBE_PIN_5  18   // Sekundeneiner (SE)
```

Ausgewählt unter Vermeidung von: Strapping-Pins (0, 3, 45, 46), nativem USB
(19, 20), JTAG (39–42), PSRAM/Octal-Flash-Bereich (26–37), sowie GPIO 1 und 14
(vom Nutzer für einen externen USB-Treiber reserviert) und allen bereits
belegten Pins (RTC 2/4/5, LDR 6, HV_SWITCH_PIN 7, I²C 8/9, Buttons 10–13,
NeoPixel 21, IR 48).

**Offener Punkt:** Vor dem Verdrahten gegenchecken, dass die konkrete
WROOM-1-Variante keine PSRAM/Flash-Sonderfunktion auf einem der 6 vorgeschlagenen
Pins hat.

## 2. Hardware-Abstraktion (`hv_dimmer.ino`)

Zwei API-Funktionen statt bisher einer, mit `#ifdef`-Zweig komplett isoliert in
dieser einen Datei:

```cpp
#ifdef HV_PER_TUBE_DIMMER
static const uint8_t hvTubePin[6] = {
  HV_TUBE_PIN_0, HV_TUBE_PIN_1, HV_TUBE_PIN_2,
  HV_TUBE_PIN_3, HV_TUBE_PIN_4, HV_TUBE_PIN_5
};

void hvDimmerInit() {
  for (uint8_t i = 0; i < 6; i++) {
    ledcAttach(hvTubePin[i], HV_PWM_FREQ_HZ, 8);
    ledcWrite(hvTubePin[i], 255);
  }
}
void hvDimmerSetDutyAll(uint8_t duty) {
  for (uint8_t i = 0; i < 6; i++) ledcWrite(hvTubePin[i], duty);
}
void hvDimmerSetDutyTube(uint8_t tube, uint8_t duty) {
  ledcWrite(hvTubePin[tube], duty);
}

#else  // heutige Hardware: ein gemeinsamer Schalter
void hvDimmerInit() {
  ledcAttach(HV_SWITCH_PIN, HV_PWM_FREQ_HZ, 8);
  ledcWrite(HV_SWITCH_PIN, 255);
}
void hvDimmerSetDutyAll(uint8_t duty) { ledcWrite(HV_SWITCH_PIN, duty); }
void hvDimmerSetDutyTube(uint8_t /*tube*/, uint8_t duty) { ledcWrite(HV_SWITCH_PIN, duty); }
#endif
```

Im Alt-Zweig ignoriert `hvDimmerSetDutyTube()` den Röhrenindex und steuert den
gemeinsamen Schalter an — dadurch bleibt das heutige Verhalten (alle dimmen
gemeinsam bei jeder Änderung) automatisch erhalten, ohne dass `digit_fade.ino`
selbst ein `#ifdef` braucht.

LEDC-Kanäle: 1 (Alt-Zweig) bzw. 6 (Neu-Zweig), beide Zweige schließen sich
gegenseitig aus — bleibt klar unter dem Limit von 8 Kanälen des ESP32-S3.

## 3. Fade-Logik (`digit_fade.ino` + `digit_fade_math.h`)

Neue reine, host-testbare Helfer-Funktion in `digit_fade_math.h`:

```cpp
inline uint8_t computeChangedMask(const uint8_t oldDigits[6], const uint8_t newDigits[6]) {
  uint8_t mask = 0;
  for (uint8_t i = 0; i < 6; i++) if (oldDigits[i] != newDigits[i]) mask |= (1 << i);
  return mask;
}
```

`display.ino`s `commitDigits()` berechnet die Maske **vor** dem Überschreiben
von `displayDigits` und reicht sie an `startDigitFade(newDigits, mask, fadeMs)`
durch.

Fade-Zustand bleibt ein **gemeinsamer** Schritt-Zähler (kein Bedarf an 6
unabhängigen Timern, da alle aktuellen Trigger — Sekundentakt, manuelles
Setzen, Datumsanzeige — Ziffern immer gemeinsam committen). Zusätzlich zum
bisherigen Zustand:

- `fadeMask` — welche Röhren-Bits betroffen sind
- `fadeMaxDuty` — Ziel-Helligkeit oben, einmalig beim Fade-Start ermittelt:
  `255` bei `NIGHT_NORMAL`, `hvDimPct * 255 / 100` bei `NIGHT_DIM`

`updateDigitFade()`: iteriert pro Schritt nur über die in `fadeMask` gesetzten
Bits und ruft `hvDimmerSetDutyTube(i, duty)` auf — unveränderte Röhren werden
nie angefasst.

**Verhaltensänderung:** Die Trigger-Bedingung in `commitDigits()` wechselt von
`nightState == NIGHT_NORMAL` zu `nightState != NIGHT_DARK`, damit der weiche
Wechsel jetzt auch im Dimm-Modus läuft. In `NIGHT_DARK` bleibt es beim harten
Umschalten (Anodenspannung ist ohnehin aus, ein Fade wäre unsichtbar).

`cancelDigitFade()` / `fadeFinishImmediately()`: setzen beim vorzeitigen
Abbruch nur die Röhren aus der aktuellen `fadeMask` auf `fadeMaxDuty` zurück
(nicht mehr pauschal alle 6) — verhindert ein falsches Aufblitzen auf 100 % bei
einem Nacht-Modus-Wechsel während eines laufenden Fades.

## 4. Anpassungen bestehender Aufrufstellen

Reines Rename `hvDimmerSetDuty(...)` → `hvDimmerSetDutyAll(...)`, keine
Logikänderung, an:

- `NixieClockUltra.ino` — Nacht-Zustands-Switch (~Zeile 413–415)
- `tube_test.ino` — Röhrentest erzwingt volle Helligkeit (~Zeile 17) und eigener
  Nacht-Zustands-Switch (~Zeile 43–45)
- `web_server.ino` — Live-Update von `hvDimPct` während `NIGHT_DIM` (~Zeile 746)

## 5. Test-Plan

- `test/digit_fade_math_test.cpp` erweitert um Fälle für `computeChangedMask()`:
  keine Änderung (`mask == 0`), eine Röhre geändert, alle 6 geändert, Bit-Position
  entspricht Röhren-Index (0 = HZ … 5 = SE).
- Bestehende `fadeDutyForStep()`-Tests bleiben unverändert gültig.
- Hardware-Test auf echtem Board:
  1. `HV_PER_TUBE_DIMMER` **aus**: Sekunden-/Minuten-/Stundenwechsel, Nacht-Modus,
     Röhrentest durchklicken — nichts darf sich gegenüber heute verändert haben.
  2. `HV_PER_TUBE_DIMMER` **an** (mit bestückter Hardware): nur wechselnde
     Ziffern blenden ab, unveränderte bleiben durchgehend hell; Fade funktioniert
     auch im Dimm-Modus.

## 6. Doku-Updates

- `docs/system/hardware.md` — neuer Abschnitt zu den 6 optionalen TLP627 in der
  Pin-Belegungstabelle und bei „Nixie Display Board"; Hinweis auf Optionalität.
- `docs/system/firmware.md` — Modulübersicht (`hv_dimmer.ino`-Zeile), neue
  Defines, `hvDimmerSetDutyAll()`/`hvDimmerSetDutyTube()` statt
  `hvDimmerSetDuty()`, Nacht-Modus-Logik-Abschnitt.
- `docs/manual/nixie-clock-ultra-bedienungsanleitung.html` — Hinweis im
  Hardware-Kapitel, dass der weiche Ziffernwechsel bei bestückter
  Pro-Röhre-Option jetzt nur die wechselnde Ziffer abblendet (ohne
  Bauteilnamen im Fließtext, gemäß bisherigem Schreibstil).
- `docs/website/aufbau.html` / `docs/website/features.html` — TLP627-Erwähnung
  um „optional pro Röhre nachrüstbar" ergänzen, Feature-Beschreibung des weichen
  Ziffernwechsels aktualisieren.

## Out of Scope

- Keine Laufzeit-Umschaltung (NVS/Web-UI) zwischen Alt- und Neu-Hardware —
  bewusst Compile-Time-`#define`, da die Hardware-Variante pro gebauter Uhr fest
  ist.
- Keine unabhängigen Timer pro Röhre — aktuell kein Anwendungsfall, der
  zeitlich versetzte Fades einzelner Röhren bräuchte.
