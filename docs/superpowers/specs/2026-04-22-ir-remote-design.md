# IR-Fernbedienungssteuerung – Design-Spec

**Projekt:** NixieClockUltra  
**Datum:** 2026-04-22  
**Status:** Freigegeben

---

## Überblick

Erweiterung der NixieClockUltra-Firmware um IR-Fernbedienungsempfang sowie einen abschaltbaren Power-Save-Modus. Ein VS1838-IR-Empfänger auf Pin IO48 empfängt Signale beliebiger Fernbedienungen. Über das Web-Interface können Fernbedienungstasten den 7 Uhrzeit-Funktionen zugeordnet werden (Anlernmodus). Die Zuordnung wird persistent im NVS-Flash gespeichert.

---

## Hardware

- **Bauteil:** VS1838 (38-kHz-IR-Empfänger)
- **Pin:** `IR_RECV_PIN = 48`
- **Bibliothek:** `IRremoteESP8266` (Arduino Library Manager)

---

## Unterstützte Funktionen

| Funktions-ID | Beschreibung |
|---|---|
| `SET` | Einstellmodus ein / weiterschalten / speichern (identisch mit BTN_SET) |
| `UP` | Wert erhöhen im Einstellmodus (identisch mit BTN_UP) |
| `DOWN` | Wert verringern im Einstellmodus (identisch mit BTN_DOWN) |
| `BRIGHTNESS` | Helligkeitsstufe wechseln (identisch mit BTN_LIGHT) |
| `ANIM_NEXT` | Nächsten Animationsmodus aktivieren |
| `SLOT` | Slot-Machine-Animation auslösen |
| `POWER_SAVE_TOGGLE` | Power-Save-Modus ein- oder ausschalten |

---

## Architektur

### Initialisierung

```cpp
#define IR_RECV_PIN 48
IRrecv irrecv(IR_RECV_PIN);
decode_results irResults;

// in setup():
irrecv.enableIRIn();
```

### Empfang im loop()

```cpp
void handleIR() {
  if (irrecv.decode(&irResults)) {
    uint64_t code = irResults.value;
    irrecv.resume();

    if (irLearnTarget != IR_LEARN_NONE) {
      // Anlernmodus: Code der Zielfunktion zuweisen
      saveIRCode(irLearnTarget, code);
      irLearnTarget = IR_LEARN_NONE;
    } else {
      // Normalbetrieb: Funktion ausführen
      dispatchIRAction(code);
    }
  }
}
```

### NeoPixel-Konflikt vermeiden

`strip.show()` wird immer von `irrecv.pause()` / `irrecv.resume()` eingeschlossen:

```cpp
irrecv.pause();
strip.show();
irrecv.resume();
```

Hintergrund: Auch auf ESP32-S3 (RMT-Backend) kann das Timing von `strip.show()` IR-GPIO-Interrupts stören und zu korrumpierten Codes führen. Ein während des ~300 µs langen Schreibvorgangs eintreffendes Signal geht verloren – das ist akzeptabel.

### Anlernmodus

```cpp
enum IrAction {
  IR_LEARN_NONE = -1,
  IR_ACTION_SET,
  IR_ACTION_UP,
  IR_ACTION_DOWN,
  IR_ACTION_BRIGHTNESS,
  IR_ACTION_ANIM_NEXT,
  IR_ACTION_SLOT,
  IR_ACTION_POWER_SAVE_TOGGLE,
  IR_ACTION_COUNT
};

volatile IrAction irLearnTarget = IR_LEARN_NONE;
```

- Über `/api/ir/learn` setzt der Web-Server `irLearnTarget` auf die gewünschte Funktion.
- 10-Sekunden-Timeout: Wenn innerhalb von 10 s kein Code empfangen wird, wird `irLearnTarget` auf `IR_LEARN_NONE` zurückgesetzt.
- Der nächste empfangene Code (egal welches Protokoll) wird dieser Funktion zugewiesen.
- Repeat-Codes (`0xFFFFFFFF` bei NEC) werden beim Anlernen ignoriert.

### Aktionsausführung

```cpp
void dispatchIRAction(uint64_t code) {
  for (int i = 0; i < IR_ACTION_COUNT; i++) {
    if (irCodes[i] != 0 && irCodes[i] == code) {
      executeAction((IrAction)i);
      lastInteractionMs = millis();  // Power-Save zurücksetzen
      return;
    }
  }
}
```

`executeAction()` führt die Aktion direkt aus (modifiziert Zustandsvariablen), ohne Button-Strukturen zu simulieren. Beispiel: `BRIGHTNESS` ruft denselben Code auf wie `handleBrightness()` bei `btnLight.pressed`. Das vermeidet Seiteneffekte durch die Entprelllogik.

---

## Datenspeicherung (NVS)

7 Einträge für IR-Codes in Preferences, Namespace `"nixie"`:

| Schlüssel | Typ | Inhalt |
|---|---|---|
| `ir_SET` | `uint64_t` | Gespeicherter Code, 0 = nicht belegt |
| `ir_UP` | `uint64_t` | — |
| `ir_DOWN` | `uint64_t` | — |
| `ir_BRIGHTNESS` | `uint64_t` | — |
| `ir_ANIM_NEXT` | `uint64_t` | — |
| `ir_SLOT` | `uint64_t` | — |
| `ir_PSTOGGLE` | `uint64_t` | — |

Zusätzlich ein Flag für den Power-Save-Aktivierungszustand:

| Schlüssel | Typ | Inhalt |
|---|---|---|
| `psEnabled` | `bool` | `true` = Power-Save aktiv (Standard), `false` = dauerhaft deaktiviert |

Beim Start werden alle 7 IR-Codes in `uint64_t irCodes[IR_ACTION_COUNT]` geladen sowie `powerSaveEnabled` aus NVS.

---

## Web-Interface

### Änderung bestehende Karte: „💡 Helligkeit & Animation"

Ein Toggle-Schalter **„Power-Save"** wird am Ende der Karte ergänzt:
- Beschriftung: `Power-Save (Auto-Dimmen nach 120 s)`
- Zustand wird per `/api/powersave` gesetzt und beim Laden der Seite aus `/api/status` gelesen.

### Neue Karte: „📡 IR-Fernbedienung"

Eingefügt nach der Helligkeit-Karte, vor dem WLAN-Bereich.

**Tabellenaufbau:**

| Funktion | Gespeicherter Code | Aktionen |
|---|---|---|
| SET | `NEC 0xFFA25D` | [Anlernen] [✕] |
| UP | `—` | [Anlernen] [✕] |
| … | … | … |
| Power-Save Toggle | `—` | [Anlernen] [✕] |

- **[Anlernen]:** Startet POST `/api/ir/learn` → Zeile blinkt orange, Statustext „Taste drücken…" (10 s Timeout)
- **[✕]:** POST `/api/ir/clear` → Code wird gelöscht, Anzeige auf `—`
- Während Lernmodus aktiv: Frontend pollt `/api/ir/status` alle 500 ms. Sobald Code empfangen, Zeile aktualisiert sich und Lernmodus endet.

### Neue API-Endpunkte

| Endpunkt | Methode | Request | Response |
|---|---|---|---|
| `/api/ir/status` | GET | — | `{"learning":"SET","codes":{"SET":"NEC 0xFFA25D","UP":"","...":"..."}}` |
| `/api/ir/learn` | POST | `{"action":"SET"}` | `{"ok":true}` |
| `/api/ir/clear` | POST | `{"action":"SET"}` | `{"ok":true}` |
| `/api/powersave` | POST | `{"enabled":true}` | `{"ok":true}` |

`/api/status` wird um das Feld `"psEnabled": true/false` erweitert.

---

## Randbedingungen

- Repeat-Codes (z.B. NEC `0xFFFFFFFFFFFFFFFF`) werden im Normalbetrieb ignoriert (kein ungewolltes Auto-Repeat).
- `POWER_SAVE_TOGGLE` schaltet `powerSaveEnabled` um und speichert den neuen Wert in NVS. Wenn deaktiviert, greift `handlePowerSave()` nicht mehr – NeoPixel bleiben dauerhaft auf voller Helligkeit.
- Die IR-Verarbeitung greift auf dieselben Zustandsvariablen zu wie die physischen Taster – kein paralleler Einstellmodus möglich (IR und Taster arbeiten additiv, nicht gleichzeitig).
- `irrecv.pause()` / `irrecv.resume()` umschließen jeden `strip.show()`-Aufruf.
- Die Bibliothek `IRremoteESP8266` muss in der `platformio.ini` bzw. im Arduino Library Manager hinzugefügt werden.
