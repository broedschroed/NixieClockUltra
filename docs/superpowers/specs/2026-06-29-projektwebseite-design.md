# Design-Spec: Nixie Clock Ultra – Projektwebseite

**Datum:** 2026-06-29
**Status:** Genehmigt, bereit zur Implementierung

---

## Ziel

Mehrseitige, lokale Projektwebseite für die Nixie Clock Ultra. Technologie: reines
HTML/CSS/JS (statisch, kein Build-Schritt). Steampunk-Ästhetik: dunkel-industriell,
Messing-/Goldakzente, Nixie-Orangeglühen, gravierte Typografie. Später direkt auf
Webserver oder GitHub Pages übertragbar.

---

## Dateistruktur

```
docs/website/
├── index.html            Startseite
├── features.html         Features & Werbetext
├── aufbau.html           Hardware-Aufbau, PCBs, Schematics
├── geschichte.html       Entwicklungshistorie & Problemlösungen
├── dokumentation.html    Downloads (Systemdoku, Anleitung)
├── galerie.html          Fotogalerie
├── css/
│   └── style.css         Globales Stylesheet (Steampunk-Theme)
├── js/
│   ├── nav.js            Gemeinsamer Header + Footer (per JS injiziert)
│   └── lightbox.js       Galerie-Lightbox (Vanilla JS)
├── img/
│   ├── hero.jpg          Hero-Foto Startseite (vom Nutzer bereitgestellt)
│   ├── gallery/          Alle Galeriefotos (vom Nutzer bereitgestellt)
│   └── pcb/              PCB-Bilder (symlink/kopie aus docs/system/assets/)
└── downloads/
    ├── NixieClockUltra-Systemdokumentation.odt
    ├── NixieClockUltra-Systemdokumentation.pdf
    └── NixieClockUltra-Bedienungsanleitung.pdf
```

---

## Design-System

### Farbpalette (CSS Custom Properties)

```css
--bg-base:        #0e0a05   /* Seitenhintergrund, tiefstes Schwarz-Braun */
--bg-panel:       #1c1408   /* Inhalts-Panels, leicht erhöht */
--bg-panel-hover: #241a0a   /* Panel-Hover-Zustand */
--text-primary:   #e8d5a3   /* Haupttext, Pergament-Creme */
--text-muted:     #a08c6a   /* Gedämpfter Text, Beschriftungen */
--accent-gold:    #c8961a   /* Primärakzent, Messing-Gold */
--accent-copper:  #cd7f32   /* Sekundärakzent, Kupfer */
--accent-nixie:   #ff8c00   /* Nixie-Glühen, für Highlights */
--border-dim:     #3a2a10   /* Ruhige Rahmenlinie */
--border-gold:    #6b4f12   /* Goldener Rahmen */
```

### Typografie

- **Überschriften:** Cinzel (Google Fonts) — graviert-klassisch
- **Fließtext:** Crimson Text (Google Fonts) — warm, antiquarisch
- **Code/Technisch:** Liberation Mono / Courier New

### Dekorelemente

- **Zahnrad-Divider:** SVG-Inline-Elemente als horizontale Trennlinien zwischen Sektionen
- **Niet-Ecken:** CSS-Pseudo-Elemente (::before / ::after) an Panel-Ecken als kleine Kreise
- **Nav-Tabs:** Rechteckige Messing-Schilde, aktive Seite gold hinterlegt
- **Download-Buttons:** Geniete Metallplatten-Optik, Hover-Effekt mit Kupfer-Glow

---

## Globales Layout

Jede Seite besteht aus vier Zonen, die `nav.js` per `innerHTML` injiziert:

```
<body>
  <div id="site-header">  ← nav.js injiziert
  <nav id="site-nav">     ← nav.js injiziert, aktive Seite per data-page markiert
  <main id="content">     ← seitenspezifischer Inhalt
  <footer id="site-footer"> ← nav.js injiziert
```

**Header:** Schmales dunkles Band. Links kleines Zahnrad-SVG, mittig
„NIXIE CLOCK ULTRA" in Cinzel 28pt gold, darunter „broed digital media · 2026"
in Crimson Text 11pt gedämpft, rechts spiegelverkehrtes Zahnrad-SVG.

**Navigation:** Horizontale Leiste direkt unter Header. Sechs Tabs:
Startseite · Features · Aufbau · Geschichte · Dokumentation · Galerie.
Aktiver Tab: goldener Hintergrund, schwarzer Text. Hover: Kupfer-Glow.
Auf Mobilgeräten (< 768px): Hamburger-Icon, aufklappbares Dropdown.

**Content-Bereich:** max-width 1100px, zentriert, padding 2rem.

**Footer:** Schmale dunkle Leiste. Oben ein Niet-Separator (horizontale Linie
mit kleinen Kreisen). Text: „© 2026 broed digital media · Nixie Clock Ultra".

---

## Seiten im Detail

### 1. Startseite (`index.html`)

**Hero-Sektion (volle Viewport-Breite):**
- Hintergrundbild: `img/hero.jpg`, `object-fit: cover`, Höhe 70vh
- Dunkles Overlay: `rgba(0,0,0,0.55)` gradient
- Darüber zentriert:
  - „NIXIE CLOCK ULTRA" — Cinzel 52pt, gold
  - „wenn Zeit leuchtet" — Crimson Text 18pt, italic, kupfer
  - „→ Mehr erfahren"-Button (Link auf features.html)

**Highlight-Kacheln (3 Stück, Flex-Row):**
Unter dem Hero, jede Kachel: dunkles Panel, goldene Überschrift, ein Satz Text.
Inhalte: „Direct Drive — kein Ghosting", „WiFi & Web-Interface",
„Nacht-Modus & Lichtsensor".

**Teaser-Text:**
Erster Absatz aus Werbetext (`NixieClockUltra-Werbetext.odt`), max 250 Wörter.
Abschluss: Link „Alle Features →" zu features.html.

---

### 2. Features (`features.html`)

**Seitentitel:** „Features" in Cinzel h1, Zahnrad-Divider darunter.

**Fließtext-Block:** Vollständiger Werbetext (alle fünf Absätze).

**Feature-Gruppen:** Fünf dunkle Panels untereinander:
- Anzeige · Beleuchtung · Konnektivität & Bedienung · Nacht-Modus · Technik
- Jedes Panel: goldene Gruppenüberschrift (h3), Bullet-Liste mit „›"-Bullet in
  Nixie-Orange, Text in Crimson Text.

---

### 3. Aufbau (`aufbau.html`)

**Seitentitel:** „Aufbau & Hardware"

**Abschnitt Logic Board:**
Zweispaltig (auf Mobile einspaltig): links `img/pcb/logic_sch-1.png` +
`img/pcb/logic_pcb-1.png` als klickbare Thumbnails (öffnen Lightbox),
rechts Text: Rev 2.1, Komponenten-Highlights (ESP32-S3, DC-DC, HV-MOD, LDR),
USB-CDC-Warnung als farbige Hinweisbox.

**Abschnitt Nixie Display Board:**
Gleiche Zweispalten-Struktur mit `nixie_sch-1.png` + `nixie_pcb-1.png`,
Text: MCP23017 Direct Drive, 60 SMBTA42-Transistoren, kein Ghosting.

**Abschnitt Spannungsversorgung:**
SVG-Flussdiagramm: USB 5V → DC-DC (U6) → 10V → HV-MOD (U4) → 170V (Nixie-Anoden)
                    USB 5V → AMS1117 (U5) → 3,3V (Logik)
Goldene Pfeile, dunkle Beschriftungsboxen.

---

### 4. Geschichte (`geschichte.html`)

**Seitentitel:** „Entwicklungsgeschichte"

**Timeline-Layout:**
Vertikale goldene Linie mittig (auf Desktop links-mittig), Messing-Punkte als
Meilenstein-Marker. Jeder Eintrag: Datum in Nixie-Orange, Titel in Cinzel,
Kurztext in Crimson Text. Basis: Inhalt aus `docs/ENTWICKLUNGSHISTORIE.md`,
manuell als HTML aufbereitet.

**Probleme & Lösungen:**
CSS-only Akkordeon (checkbox-hack): Überschrift mit „▶"-Pfeil, Klick klappt
Lösungstext auf. Keine JS-Abhängigkeit.

---

### 5. Dokumentation (`dokumentation.html`)

**Seitentitel:** „Dokumentation & Downloads"

**Zwei Download-Karten** (Flex-Row, auf Mobile gestapelt):

*Karte 1 — Systemdokumentation:*
Icon (Zahnrad-SVG), Titel, Kurztext (2 Sätze), zwei Buttons:
„ODT herunterladen" + „PDF herunterladen" → `downloads/`

*Karte 2 — Bedienungsanleitung:*
Icon (Buch-SVG), Titel, Kurztext (2 Sätze), Button:
„PDF herunterladen" → `downloads/`

**Hinweis-Box:** „ODT-Dateien können mit LibreOffice geöffnet werden."

---

### 6. Galerie (`galerie.html`)

**Seitentitel:** „Galerie"

**Masonry-Grid:** CSS columns (3 Spalten Desktop, 2 Tablet, 1 Mobile).
Jedes Bild: dunkler Rahmen 3px `--border-gold`, leichter Box-Shadow,
Hover: leicht aufhellen + Kupfer-Glow.

**Lightbox (`lightbox.js`):**
- Klick auf Bild → Overlay mit schwarzem Hintergrund, Bild zentriert, max 90vw/90vh
- Navigation: Pfeil links/rechts (Tastatur + Klick)
- Schließen: Klick außerhalb oder ESC-Taste
- Kein externes Framework, ~60 Zeilen JS

---

## Responsive Breakpoints

| Breakpoint | Verhalten |
|-----------|-----------|
| > 1100px  | Volle Breite, max-width 1100px zentriert |
| 768–1100px | Zweispalten-Layouts bleiben, kleinere Schrift |
| < 768px   | Alles einspaltig, Hamburger-Nav, Hero 50vh |

---

## Assets die vom Nutzer bereitgestellt werden

- `img/hero.jpg` — Hauptfoto der fertigen Uhr (für Hero-Sektion)
- `img/gallery/*.jpg` — Alle Galeriefotos

PCB-Bilder werden aus `docs/system/assets/` in `img/pcb/` kopiert.
Downloads werden aus bestehenden Docs-Ordnern in `downloads/` kopiert.

---

## Nicht im Scope

- Kein Server-Side-Rendering
- Kein CMS
- Kein CSS-Framework (Bootstrap, Tailwind etc.)
- Kein TypeScript / Build-Tool
- Keine Suchfunktion
- Keine Mehrsprachigkeit
