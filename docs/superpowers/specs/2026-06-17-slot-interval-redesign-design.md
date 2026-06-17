# Design: Slot-Machine Intervall & Animationstrennung

**Datum:** 2026-06-17  
**Status:** Genehmigt

## Zusammenfassung

Drei Änderungen in einem Schritt:

1. `ANIM_SLOTS` wird aus dem Animationsmodus-Enum entfernt — Hintergrundbeleuchtung und Slot-Machine sind konzeptuell getrennte Einstellungen.
2. Ein neues `SlotInterval`-Enum ersetzt die hart kodierte 10-Sekunden-Logik mit fünf wählbaren Stufen (Aus / 10 Sek / 1 Min / 15 Min / 1 Std).
3. `colonBright`-Default wird von 15 auf 80 angehoben (war kaum sichtbar).

## Datenmodell

### AnimMode (NixieClockUltra.ino)

```cpp
enum AnimMode { ANIM_RAINBOW, ANIM_STATIC, ANIM_PULSE, ANIM_COUNT };
```

`ANIM_SLOTS` (Wert 3) entfällt. `ANIM_COUNT` ist jetzt 3.  
**Migration:** Beim Laden aus NVS — falls gespeicherter Wert `>= ANIM_COUNT`, wird er auf 0 (RAINBOW) zurückgesetzt.

### SlotInterval (NixieClockUltra.ino)

```cpp
enum SlotInterval { SLOT_OFF, SLOT_10S, SLOT_1MIN, SLOT_15MIN, SLOT_1HR };
SlotInterval slotInterval = SLOT_OFF;
```

NVS-Key: `"slotIval"` (uint8_t, Default 0 = OFF).

### colonBright-Default

```cpp
colonBright = prefs.getUChar("colonBright", 80);  // war: 15
```

## Loop-Trigger-Logik (NixieClockUltra.ino)

Ersetzt den bisherigen `if (animMode == ANIM_SLOTS && curSec % 10 == 0)` Block:

```cpp
bool triggerSlot = false;
switch (slotInterval) {
  case SLOT_10S:   triggerSlot = (curSec % 10 == 0); break;
  case SLOT_1MIN:  triggerSlot = (curSec == 0); break;
  case SLOT_15MIN: triggerSlot = (curSec == 0 && curMin % 15 == 0); break;
  case SLOT_1HR:   triggerSlot = (curSec == 0 && curMin == 0); break;
  default: break;
}
if (triggerSlot && !slotActive) startSlotAnimation(curHour, curMin, curSec);
```

`!slotActive` verhindert Neustart einer laufenden Animation bei mehrfachem RTC-Read in derselben Sekunde.

## NeoPixel-Animation (neo_animation.ino)

`case ANIM_SLOTS` wird vollständig entfernt.

Nach dem `switch`-Block, vor `colonStatic`-Override, neuer Slot-Effekt-Block:

```cpp
// Slot-Effekt überschreibt Hintergrundanimation solange slotActive
if (slotActive) {
  uint32_t col = strip.ColorHSV((millis() / 5) % 65536, 255, 255);
  for (int i = 0; i < 6;  i++) strip.setPixelColor(i, scaleColor(col, neoBright));
  for (int i = 6; i < 10; i++) strip.setPixelColor(i, rgbSwap(scaleColor(col, colonBright)));
}
```

Der Regenbogen-Flash-Effekt während der Slot-Animation bleibt damit erhalten und funktioniert mit jedem Hintergrundmodus.

## Web UI (web_server.ino)

### Animations-Select

Option "Slot-Machine" (value=3) wird entfernt. Nur noch:
- 0 = Rainbow
- 1 = Statisch Warmweiß
- 2 = Puls

### Karte "Slot-Animation"

Neue Zeile mit Intervall-Dropdown (oberhalb des manuellen Buttons):

```html
<div class="row">
  <label>Automatisch</label>
  <select id="slotIval" onchange="setSlotInterval()">
    <option value="0">Aus</option>
    <option value="1">10 Sek</option>
    <option value="2">1 Min</option>
    <option value="3">15 Min</option>
    <option value="4">1 Std</option>
  </select>
</div>
```

### API

**Neuer Endpoint** `POST /api/slotinterval` mit Body `{interval: 0–4}`:
- Setzt `slotInterval`, speichert in NVS als `"slotIval"`.

**Erweiterung** `GET /api/status`:
- Liefert zusätzlich `"slotIval": <0–4>`.

**`refreshClock()`** liest `d.slotIval` und setzt `document.getElementById('slotIval').value`.

### JavaScript

```js
async function setSlotInterval() {
  let v = parseInt(document.getElementById('slotIval').value);
  await api('/api/slotinterval', {interval: v});
}
```

## Trennpunkt-Bug

Die Trennpunkt-Pixel (6–9) reagieren nicht auf Software-Einstellungen — auch nicht bei maximaler Helligkeit und aktivem `colonStatic`. Das deutet auf einen Hardware-Fehler hin (THT WS2812 vermutlich falsch herum eingelötet: VCC↔GND oder DIN↔DOUT vertauscht). Die Software-seitige Anhebung des Defaults auf 80 verbessert die Sichtbarkeit nach der Hardware-Korrektur.

## Betroffene Dateien

| Datei | Änderungen |
|---|---|
| `NixieClockUltra.ino` | `AnimMode` kürzen, `SlotInterval` enum + Variable, NVS-Laden/-Speichern, Loop-Trigger-Logik, colonBright-Default |
| `neo_animation.ino` | `ANIM_SLOTS` case entfernen, Slot-Override-Block hinzufügen |
| `web_server.ino` | HTML (Select + Dropdown), `/api/status` Response, neuer `/api/slotinterval` Endpoint, JS `setSlotInterval()` + `refreshClock()` |
