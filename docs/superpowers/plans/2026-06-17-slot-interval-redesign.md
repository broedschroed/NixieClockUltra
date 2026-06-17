# Slot-Machine Intervall & Animationstrennung — Implementierungsplan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `ANIM_SLOTS` aus dem Animationsmodus entfernen, Slot-Machine-Intervall als eigenständige Einstellung (Aus/10Sek/1Min/15Min/1Std) einführen, colonBright-Default von 15 auf 80 anheben.

**Architecture:** `SlotInterval`-Enum als neue globale Variable neben `animMode`. NeoPixel-Slot-Effekt als Override-Schicht nach dem Animations-Switch. Web-UI bekommt ein separates Intervall-Dropdown in der Slot-Karte.

**Tech Stack:** Arduino/ESP32-S3, Adafruit NeoPixel, ESPAsyncWebServer, ArduinoJson, Preferences (NVS)

**Compile-Befehl:** Arduino IDE → Sketch → Verify/Compile (Strg+R). Kein Unit-Test-Framework verfügbar; Verifikation erfolgt durch Compile-Check + Hardware-Test nach dem Flashen.

---

## Betroffene Dateien

| Datei | Was ändert sich |
|---|---|
| `NixieClockUltra.ino` | `AnimMode` kürzen, `SlotInterval` enum + Variable, NVS-Laden, Loop-Trigger, colonBright-Default |
| `neo_animation.ino` | `ANIM_SLOTS` case entfernen, Slot-Override-Block hinzufügen |
| `web_server.ino` | HTML: Slot-Card + Animation-Select; C++: `/api/status`, neuer `/api/slotinterval` Endpoint; JS: `setSlotInterval()`, `refreshClock()` |

---

## Task 1: Datenmodell & Loop-Logik (NixieClockUltra.ino)

**Files:**
- Modify: `NixieClockUltra.ino`

- [ ] **Schritt 1: AnimMode kürzen**

Zeile 121 — `ANIM_SLOTS` entfernen:

```cpp
// ALT:
enum AnimMode { ANIM_RAINBOW, ANIM_STATIC, ANIM_PULSE, ANIM_SLOTS, ANIM_COUNT };

// NEU:
enum AnimMode { ANIM_RAINBOW, ANIM_STATIC, ANIM_PULSE, ANIM_COUNT };
```

- [ ] **Schritt 2: SlotInterval-Enum und Variable einfügen**

Direkt nach der `AnimMode`-Zeile (nach Zeile 122) einfügen:

```cpp
enum SlotInterval { SLOT_OFF, SLOT_10S, SLOT_1MIN, SLOT_15MIN, SLOT_1HR };
SlotInterval slotInterval = SLOT_OFF;
```

- [ ] **Schritt 3: NVS-Laden in setup() anpassen**

Zeile 240 — `animMode`-Laden ersetzen und `slotInterval` + Migration hinzufügen:

```cpp
// ALT:
animMode    = (AnimMode)prefs.getUChar("animMode", 0);

// NEU:
uint8_t savedAnim = prefs.getUChar("animMode", 0);
animMode     = (savedAnim < (uint8_t)ANIM_COUNT) ? (AnimMode)savedAnim : ANIM_RAINBOW;
slotInterval = (SlotInterval)prefs.getUChar("slotIval", 0);
```

- [ ] **Schritt 4: colonBright-Default anheben**

Zeile 239 — Default von 15 auf 80:

```cpp
// ALT:
colonBright = prefs.getUChar("colonBright", 15);

// NEU:
colonBright = prefs.getUChar("colonBright", 80);
```

- [ ] **Schritt 5: Loop-Trigger ersetzen**

Zeilen 325–330 — alten Slot-Trigger ersetzen:

```cpp
// ALT:
if (animMode == ANIM_SLOTS && curSec % 10 == 0) {
  // Alle 10 Sekunden Slot-Animation
  startSlotAnimation(curHour, curMin, curSec);
} else if (!slotActive) {
  setDisplayTime(curHour, curMin, curSec);
}

// NEU:
bool triggerSlot = false;
switch (slotInterval) {
  case SLOT_10S:   triggerSlot = (curSec % 10 == 0); break;
  case SLOT_1MIN:  triggerSlot = (curSec == 0); break;
  case SLOT_15MIN: triggerSlot = (curSec == 0 && curMin % 15 == 0); break;
  case SLOT_1HR:   triggerSlot = (curSec == 0 && curMin == 0); break;
  default: break;
}
if (triggerSlot && !slotActive) startSlotAnimation(curHour, curMin, curSec);
else if (!slotActive) setDisplayTime(curHour, curMin, curSec);
```

- [ ] **Schritt 6: Compile-Check**

Arduino IDE → Strg+R. Erwartetes Ergebnis: keine Fehler, keine Warnings zu `ANIM_SLOTS`.

- [ ] **Schritt 7: Commit**

```bash
git add NixieClockUltra.ino
git commit -m "feat: SlotInterval enum, AnimMode ohne ANIM_SLOTS, colonBright-Default 80"
```

---

## Task 2: NeoPixel-Override-Schicht (neo_animation.ino)

**Files:**
- Modify: `neo_animation.ino`

- [ ] **Schritt 1: ANIM_SLOTS case entfernen**

Den gesamten `case ANIM_SLOTS:` Block (Zeilen 66–84) löschen:

```cpp
// ENTFERNEN — komplett löschen:
    case ANIM_SLOTS: {
      if (slotActive) {
        uint32_t col = strip.ColorHSV((millis() / 5) % 65536, 255, 255);
        for (int i = 0; i < 6; i++) strip.setPixelColor(i, scaleColor(col, neoBright));
        for (int i = 6; i < 10; i++) strip.setPixelColor(i, rgbSwap(scaleColor(col, colonBright)));
      } else {
        for (int i = 0; i < 6; i++) {
          uint8_t hue = neoHue + i * 40;
          strip.setPixelColor(i, scaleColor(strip.ColorHSV(hue * 256, neoSat, 255), neoBright));
        }
        neoHue++;
        bool colonOn = colonAlwaysOn || (curSec % 2 == 0);
        uint32_t colonColor = colonOn
          ? strip.ColorHSV((neoHue + 128) * 256, 200, 255)
          : strip.Color(0, 0, 0);
        for (int i = 6; i < 10; i++) strip.setPixelColor(i, rgbSwap(scaleColor(colonColor, colonBright)));
      }
      break;
    }
```

- [ ] **Schritt 2: Slot-Override-Block einfügen**

Nach dem `switch`-Block, direkt vor dem `colonStatic`-Block (vor Zeile 89) einfügen:

```cpp
  // Slot-Effekt überschreibt Hintergrundanimation solange slotActive
  if (slotActive) {
    uint32_t col = strip.ColorHSV((millis() / 5) % 65536, 255, 255);
    for (int i = 0; i < 6;  i++) strip.setPixelColor(i, scaleColor(col, neoBright));
    for (int i = 6; i < 10; i++) strip.setPixelColor(i, rgbSwap(scaleColor(col, colonBright)));
  }
```

Die Datei soll danach so aussehen (Struktur nach dem switch):

```cpp
    default: break;
  }

  // Slot-Effekt überschreibt Hintergrundanimation solange slotActive
  if (slotActive) {
    uint32_t col = strip.ColorHSV((millis() / 5) % 65536, 255, 255);
    for (int i = 0; i < 6;  i++) strip.setPixelColor(i, scaleColor(col, neoBright));
    for (int i = 6; i < 10; i++) strip.setPixelColor(i, rgbSwap(scaleColor(col, colonBright)));
  }

  // Statischer Modus: Trennpunkte unabhängig von Animation warmweiß überschreiben
  if (colonStatic) {
```

- [ ] **Schritt 3: Compile-Check**

Arduino IDE → Strg+R. Erwartetes Ergebnis: keine Fehler.

- [ ] **Schritt 4: Commit**

```bash
git add neo_animation.ino
git commit -m "feat: Slot-Effekt als Override-Schicht, ANIM_SLOTS case entfernt"
```

---

## Task 3: Web UI — HTML & JavaScript (web_server.ino)

**Files:**
- Modify: `web_server.ino` (PROGMEM-String und Script-Block)

- [ ] **Schritt 1: "Slot-Machine"-Option aus Animations-Select entfernen**

Im PROGMEM-HTML-String die Zeile mit `value="3"` löschen:

```html
<!-- ENTFERNEN: -->
      <option value="3">Slot-Machine</option>
```

Der Select soll danach so aussehen:

```html
    <select id="anim" onchange="setAnim()">
      <option value="0">Rainbow</option>
      <option value="1">Statisch Warmweiß</option>
      <option value="2">Puls</option>
    </select>
```

- [ ] **Schritt 2: Slot-Karte um Intervall-Dropdown erweitern**

Den bisherigen Slot-Card-Block:

```html
<div class="card">
  <h2>🎰 Slot-Animation starten</h2>
  <div class="row">
    <button onclick="triggerSlot()">Slot-Machine!</button>
    <span class="badge" id="slotStatus">bereit</span>
  </div>
</div>
```

ersetzen durch:

```html
<div class="card">
  <h2>🎰 Slot-Animation</h2>
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
  <div class="row">
    <button onclick="triggerSlot()">Slot-Machine!</button>
    <span class="badge" id="slotStatus">bereit</span>
  </div>
</div>
```

- [ ] **Schritt 3: refreshClock() erweitern**

In `refreshClock()` nach der Zeile `if(d.anim!==undefined)...` einfügen:

```js
  if(d.slotIval!==undefined) document.getElementById('slotIval').value=d.slotIval;
```

Die Funktion soll danach so aussehen (relevanter Ausschnitt):

```js
  if(d.anim!==undefined) document.getElementById('anim').value=d.anim;
  if(d.slotIval!==undefined) document.getElementById('slotIval').value=d.slotIval;
  if(d.colonOn!==undefined)document.getElementById('colonOn').checked=d.colonOn;
```

- [ ] **Schritt 4: setSlotInterval()-Funktion hinzufügen**

Nach der `setAnim()`-Funktion einfügen:

```js
async function setSlotInterval(){
  let v=parseInt(document.getElementById('slotIval').value);
  await api('/api/slotinterval',{interval:v});
}
```

- [ ] **Schritt 5: Compile-Check**

Arduino IDE → Strg+R. Erwartetes Ergebnis: keine Fehler.

- [ ] **Schritt 6: Commit**

```bash
git add web_server.ino
git commit -m "feat: Web-UI Slot-Intervall-Dropdown, Animation-Select ohne Slot-Option"
```

---

## Task 4: Web API (web_server.ino — C++ setupWebServer)

**Files:**
- Modify: `web_server.ino` (C++-Teil, `setupWebServer()`-Funktion)

- [ ] **Schritt 1: slotIval zu /api/status hinzufügen**

Im `/api/status`-Handler nach `doc["slot"] = slotActive;` einfügen:

```cpp
doc["slotIval"] = (int)slotInterval;
```

Der Block soll danach so aussehen:

```cpp
doc["bright"]    = brightLevel;
doc["neoBright"] = neoBright;
doc["anim"]      = (int)animMode;
doc["slot"]      = slotActive;
doc["slotIval"]  = (int)slotInterval;
doc["colonOn"]     = colonAlwaysOn;
```

Hinweis: `StaticJsonDocument<256>` reicht für das eine zusätzliche Feld.

- [ ] **Schritt 2: /api/slotinterval Endpoint hinzufügen**

Nach dem `/api/slot`-Handler (nach Zeile ~406) einfügen:

```cpp
  server.on("/api/slotinterval", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      StaticJsonDocument<64> doc;
      if (!deserializeJson(doc, data, len)) {
        int v = constrain((int)doc["interval"], 0, 4);
        slotInterval = (SlotInterval)v;
        prefs.putUChar("slotIval", v);
        req->send(200, "application/json", "{\"ok\":true}");
      } else {
        req->send(400, "application/json", "{\"ok\":false}");
      }
    }
  );
```

- [ ] **Schritt 3: Compile-Check**

Arduino IDE → Strg+R. Erwartetes Ergebnis: keine Fehler, keine Warnungen.

- [ ] **Schritt 4: Commit**

```bash
git add web_server.ino
git commit -m "feat: /api/slotinterval Endpoint, slotIval in /api/status"
```

---

## Task 5: Flash & Hardware-Verifikation

**Files:** keine Codeänderungen

- [ ] **Schritt 1: Flashen**

Arduino IDE → Strg+U (Upload). ESP32-S3 ggf. vorher in Download-Mode: BOOT gedrückt halten → RST drücken → BOOT loslassen.

- [ ] **Schritt 2: Animationsmodi prüfen**

Web-UI öffnen (`192.168.4.1`). Im Dropdown "Animation" darf "Slot-Machine" nicht mehr erscheinen. Alle drei Modi (Rainbow, Statisch Warmweiß, Puls) umschalten — jeder muss funktionieren.

- [ ] **Schritt 3: Slot-Intervall 10 Sek testen**

Intervall auf "10 Sek" setzen. Nach spätestens 10 Sekunden muss die Slot-Animation automatisch starten. Während der Animation soll der Regenbogen-Flash-Effekt auf den Hintergrund-Pixeln laufen.

- [ ] **Schritt 4: Manuellen Slot-Button testen**

Intervall auf "Aus" setzen. Den "Slot-Machine!"-Button drücken — Animation muss sofort starten.

- [ ] **Schritt 5: NVS-Persistenz prüfen**

Intervall auf "1 Min" setzen, ESP32-S3 per RST neu starten. Web-UI öffnen — Dropdown muss noch "1 Min" zeigen.

- [ ] **Schritt 6: Hintergrundanimation während Slot prüfen**

Animation auf "Puls" setzen, dann Slot-Button drücken. Während der Röhren-Animation soll der Regenbogen-Flash erscheinen (Hintergrundanimation wird überschrieben). Nach Ende der Slot-Animation soll Puls wieder laufen.

- [ ] **Schritt 7: Abschluss-Commit (falls keine Fixes nötig)**

```bash
git tag v$(date +%Y%m%d)-slot-interval
```
