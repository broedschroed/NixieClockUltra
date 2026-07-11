# NixieClockUltra – Weicher Ziffernwechsel & Slot-Machine-Geschwindigkeit

**Datum:** 2026-07-11
**Status:** Genehmigt
**Branch:** neu anzulegen von `master` (nach Commit `0516472`)

---

## Überblick

Zwei unabhängige, kleine Firmware-Erweiterungen, gemeinsam auf einem
Branch umgesetzt:

1. **Weicher Ziffernwechsel** – nutzt den bestehenden HV-Dimmer
   (`hv_dimmer.ino`, TLP627 auf `HV_SWITCH_PIN`) für einen kurzen
   Crossfade der gesamten Anodenspannung beim Wechsel zwischen
   Zeit- und Datumsanzeige, sowie optional bei jedem Sekundentakt.
2. **Slot-Machine-Geschwindigkeit** – ein Web-UI-Regler skaliert die
   Roll- und Stopp-Zeiten der bestehenden Slot-Animation
   (`updateSlotAnimation()`).

Beide Features sind per Web-UI ein-/ausschaltbar bzw. einstellbar und
persistieren ihre Einstellung in NVS (`Preferences`), analog zu allen
bestehenden Einstellungen im Projekt.

---

## 1. Weicher Ziffernwechsel

### 1.1 Scope

- Betrifft **ausschließlich** zwei Übergänge, jeweils per eigenem
  Web-UI-Toggle:
  - **Sekundentakt** (`softFadeSecondEnabled`, NVS-Key `sfSecEn`,
    Default `false`): jeder reguläre Zeit-Update-Aufruf
    (`curSec != lastSec`, außerhalb Slot-Animation/Datumsanzeige).
    Fade-Dauer: **100ms**.
  - **Zeit↔Datum-Wechsel** (`softFadeDateEnabled`, NVS-Key `sfDateEn`,
    Default `false`): Übergang Slot-Ende→Datumsanzeige und
    Datumsanzeige-Ende→Zeit. Fade-Dauer: **200ms**.
- **Nicht** betroffen: Slot-Machine-Animation selbst (rollt bereits
  schneller als eine Fade-Dauer), Editiermodus-Blinken
  (`buttons.ino`, ruft `nixieWrite()` direkt), Initialisierung/NTP-Sync.
- Der Effekt ist **nur im Zustand `NIGHT_NORMAL`** aktiv (volle
  Anodenspannung). Im Dimm-/Dunkel-Zustand bleibt der Ziffernwechsel
  hart, wie bisher – wird zentral erzwungen, nicht nur an den
  Aufrufstellen geprüft (siehe 1.3).
- Minimalhelligkeit während des Fades: **5%** Duty (≈13 von 255),
  fest codiert (`#define DIGIT_FADE_MIN_DUTY 13`).

### 1.2 Warum ein globaler Fade (kein Pro-Röhre-Crossfade)

Der TLP627 dimmt die Anodenspannung aller 6 Röhren gemeinsam über
einen einzigen LEDC-Kanal – es gibt keine Pro-Röhre-Dimmung. Ein Fade
betrifft daher immer das komplette Display, nicht nur die sich
ändernde Ziffer. Eine echte Pro-Röhre-Überblendung (zwei Kathoden
gleichzeitig an einer Röhre, Duty-Cycle-Verschiebung) wurde als
Alternative erwogen, aber verworfen: sie erfordert eine Prüfung, ob
der Anoden-Vorwiderstand doppelten Kathodenstrom verträgt, und
nähert sich konzeptionell der Mehrfach-Ansteuerung an, die beim
Anti-Ghosting-Umbau (Mai/Juni 2026) bewusst vermieden wurde. Nicht
Teil dieser Spec.

### 1.3 Neues Modul `digit_fade.ino`

Non-blocking State-Machine, angetrieben von `updateDigitFade()` in
`loop()` (wie `updateSlotAnimation()`):

```cpp
#define DIGIT_FADE_MIN_DUTY   13   // ~5% von 255
#define DIGIT_FADE_STEP_MS    5    // Schrittintervall

enum FadeState { FADE_IDLE, FADE_DOWN, FADE_UP };
static FadeState fadeState = FADE_IDLE;
static uint8_t   fadeTargetDigits[6];
static uint32_t  fadeLastStepMs;
static uint8_t   fadeStepsTotal;
static uint8_t   fadeStepsDone;

// Schließt einen laufenden Fade sofort ab (Zielziffern schreiben,
// Duty auf 255). Wird aufgerufen, wenn ein neuer Fade angefordert
// wird während einer noch läuft, oder wenn sich nightState während
// eines Fades ändert (verhindert Race mit der Nacht-Modus-Duty-
// Steuerung in NixieClockUltra.ino).
static void fadeFinishImmediately();

void startDigitFade(uint8_t newDigits[6], uint16_t fadeMs);
void updateDigitFade();   // in loop() aufrufen
void cancelDigitFade();   // von NixieClockUltra.ino bei nightState-Wechsel aufgerufen
```

**Ablauf pro Fade:**
1. `fadeStepsTotal = max(1, (fadeMs / 2) / DIGIT_FADE_STEP_MS)`
   (100ms → 10 Schritte runter + 10 rauf; 200ms → 20 + 20).
2. `FADE_DOWN`: pro Schritt Duty linear von 255 auf 13 interpolieren,
   `hvDimmerSetDuty()`.
3. Bei letztem Runter-Schritt: `nixieWrite(fadeTargetDigits)` (Ziffer
   bei Minimalhelligkeit umschalten), Zustand → `FADE_UP`.
4. `FADE_UP`: Duty linear von 13 zurück auf 255, letzter Schritt setzt
   exakt 255 und Zustand → `FADE_IDLE`.

### 1.4 Integration `display.ino`

`setDisplayTime()`/`setDisplayDate()` bleiben unverändert als
"sofort hart schreiben"-Funktionen (Startup, Editiermodus-Bestätigung
etc. nutzen weiterhin genau diese, unverändertes Verhalten). Beide
Funktionen und ihre neuen Soft-Varianten laufen über eine gemeinsame
interne Funktion:

```cpp
static void commitDigits(uint8_t newDigits[6], uint16_t fadeMs) {
  bool changed = memcmp(newDigits, displayDigits, 6) != 0;
  memcpy(displayDigits, newDigits, 6);
  if (!changed) return;
  if (fadeMs > 0 && nightState == NIGHT_NORMAL) {
    startDigitFade(newDigits, fadeMs);
  } else {
    nixieWrite(displayDigits);
  }
}

void setDisplayTime(uint8_t h, uint8_t m, uint8_t s);              // commitDigits(d, 0)
void setDisplayTimeSoft(uint8_t h, uint8_t m, uint8_t s, uint16_t fadeMs);
void setDisplayDate();                                              // commitDigits(d, 0)
void setDisplayDateSoft(uint16_t fadeMs);
```

### 1.5 Aufrufstellen `NixieClockUltra.ino`

Drei Stellen in `loop()` werden angepasst (fadeMs jeweils `0`, wenn
das zugehörige Toggle aus ist – identisch zum heutigen Verhalten):

- Sekundentakt (aktuell `setDisplayTime(curHour, curMin, curSec)`
  im `if (!slotActive && !dateShowActive)`-Zweig) →
  `setDisplayTimeSoft(curHour, curMin, curSec, softFadeSecondEnabled ? 100 : 0)`.
- Slot-Ende → Datumsanzeige (aktuell `setDisplayDate()`) →
  `setDisplayDateSoft(softFadeDateEnabled ? 200 : 0)`.
- Datumsanzeige-Ende → Zeit (aktuell `setDisplayTime(curHour, curMin, curSec)`) →
  `setDisplayTimeSoft(curHour, curMin, curSec, softFadeDateEnabled ? 200 : 0)`.

Zusätzlich: im bestehenden `nightState != prevNightState`-Block
(vor dem Setzen der nacht-modus-abhängigen Duty) wird
`cancelDigitFade()` aufgerufen, damit ein laufender Fade nicht mit
der Nacht-Modus-Duty-Steuerung kollidiert.

### 1.6 Web-UI & NVS

Neue Karte "Weicher Ziffernwechsel" mit zwei Checkboxen
("Bei Sekundentakt", "Bei Zeit/Datum-Wechsel"). Neuer Endpunkt
`/api/softfade`, GET (Status) + POST (Toggle setzen), Muster analog
zu `/api/nightmode`:

```cpp
server.on("/api/softfade", HTTP_GET, [](AsyncWebServerRequest *req) {
  // JSON: { sec: bool, date: bool }
});
server.on("/api/softfade", HTTP_POST, ...,
  // { sec?: bool, date?: bool } – nur übergebene Felder ändern
  // prefs.putBool("sfSecEn", ...) / prefs.putBool("sfDateEn", ...)
);
```

Laden beim Boot in `setup()` (analog zu `hvDimPct`):
```cpp
softFadeSecondEnabled = prefs.getBool("sfSecEn", false);
softFadeDateEnabled   = prefs.getBool("sfDateEn", false);
```

---

## 2. Slot-Machine-Geschwindigkeit

### 2.1 Scope

Neuer Regler in der Web-UI (bestehende Animations-Karte, neben
"Slot-Intervall"), Bereich **20%–100%**, Default **100%** (= aktuelles
Verhalten unverändert). Werte unter 100% verlangsamen die Animation,
keine Beschleunigung über die heutige Geschwindigkeit hinaus.

### 2.2 Variable & Persistenz

```cpp
uint8_t slotSpeedPct = 100;   // 20–100, NVS-Key "slotSpeed"
```

Laden in `setup()`:
```cpp
slotSpeedPct = (uint8_t)constrain((int)prefs.getUChar("slotSpeed", 100), 20, 100);
```

### 2.3 Anwendung in `display.ino`

Die beiden heute fest codierten Timing-Konstanten in
`updateSlotAnimation()` (Roll-Intervall 60ms, Stopp-Zeitplan
`600 + i*180`ms) werden **einmalig bei Animationsstart** skaliert und
zwischengespeichert – nicht pro Frame neu berechnet, damit ein
Reglerwechsel während einer laufenden Animation diese nicht
"ruckeln" lässt. Die neue Geschwindigkeit greift erst bei der
nächsten Animation.

```cpp
uint16_t slotRollIntervalMs;   // = 60 * 100 / slotSpeedPct
uint16_t slotStopMs[6];        // = (600 + i*180) * 100 / slotSpeedPct

void startSlotAnimation(uint8_t h, uint8_t m, uint8_t s) {
  slotRollIntervalMs = 60UL * 100 / slotSpeedPct;
  for (int i = 0; i < 6; i++) slotStopMs[i] = (600UL + i * 180) * 100 / slotSpeedPct;
  // ... bestehender Rest unverändert (slotTarget/slotCurrent/slotActive/slotStartMs)
}

void updateSlotAnimation() {
  // ... elapsed wie bisher
  // stopTime = slotStopMs[i]  statt  600 + i*180
  // Rollbedingung: (millis() % slotRollIntervalMs) < (slotRollIntervalMs / 2)
  //   statt  (millis() % 60) < 30
}
```

Bei 20%: Roll-Intervall 300ms, letzte Röhre stoppt nach 7,5s statt
1,5s. Bei 100%: unverändert.

### 2.4 Web-UI & NVS

Neuer Regler (`<input type="range" min="20" max="100">`), Endpunkt
`/api/slotspeed` (POST `{val}`), Muster analog zu `/api/neobright`.
Aktueller Wert zusätzlich in `/api/status` (`doc["slotSpeed"]`),
damit der Regler beim Laden der Seite korrekt initialisiert wird.

```cpp
server.on("/api/slotspeed", HTTP_POST, [](AsyncWebServerRequest *req){}, ...,
  [](AsyncWebServerRequest *req, uint8_t *data, size_t len, ...) {
    // JSON {val: int}
    slotSpeedPct = (uint8_t)constrain(doc["val"].as<int>(), 20, 100);
    prefs.putUChar("slotSpeed", slotSpeedPct);
    req->send(200, "application/json", "{\"ok\":true}");
  });
```

---

## 3. Testplan (manuell, auf Hardware)

- **Weicher Ziffernwechsel Sekundentakt:** Toggle an, `NIGHT_NORMAL`,
  beobachten dass jede Sekunde ein kurzes Abdunkeln/Aufhellen aller
  Röhren sichtbar ist (~100ms). Toggle aus → sofortiger Wechsel wie
  bisher.
- **Weicher Ziffernwechsel Zeit/Datum:** Slot-Intervall auf `SLOT_10S`
  stellen, warten bis Slot-Animation endet und Datumsanzeige startet
  → 200ms-Fade beobachten; nach 5s Rückwechsel zur Zeit → ebenfalls
  200ms-Fade.
- **Interaktion mit Nacht-Modus:** Während eines Sekundentakt-Fades
  in den Dimm-Zustand wechseln (LDR abdecken oder Zeitbereich
  triggern) → kein hängender Zwischenzustand, Duty landet korrekt auf
  Dimm-Helligkeit.
- **Slot-Speed:** Regler auf 20% stellen, Slot-Animation auslösen
  (manueller Trigger-Button in Web-UI) → deutlich langsameres Rollen,
  letzte Röhre stoppt nach ~7,5s. Regler zurück auf 100% → Verhalten
  wie vor dieser Änderung.
- Reboot nach jedem Test: alle vier neuen Einstellungen
  (`sfSecEn`, `sfDateEn`, `slotSpeed`, und ggf. Regler-Position in der
  UI) bleiben erhalten (NVS-Persistenz).

---

## Nicht Teil dieser Änderung

- Pro-Röhre-Crossfade (Dual-Kathoden-Technik, siehe 1.2).
- Konfigurierbarkeit von Fade-Dauer (100/200ms) oder Minimalhelligkeit
  (5%) über die Web-UI – fest codiert.
- Geschwindigkeiten über 100% (schneller als heute) für die
  Slot-Animation.
