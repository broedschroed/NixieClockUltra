# Design-Spec: Website – Sprachumschalter Deutsch/Englisch

**Datum:** 2026-07-12
**Status:** Genehmigt, bereit zur Implementierung

---

## Ziel

Die bestehende statische Projektwebseite (`docs/website/`, 6 Seiten, siehe
`2026-06-29-projektwebseite-design.md`) bekommt einen client-seitigen Sprachumschalter
Deutsch/Englisch. Umschalten erfolgt ohne Seiten-Reload und ohne URL-Wechsel, die Wahl
bleibt über Seitennavigation hinweg erhalten. Kein Build-Schritt — der GitHub-Pages-Workflow
lädt `docs/website` weiterhin unverändert 1:1 hoch.

Geltungsbereich: alle 6 Seiten (`index`, `features`, `aufbau`, `geschichte`,
`dokumentation`, `galerie`).

---

## Architektur & neue/geänderte Dateien

**Neu:**
- `js/i18n-de.js` — Wörterbuch, ein `key: "deutscher Text"`-Eintrag pro übersetzbarem
  Textbaustein, gruppiert nach Seite plus globale `nav.*`/`footer.*`-Keys
- `js/i18n-en.js` — gleiche Keys, englische Übersetzungen (von mir übersetzt, analog zum
  bereits vorhandenen englischen README)
- `js/i18n.js` — Kernlogik: Sprache ermitteln, Wörterbuch anwenden, Umschalter-Klicks
  behandeln

**Geändert:**
- Alle 6 HTML-Seiten: übersetzbare Textknoten bekommen `data-i18n="key"`-Attribute;
  deutscher Text bleibt als sichtbarer Fallback-Inhalt im HTML stehen (progressive
  enhancement — ohne JS bleibt die Seite auf Deutsch lesbar)
- `js/nav.js`: rendert den DE/EN-Umschalter rechts in der Nav-Leiste (`.nav-inner`,
  analog zum bestehenden `.nav-toggle`/`.nav-inner a.active`-Muster); ruft nach dem
  Injizieren von Header/Nav/Footer `applyLanguage()` aus `i18n.js` auf, damit auch die
  injizierten Footer-Texte übersetzt werden
- `css/style.css`: Styling für `.lang-switch` (DE│EN-Umschalter), passend zum
  bestehenden Nav-Look

**Reihenfolge beim Seitenaufbau:** `nav.js` injiziert Header/Nav/Footer →
`i18n.js` wendet die gespeicherte Sprache auf das gesamte Dokument an (inkl. der
gerade injizierten Teile). Beide Skripte laufen bei `DOMContentLoaded`.

---

## Datenfluss & Umschalt-Verhalten

**Beim Laden einer Seite:**
1. `nav.js` injiziert Header/Nav/Footer (wie bisher)
2. `i18n.js` liest `localStorage['lang']` (Default `'de'`, falls noch nie gesetzt —
   keine Browsersprachen-Erkennung)
3. `applyLanguage(lang)` läuft: setzt `document.documentElement.lang = lang`, setzt
   `document.title` anhand eines seitenspezifischen `meta.title`-Keys, geht alle
   `[data-i18n]`-Elemente durch und schreibt den passenden Wörterbuch-Text hinein,
   markiert den aktiven Button im Umschalter (`.active`-Klasse)

**Beim Klick auf DE/EN:**
1. Click-Handler auf `.lang-switch button` setzt `localStorage['lang']`
2. `applyLanguage(lang)` wird sofort erneut aufgerufen — kein Reload
3. Die Sprachwahl bleibt beim Navigieren zu einer der anderen 5 Seiten erhalten (wird
   beim nächsten Seitenaufbau wieder aus `localStorage` gelesen)

**Platzierung des Umschalters:** In der bestehenden sticky Nav-Leiste, rechts neben den
Menüpunkten (Desktop wie Mobile/Hamburger-Ansicht).

---

## Wörterbuch-Format

Key-Schema: `<seite>.<bereich>.<element>` für seitenspezifische Texte, `nav.*` und
`footer.*` für global wiederverwendete Texte (Navigation, Copyright).

```js
// i18n-de.js
{
  "index.meta.title": "Nixie Clock Ultra",
  "index.hero.title": "NIXIE CLOCK ULTRA",
  "index.hero.tagline": "wenn Zeit leuchtet",
  "index.hero.cta": "Mehr erfahren →",
  "index.highlight1.title": "Direct Drive",
  "index.highlight1.body": "Jede Kathode hat ihren eigenen Transistor — kein Multiplexing, kein Ghosting, kein Flimmern.",
  "nav.startseite": "Startseite",
  "nav.features": "Features",
  "footer.copyright": "© 2026 broed digital media · Nixie Clock Ultra"
}
```

`i18n-en.js` verwendet dieselben Keys mit englischen Werten. Geschätzter Umfang:
ca. 120–150 Key/Value-Paare pro Sprache (bei ~550 Zeilen HTML-Ausgangstext).

HTML-Beispiel (`index.html`):
```html
<h1 class="hero-title" data-i18n="index.hero.title">NIXIE CLOCK ULTRA</h1>
<p class="hero-tagline" data-i18n="index.hero.tagline">wenn Zeit leuchtet</p>
```

---

## Nicht übersetzte Inhalte (bewusste Ausnahmen)

- **Downloads** (`dokumentation.html`): PDF/ODT-Dateien (Bedienungsanleitung,
  Systemdokumentation) bleiben ausschließlich Deutsch. Zusätzlicher `data-i18n`-Hinweis
  neben den Download-Karten: „(nur auf Deutsch verfügbar)“ / „(German only)“.
- **Bild-Alt-Texte auf technischen Fotos** (PCB-Fotos, Schematics mit eingebranntem
  Text): bleiben unverändert, da es sich um technische Abbildungen und keine UI-Texte
  handelt.

**Übersetzte Ausnahme:** `<title>`-Tags (Browser-Tab-Titel) werden mit übersetzt
(z. B. „Nixie Clock Ultra – Features“ / „Nixie Clock Ultra – Features“ analog EN),
gesteuert über den `meta.title`-Key pro Seite.

---

## Testing & Verifikation

Kein Build- oder Test-Framework für die Website vorhanden. Verifikation erfolgt manuell
im Browser:
- Jede der 6 Seiten in DE und EN durchklicken
- Umschalter testen, inkl. Persistenz beim Navigieren zwischen Seiten
- Mobile Ansicht (Hamburger-Nav) mit Umschalter prüfen
- Kurzer Fallback-Check: JS deaktiviert → Seite bleibt auf Deutsch lesbar

Kein automatisierter Test nötig, da reine Präsentationsschicht ohne Geschäftslogik.

---

## Offene Nicht-Themen (bewusst außerhalb des Scopes)

- Keine Erkennung der Browsersprache (`navigator.language`) — Default ist immer Deutsch
- Keine weiteren Sprachen außer DE/EN
- Keine Übersetzung der PDF/ODT-Downloads selbst
