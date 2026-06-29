# Nixie Clock Ultra – Projektwebseite Implementierungsplan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Statische, mehrseitige Projektwebseite im Steampunk-Design für die Nixie Clock Ultra, lokal im Browser aufrufbar.

**Architecture:** Reines HTML/CSS/JS ohne Build-Tools. Gemeinsamer Header/Nav/Footer wird von `nav.js` per DOM-Injection auf allen Seiten eingefügt. Aktive Seite wird über `data-page`-Attribut am `<body>` markiert. Lightbox für Galerie in Vanilla-JS.

**Tech Stack:** HTML5, CSS3 (Custom Properties, CSS Grid, Flexbox, CSS-only Akkordeon), Vanilla JS (~80 Zeilen gesamt), Google Fonts (Cinzel + Crimson Text), Python 3 HTTP-Server zum lokalen Testen.

## Global Constraints

- Kein CSS-Framework, kein JavaScript-Framework, kein Build-Tool
- Google Fonts werden per `<link>` aus dem Netz geladen (lokal reicht das)
- Alle Dateien unter `docs/website/` im Projektverzeichnis
- Sprache: Deutsch
- Ziel-Browser: aktuelle Chrome/Firefox/Safari (kein IE-Support nötig)
- Farbpalette und Typografie exakt wie in Design-Spec definiert

---

## Dateistruktur (Gesamtüberblick)

```
docs/website/
├── index.html
├── features.html
├── aufbau.html
├── geschichte.html
├── dokumentation.html
├── galerie.html
├── css/
│   └── style.css
├── js/
│   ├── nav.js
│   └── lightbox.js
├── img/
│   ├── hero.jpg            ← vom Nutzer bereitzustellen
│   ├── placeholder-hero.svg ← Fallback bis hero.jpg vorliegt
│   ├── gallery/            ← vom Nutzer bereitzustellen (*.jpg)
│   └── pcb/
│       ├── logic_sch-1.png
│       ├── logic_pcb-1.png
│       ├── logic_pcb-2.png
│       ├── nixie_sch-1.png
│       ├── nixie_pcb-1.png
│       └── nixie_pcb-2.png
└── downloads/
    ├── NixieClockUltra-Systemdokumentation.odt
    ├── NixieClockUltra-Systemdokumentation.pdf
    └── NixieClockUltra-Bedienungsanleitung.pdf
```

---

### Task 1: Scaffold + CSS Design System

**Files:**
- Create: `docs/website/css/style.css`
- Create: `docs/website/index.html` (Shell zum Testen)

**Interfaces:**
- Produces: CSS Custom Properties, alle Basis-Styles, Panel, Button, Gear-Divider, responsive Breakpoints — werden von allen späteren HTML-Dateien konsumiert

- [ ] **Schritt 1: Verzeichnisse anlegen**

```bash
mkdir -p docs/website/css docs/website/js docs/website/img/gallery docs/website/img/pcb docs/website/downloads
```

- [ ] **Schritt 2: `docs/website/css/style.css` erstellen**

```css
/* === Google Fonts === */
@import url('https://fonts.googleapis.com/css2?family=Cinzel:wght@400;700&family=Crimson+Text:ital,wght@0,400;0,600;1,400&display=swap');

/* === Custom Properties === */
:root {
  --bg-base:        #0e0a05;
  --bg-panel:       #1c1408;
  --bg-panel-hover: #241a0a;
  --text-primary:   #e8d5a3;
  --text-muted:     #a08c6a;
  --accent-gold:    #c8961a;
  --accent-copper:  #cd7f32;
  --accent-nixie:   #ff8c00;
  --border-dim:     #3a2a10;
  --border-gold:    #6b4f12;
}

/* === Reset === */
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
html { scroll-behavior: smooth; }
body {
  background: var(--bg-base);
  color: var(--text-primary);
  font-family: 'Crimson Text', Georgia, serif;
  font-size: 18px;
  line-height: 1.65;
}

/* === Typografie === */
h1, h2, h3 { font-family: 'Cinzel', 'Times New Roman', serif; color: var(--accent-gold); }
h1 { font-size: 2.2rem; margin-bottom: 1rem; }
h2 { font-size: 1.6rem; margin: 2rem 0 0.8rem; border-bottom: 1px solid var(--border-dim); padding-bottom: 0.4rem; }
h3 { font-size: 1.15rem; margin: 1.2rem 0 0.4rem; color: var(--accent-copper); }
p { margin-bottom: 1rem; }
a { color: var(--accent-gold); text-decoration: none; }
a:hover { color: var(--accent-nixie); }
strong { color: var(--accent-copper); font-weight: 600; }

/* === Layout === */
.container { max-width: 1100px; margin: 0 auto; padding: 0 2rem; }
main { padding: 2.5rem 0 4rem; }

/* === Header === */
#site-header {
  background: #080602;
  border-bottom: 2px solid var(--border-gold);
  padding: 1rem 0;
}
.header-inner {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 1.5rem;
}
.header-text { text-align: center; }
.site-title {
  font-family: 'Cinzel', serif;
  font-size: 1.8rem;
  color: var(--accent-gold);
  letter-spacing: 0.12em;
  text-shadow: 0 0 20px rgba(200,150,26,0.3);
}
.site-subtitle {
  font-size: 0.85rem;
  color: var(--text-muted);
  letter-spacing: 0.05em;
  margin-top: 0.2rem;
}

/* === Navigation === */
#site-nav {
  background: #100c06;
  border-bottom: 1px solid var(--border-gold);
  position: sticky;
  top: 0;
  z-index: 100;
}
.nav-inner {
  display: flex;
  justify-content: center;
  flex-wrap: wrap;
}
.nav-inner a {
  display: block;
  padding: 0.75rem 1.3rem;
  font-family: 'Cinzel', serif;
  font-size: 0.8rem;
  letter-spacing: 0.06em;
  text-transform: uppercase;
  color: var(--text-muted);
  border-right: 1px solid var(--border-dim);
  transition: background 0.2s, color 0.2s;
}
.nav-inner a:last-child { border-right: none; }
.nav-inner a:hover {
  background: var(--bg-panel-hover);
  color: var(--accent-copper);
}
.nav-inner a.active {
  background: var(--accent-gold);
  color: #0e0a05;
  font-weight: 700;
}
.nav-toggle {
  display: none;
  background: none;
  border: 1px solid var(--border-gold);
  color: var(--accent-gold);
  padding: 0.4rem 0.9rem;
  font-size: 1.3rem;
  cursor: pointer;
  margin: 0.5rem auto;
}

/* === Gear Divider === */
.gear-divider {
  display: flex;
  align-items: center;
  gap: 0.8rem;
  margin: 2rem 0;
  color: var(--border-gold);
  font-size: 1rem;
}
.gear-divider::before,
.gear-divider::after {
  content: '';
  flex: 1;
  height: 1px;
  background: linear-gradient(to right, transparent, var(--border-gold), transparent);
}

/* === Panel === */
.panel {
  background: var(--bg-panel);
  border: 1px solid var(--border-gold);
  border-radius: 2px;
  padding: 1.5rem 2rem;
  margin-bottom: 1.5rem;
  position: relative;
}
.panel::before { content: '⬤'; color: var(--border-gold); font-size: 0.35rem; position: absolute; top: 0.5rem; left: 0.5rem; }
.panel::after  { content: '⬤'; color: var(--border-gold); font-size: 0.35rem; position: absolute; bottom: 0.5rem; right: 0.5rem; }

/* === Buttons === */
.btn {
  display: inline-block;
  padding: 0.55rem 1.3rem;
  font-family: 'Cinzel', serif;
  font-size: 0.8rem;
  letter-spacing: 0.06em;
  text-transform: uppercase;
  border: 1px solid var(--accent-gold);
  color: var(--accent-gold);
  background: transparent;
  cursor: pointer;
  transition: all 0.2s;
  text-decoration: none;
}
.btn:hover {
  background: var(--accent-gold);
  color: var(--bg-base);
  box-shadow: 0 0 14px rgba(200,150,26,0.35);
  color: var(--bg-base);
}
.btn-primary {
  background: var(--accent-gold);
  color: var(--bg-base);
}
.btn-primary:hover {
  background: var(--accent-copper);
  border-color: var(--accent-copper);
  color: var(--bg-base);
}
.btn-group { display: flex; gap: 0.8rem; flex-wrap: wrap; }

/* === Warning / Note Box === */
.warning-box {
  background: #1a0d0a;
  border-left: 3px solid var(--accent-nixie);
  padding: 1rem 1.5rem;
  margin: 1rem 0;
  font-size: 0.95rem;
}
.warning-box strong { color: var(--accent-nixie); }
.note-box {
  background: #0e0c08;
  border: 1px solid var(--border-dim);
  padding: 0.9rem 1.3rem;
  margin: 1rem 0;
  color: var(--text-muted);
  font-size: 0.9rem;
}

/* === Page Title === */
.page-title { padding: 2.5rem 0 0; }
.page-title h1 { font-size: 2rem; }

/* === Footer === */
#site-footer {
  background: #080602;
  border-top: 1px solid var(--border-gold);
  padding: 1.2rem 0;
  text-align: center;
}
.footer-text { font-size: 0.85rem; color: var(--text-muted); letter-spacing: 0.03em; }

/* === Responsive === */
@media (max-width: 768px) {
  body { font-size: 16px; }
  .site-title { font-size: 1.3rem; }
  main { padding: 1.5rem 0 3rem; }
  .container { padding: 0 1rem; }
  .nav-toggle { display: block; }
  .nav-inner { display: none; flex-direction: column; }
  .nav-inner.open { display: flex; }
  .nav-inner a { border-right: none; border-bottom: 1px solid var(--border-dim); text-align: center; padding: 0.9rem; }
  h1 { font-size: 1.7rem; }
  h2 { font-size: 1.3rem; }
  .panel { padding: 1rem 1.2rem; }
}
```

- [ ] **Schritt 3: Test-Shell `docs/website/index.html` erstellen**

```html
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Nixie Clock Ultra</title>
  <link rel="stylesheet" href="css/style.css">
</head>
<body data-page="index">
  <header id="site-header"></header>
  <nav id="site-nav"></nav>
  <main>
    <div class="container">
      <div class="panel"><h1>CSS-Test</h1><p>Wenn dieser Text in Crimson Text auf dunklem Hintergrund erscheint, sind die Styles geladen.</p><a href="#" class="btn">Button-Test</a></div>
      <div class="gear-divider">⚙</div>
      <div class="warning-box"><strong>Hinweis:</strong> Warning-Box Test.</div>
    </div>
  </main>
  <footer id="site-footer"></footer>
  <script src="js/nav.js"></script>
</body>
</html>
```

- [ ] **Schritt 4: Lokalen Server starten und prüfen**

```bash
python3 -m http.server 8080 --directory docs/website
```

Öffne http://localhost:8080 im Browser.
Erwartetes Ergebnis: dunkler Hintergrund `#0e0a05`, Text in Cremeton, Panel mit Goldrand, Button mit Goldrahmen. Header/Nav/Footer noch leer (nav.js fehlt noch).

- [ ] **Schritt 5: Commit**

```bash
git add docs/website/css/style.css docs/website/index.html
git commit -m "feat: website scaffold + CSS design system (Steampunk)"
```

---

### Task 2: nav.js — Gemeinsamer Header, Navigation, Footer

**Files:**
- Create: `docs/website/js/nav.js`

**Interfaces:**
- Consumes: `document.body.dataset.page` — Wert muss mit einem der `key`-Werte in `NAV_PAGES` übereinstimmen
- Produces: Füllt `#site-header`, `#site-nav`, `#site-footer` per innerHTML; markiert aktiven Nav-Link mit Klasse `active`

- [ ] **Schritt 1: `docs/website/js/nav.js` erstellen**

```javascript
const NAV_PAGES = [
  { href: 'index.html',         label: 'Startseite',    key: 'index' },
  { href: 'features.html',      label: 'Features',      key: 'features' },
  { href: 'aufbau.html',        label: 'Aufbau',        key: 'aufbau' },
  { href: 'geschichte.html',    label: 'Geschichte',    key: 'geschichte' },
  { href: 'dokumentation.html', label: 'Dokumentation', key: 'dokumentation' },
  { href: 'galerie.html',       label: 'Galerie',       key: 'galerie' },
];

const GEAR_SVG = `<svg width="32" height="32" viewBox="0 0 100 100" fill="none" xmlns="http://www.w3.org/2000/svg" aria-hidden="true">
  <path d="M43 5h14l2 11a34 34 0 0 1 8 4.5l10-3.5 9.9 9.9-3.5 10A34 34 0 0 1 88 45l11 2v14l-11 2a34 34 0 0 1-4.5 8l3.5 10-9.9 9.9-10-3.5A34 34 0 0 1 59 91l-2 9H43l-2-9a34 34 0 0 1-8-4.5l-10 3.5-9.9-9.9 3.5-10A34 34 0 0 1 12 57L1 55V41l11-2a34 34 0 0 1 4.5-8l-3.5-10 9.9-9.9 10 3.5A34 34 0 0 1 41 10Z"
    fill="none" stroke="#6b4f12" stroke-width="5"/>
  <circle cx="50" cy="50" r="16" fill="none" stroke="#6b4f12" stroke-width="5"/>
</svg>`;

function injectNav() {
  const page = document.body.dataset.page || '';

  const header = document.getElementById('site-header');
  if (header) {
    header.innerHTML = `
      <div class="container">
        <div class="header-inner">
          ${GEAR_SVG}
          <div class="header-text">
            <div class="site-title">NIXIE CLOCK ULTRA</div>
            <div class="site-subtitle">broed digital media &middot; 2026</div>
          </div>
          ${GEAR_SVG}
        </div>
      </div>`;
  }

  const nav = document.getElementById('site-nav');
  if (nav) {
    const links = NAV_PAGES.map(p =>
      `<a href="${p.href}"${p.key === page ? ' class="active"' : ''}>${p.label}</a>`
    ).join('');
    nav.innerHTML = `
      <div class="container">
        <button class="nav-toggle" aria-label="Navigation" onclick="this.nextElementSibling.classList.toggle('open')">&#9776;</button>
        <div class="nav-inner">${links}</div>
      </div>`;
  }

  const footer = document.getElementById('site-footer');
  if (footer) {
    footer.innerHTML = `
      <div class="container">
        <p class="footer-text">&copy; 2026 broed digital media &middot; Nixie Clock Ultra</p>
      </div>`;
  }
}

document.addEventListener('DOMContentLoaded', injectNav);
```

- [ ] **Schritt 2: Browser prüfen**

Server läuft noch auf Port 8080. Reload von http://localhost:8080
Erwartetes Ergebnis:
- Header mit Zahnrad-SVG links, „NIXIE CLOCK ULTRA" in Cinzel mittig, Zahnrad rechts
- Nav mit allen 6 Links, „Startseite" golden hinterlegt (active)
- Footer mit Copyright-Text

- [ ] **Schritt 3: Commit**

```bash
git add docs/website/js/nav.js
git commit -m "feat: nav.js gemeinsamer Header, Navigation, Footer"
```

---

### Task 3: Startseite (index.html)

**Files:**
- Modify: `docs/website/index.html` (Shell aus Task 1 ersetzen)
- Modify: `docs/website/css/style.css` (Hero + Highlight-Grid Styles ergänzen)

**Interfaces:**
- Consumes: `nav.js`, `css/style.css`, `img/hero.jpg` (Fallback: `img/placeholder-hero.svg`)

- [ ] **Schritt 1: Hero + Highlight-Grid CSS am Ende von `style.css` ergänzen**

```css
/* ===== STARTSEITE ===== */
.hero {
  position: relative;
  height: 70vh;
  min-height: 420px;
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: hidden;
}
.hero-bg {
  position: absolute;
  inset: 0;
  background-size: cover;
  background-position: center;
  background-color: #1a1008;
}
.hero-overlay {
  position: absolute;
  inset: 0;
  background: linear-gradient(to bottom, rgba(8,6,2,0.35) 0%, rgba(8,6,2,0.75) 100%);
}
.hero-content {
  position: relative;
  text-align: center;
  padding: 2rem;
  max-width: 700px;
}
.hero-title {
  font-family: 'Cinzel', serif;
  font-size: 3.5rem;
  color: var(--accent-gold);
  letter-spacing: 0.15em;
  text-shadow: 0 0 40px rgba(200,150,26,0.55);
  margin-bottom: 0.5rem;
}
.hero-tagline {
  font-style: italic;
  font-size: 1.4rem;
  color: var(--accent-copper);
  margin-bottom: 1.8rem;
}
.highlight-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 1.5rem;
  margin: 2.5rem 0;
}
.highlight-card h3 { color: var(--accent-nixie); margin-top: 0; }
.highlight-card p { color: var(--text-muted); font-size: 0.95rem; margin-bottom: 0; }
.teaser-section { font-size: 1.05rem; }
.teaser-section p:last-child { margin-bottom: 0; }
@media (max-width: 768px) {
  .hero { height: 50vh; }
  .hero-title { font-size: 2rem; letter-spacing: 0.08em; }
  .hero-tagline { font-size: 1.1rem; }
  .highlight-grid { grid-template-columns: 1fr; }
}
```

- [ ] **Schritt 2: Placeholder-Hero SVG erstellen** (Fallback bis `hero.jpg` vorliegt)

Erstelle `docs/website/img/placeholder-hero.svg`:

```svg
<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="600" viewBox="0 0 1200 600">
  <rect width="1200" height="600" fill="#1c1408"/>
  <text x="600" y="270" font-family="serif" font-size="28" fill="#6b4f12" text-anchor="middle">[ hero.jpg hier einsetzen ]</text>
  <text x="600" y="320" font-family="serif" font-size="18" fill="#3a2a10" text-anchor="middle">docs/website/img/hero.jpg</text>
</svg>
```

- [ ] **Schritt 3: `docs/website/index.html` fertigstellen**

```html
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Nixie Clock Ultra</title>
  <link rel="stylesheet" href="css/style.css">
</head>
<body data-page="index">
  <header id="site-header"></header>
  <nav id="site-nav"></nav>
  <main>

    <section class="hero">
      <div class="hero-bg" style="background-image: url('img/hero.jpg'), url('img/placeholder-hero.svg')"></div>
      <div class="hero-overlay"></div>
      <div class="hero-content">
        <h1 class="hero-title">NIXIE CLOCK ULTRA</h1>
        <p class="hero-tagline">wenn Zeit leuchtet</p>
        <a href="features.html" class="btn btn-primary">Mehr erfahren &rarr;</a>
      </div>
    </section>

    <div class="container">
      <div class="highlight-grid" style="margin-top: 2.5rem;">
        <div class="panel highlight-card">
          <h3>Direct Drive</h3>
          <p>Jede Kathode hat ihren eigenen Transistor &mdash; kein Multiplexing, kein Ghosting, kein Flimmern.</p>
        </div>
        <div class="panel highlight-card">
          <h3>WiFi &amp; Web-Interface</h3>
          <p>Vollständige Steuerung über jeden Browser &mdash; kein App-Download, keine Einrichtung.</p>
        </div>
        <div class="panel highlight-card">
          <h3>Nacht-Modus &amp; LDR</h3>
          <p>Zeitgesteuert oder per Lichtsensor &mdash; die Uhr dimmt sich automatisch zur Ruhe.</p>
        </div>
      </div>

      <div class="gear-divider">&#9881;</div>

      <div class="panel teaser-section">
        <p>Nixie-Röhren gehören zu den faszinierendsten Anzeigetechnologien, die je entwickelt wurden: warmes, leicht diffuses Licht, das Ziffern plastisch aus dem Glas herausleuchten lässt &mdash; unverwechselbar, zeitlos und heute begehrter denn je. Die Nixie Clock Ultra bringt dieses Feeling ins 21. Jahrhundert, ohne seinen Charakter zu verlieren.</p>
        <p>Sechs IN-12A-Nixieröhren, RGB-Hintergrundbeleuchtung mit drei Animationsmodi, Slot-Machine-Effekt, vollständiges Web-Interface, IR-Fernbedienung und automatische NTP-Zeitsynchronisation &mdash; in einem handgefertigten Gehäuse.</p>
        <p><a href="features.html">Alle Features entdecken &rarr;</a></p>
      </div>
    </div>

  </main>
  <footer id="site-footer"></footer>
  <script src="js/nav.js"></script>
</body>
</html>
```

- [ ] **Schritt 4: Browser prüfen**

Reload http://localhost:8080
Erwartetes Ergebnis: Hero-Bereich mit dunklem Hintergrund (Placeholder-SVG sichtbar), Titel in Gold, drei Highlight-Kacheln, Teaser-Text, Gear-Divider.

- [ ] **Schritt 5: Commit**

```bash
git add docs/website/index.html docs/website/css/style.css docs/website/img/placeholder-hero.svg
git commit -m "feat: Startseite mit Hero, Highlight-Kacheln, Teaser"
```

---

### Task 4: features.html

**Files:**
- Create: `docs/website/features.html`
- Modify: `docs/website/css/style.css` (Feature-Panel Styles)

**Interfaces:**
- Consumes: `nav.js`, `css/style.css`

- [ ] **Schritt 1: Feature-Styles am Ende von `style.css` ergänzen**

```css
/* ===== FEATURES ===== */
.werbetext { font-size: 1.05rem; line-height: 1.75; margin-bottom: 2.5rem; }
.feature-groups { display: flex; flex-direction: column; gap: 1.2rem; }
.feature-group-label {
  font-family: 'Cinzel', serif;
  font-size: 0.8rem;
  text-transform: uppercase;
  letter-spacing: 0.1em;
  color: var(--accent-nixie);
  margin-bottom: 0.7rem;
}
.feature-list { list-style: none; padding: 0; margin: 0; }
.feature-list li {
  padding: 0.35rem 0 0.35rem 1.6rem;
  position: relative;
  color: var(--text-primary);
  font-size: 1rem;
  line-height: 1.5;
}
.feature-list li::before {
  content: '\203A';
  color: var(--accent-nixie);
  position: absolute;
  left: 0.2rem;
  font-size: 1.3rem;
  line-height: 1.3;
  font-weight: bold;
}
```

- [ ] **Schritt 2: `docs/website/features.html` erstellen**

```html
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Features – Nixie Clock Ultra</title>
  <link rel="stylesheet" href="css/style.css">
</head>
<body data-page="features">
  <header id="site-header"></header>
  <nav id="site-nav"></nav>
  <main>
    <div class="container">
      <div class="page-title"><h1>Features</h1></div>
      <div class="gear-divider">&#9881;</div>

      <div class="panel werbetext">
        <p>Nixie-Röhren gehören zu den faszinierendsten Anzeigetechnologien, die je entwickelt wurden: warmes, leicht diffuses Licht, das Ziffern plastisch aus dem Glas herausleuchten lässt &mdash; unverwechselbar, zeitlos und heute begehrter denn je. Die Nixie Clock Ultra bringt dieses Feeling ins 21. Jahrhundert, ohne seinen Charakter zu verlieren.</p>
        <p>Sechs IN-12A-Nixieröhren zeigen Stunden, Minuten und Sekunden &mdash; und auf Wunsch auch das Datum. Hinter jeder Röhre sitzt eine individuell ansteuerbare RGB-LED, die das Glas von innen in warmem Licht erstrahlen lässt: statisch, im sanften Atemrhythmus oder als fließendes Farbspektrum. Vier weitere LEDs beleuchten die Trennpunkte zwischen den Zifferngruppen.</p>
        <p>Was diese Uhr von den meisten anderen Nixie-Projekten unterscheidet, ist die Art der Röhrenansteuerung: Kein Multiplexing, keine Kompromisse. Jede der 60 Kathoden besitzt ihren eigenen Hochvolt-Transistor &mdash; die Ziffern leuchten dauerhaft, ohne jegliches Ghosting oder Flimmern. Das Ergebnis ist ein sauberes, stabiles Bild, das man sofort sieht und lange nicht vergisst.</p>
        <p>Die Uhr denkt mit: Ein ESP32-S3 verbindet sich mit dem Heimnetz, synchronisiert die Uhrzeit vollautomatisch per NTP aus dem Internet und ist über jeden Browser erreichbar &mdash; kein App-Download, kein Pairing. Wer lieber zur Fernbedienung greift: fast jede handelsübliche IR-Fernbedienung lässt sich anlernen.</p>
        <p>Die Nixie Clock Ultra ist ein Unikat &mdash; handgefertigte Einzelplatinen, sorgfältig bestückt und getestet. Kein Bausatz, keine Kompromisse.</p>
      </div>

      <div class="gear-divider">&#9881;</div>
      <h2>Features auf einen Blick</h2>

      <div class="feature-groups">

        <div class="panel">
          <div class="feature-group-label">Anzeige</div>
          <ul class="feature-list">
            <li>6 &times; IN-12A Nixie-Röhren &mdash; direkte Kathodensteuerung ohne Multiplexing, kein Ghosting, kein Flimmern</li>
            <li>Stunden &middot; Minuten &middot; Sekunden sowie Datumsanzeige (TT MM JJ)</li>
            <li>Sanfter Fade-In beim Einschalten</li>
          </ul>
        </div>

        <div class="panel">
          <div class="feature-group-label">Beleuchtung</div>
          <ul class="feature-list">
            <li>6 &times; WS2812B RGB-LEDs als Röhrenhintergrundbeleuchtung (individuell pro Röhre)</li>
            <li>4 &times; WS2812B RGB-LEDs für die Trennpunkt-Beleuchtung</li>
            <li>Animationsmodi: Rainbow, Statisch (warmweiß), Puls</li>
            <li>Slot-Machine-Effekt: konfigurierbar (aus / alle 10 s / 1 min / 15 min / 1 h), zeigt danach automatisch das Datum</li>
          </ul>
        </div>

        <div class="panel">
          <div class="feature-group-label">Konnektivität &amp; Bedienung</div>
          <ul class="feature-list">
            <li>ESP32-S3 mit integriertem WLAN (2,4 GHz)</li>
            <li>Gleichzeitig eigener WLAN-Accesspoint (kein Heimnetz notwendig) und WLAN-Client</li>
            <li>Automatische Zeitsynchronisation per NTP</li>
            <li>Vollständiges Web-Interface im Browser &mdash; kein App-Download, funktioniert auf Smartphone, Tablet und PC</li>
            <li>Erreichbar per IP oder als <code>nixieclockcs.local</code> im Heimnetz (mDNS)</li>
            <li>IR-Fernbedienung: 8 Funktionen frei auf jede handelsübliche Fernbedienung anlernbar</li>
            <li>4 Bedientaster direkt an der Uhr</li>
          </ul>
        </div>

        <div class="panel">
          <div class="feature-group-label">Nacht-Modus</div>
          <ul class="feature-list">
            <li>Zeitgesteuertes Dimmen oder vollständiges Ausschalten der Röhren</li>
            <li>Automatische Helligkeitsanpassung über eingebauten Lichtsensor (LDR)</li>
            <li>NeoPixel-Helligkeit wird im Nacht-Modus proportional mitgedimmt</li>
          </ul>
        </div>

        <div class="panel">
          <div class="feature-group-label">Technik</div>
          <ul class="feature-list">
            <li>Zweiplatinen-Design: Logic Board (Rev 2.1) + Nixie Display Board (Rev 2.01)</li>
            <li>DS1302 Echtzeituhr mit CR2032-Batterie-Backup &mdash; hält Zeit auch ohne Strom</li>
            <li>Hochvolt-Versorgung: 5 V USB &rarr; DC-DC-Konverter &rarr; 10 V &rarr; HV-Modul &rarr; ~170 V Nixie-Anoden</li>
            <li>60 diskrete SMBTA42-Hochvolt-Transistoren (je eine Kathode, je eine Röhrenziffer)</li>
            <li>Alle Einstellungen bleiben nach Stromunterbrechung erhalten (NVS-Flash)</li>
            <li>Stromversorgung: Micro-USB, 5 V</li>
          </ul>
        </div>

      </div>
    </div>
  </main>
  <footer id="site-footer"></footer>
  <script src="js/nav.js"></script>
</body>
</html>
```

- [ ] **Schritt 3: Browser prüfen**

Öffne http://localhost:8080/features.html
Erwartetes Ergebnis: „Features" im Nav golden (active), Werbetext in Fließtext-Panel, fünf Feature-Gruppen-Panels mit orangefarbenen `›`-Bullets, Cinzel-Gruppenbezeichnungen.

- [ ] **Schritt 4: Commit**

```bash
git add docs/website/features.html docs/website/css/style.css
git commit -m "feat: features.html mit Werbetext und Feature-Gruppen"
```

---

### Task 5: aufbau.html + SVG Spannungsdiagramm

**Files:**
- Create: `docs/website/aufbau.html`
- Modify: `docs/website/css/style.css`

**Interfaces:**
- Consumes: `nav.js`, `css/style.css`, `img/pcb/*.png` (werden in Task 9 kopiert), `lightbox.js` (für klickbare PCB-Bilder, wird in Task 8 erstellt — Bilder vorerst ohne Lightbox)

- [ ] **Schritt 1: Aufbau-Styles am Ende von `style.css` ergänzen**

```css
/* ===== AUFBAU ===== */
.two-col {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 2rem;
  align-items: start;
  margin-bottom: 2rem;
}
.two-col-text h3 { margin-top: 0; }
.pcb-thumbs { display: flex; flex-direction: column; gap: 0.8rem; }
.pcb-thumb {
  width: 100%;
  border: 1px solid var(--border-gold);
  cursor: pointer;
  display: block;
  transition: border-color 0.2s, box-shadow 0.2s;
}
.pcb-thumb:hover {
  border-color: var(--accent-copper);
  box-shadow: 0 0 10px rgba(205,127,50,0.3);
}
.pcb-caption {
  font-size: 0.8rem;
  color: var(--text-muted);
  text-align: center;
  margin-top: 0.3rem;
}
.voltage-diagram {
  background: var(--bg-panel);
  border: 1px solid var(--border-gold);
  border-radius: 2px;
  padding: 2rem;
  text-align: center;
  margin: 1.5rem 0;
  overflow-x: auto;
}
@media (max-width: 768px) {
  .two-col { grid-template-columns: 1fr; }
}
```

- [ ] **Schritt 2: `docs/website/aufbau.html` erstellen**

```html
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Aufbau – Nixie Clock Ultra</title>
  <link rel="stylesheet" href="css/style.css">
</head>
<body data-page="aufbau">
  <header id="site-header"></header>
  <nav id="site-nav"></nav>
  <main>
    <div class="container">
      <div class="page-title"><h1>Aufbau &amp; Hardware</h1></div>
      <div class="gear-divider">&#9881;</div>

      <div class="panel">
        <p>Die Nixie Clock Ultra besteht aus zwei Platinen, die über zwei Steckverbinder verbunden sind: Das <strong>Logic Board</strong> beherbergt den Mikrocontroller, die Echtzeituhr und die Spannungsversorgung. Das <strong>Nixie Display Board</strong> enthält die Anzeigeelektronik mit vier MCP23017 Port-Expandern, 60 Hochvolt-Transistoren und den WS2812B RGB-LEDs.</p>
      </div>

      <!-- Logic Board -->
      <h2>Logic Board (Rev 2.1)</h2>
      <div class="two-col">
        <div class="pcb-thumbs">
          <div>
            <img class="pcb-thumb" src="img/pcb/logic_sch-1.png" alt="Logic Board Schaltplan" data-lightbox="pcb">
            <p class="pcb-caption">Schaltplan</p>
          </div>
          <div>
            <img class="pcb-thumb" src="img/pcb/logic_pcb-1.png" alt="Logic Board PCB Oberseite" data-lightbox="pcb">
            <p class="pcb-caption">PCB-Layout Oberseite</p>
          </div>
          <div>
            <img class="pcb-thumb" src="img/pcb/logic_pcb-2.png" alt="Logic Board PCB Unterseite" data-lightbox="pcb">
            <p class="pcb-caption">PCB-Layout Unterseite</p>
          </div>
        </div>
        <div class="two-col-text">
          <h3>Komponenten</h3>
          <p>Herzstück ist der <strong>ESP32-S3-WROOM-1</strong> (U1) mit integriertem WLAN. Die batteriegepufferte <strong>DS1302 Echtzeituhr</strong> (U2) hält die Zeit auch bei Stromausfall. Der <strong>VS1838B</strong> (U3) empfängt und demoduliert IR-Signale.</p>
          <p>Die Spannungsversorgung erfolgt dreistufig: <strong>AMS1117-3.3</strong> (U5) liefert 3,3 V für die Logik; der <strong>DC-DC-Konverter</strong> (U6) hebt 5 V auf 10 V an; das <strong>HV-Modul</strong> (U4) erzeugt daraus ~170 V für die Nixie-Anoden.</p>
          <p>Neu in Rev 2.1: Lichtsensor-Anschluss <strong>J5</strong> (LDR, GPIO6) für den automatischen Nacht-Modus.</p>
          <div class="warning-box">
            <strong>USB-CDC-Hinweis:</strong> Der ESP32-S3 verwendet native USB (kein UART-Chip). In der Arduino IDE muss <strong>USB CDC On Boot: Enabled</strong> gesetzt sein &mdash; sonst ist nach dem ersten Flash keine erneute Programmierung über USB möglich (BOOT-Taster nicht bestückt).
          </div>
        </div>
      </div>

      <div class="gear-divider">&#9881;</div>

      <!-- Nixie Display Board -->
      <h2>Nixie Display Board (Rev 2.01)</h2>
      <div class="two-col">
        <div class="pcb-thumbs">
          <div>
            <img class="pcb-thumb" src="img/pcb/nixie_sch-1.png" alt="Nixie Display Board Schaltplan" data-lightbox="pcb">
            <p class="pcb-caption">Schaltplan</p>
          </div>
          <div>
            <img class="pcb-thumb" src="img/pcb/nixie_pcb-1.png" alt="Nixie Display Board PCB Oberseite" data-lightbox="pcb">
            <p class="pcb-caption">PCB-Layout Oberseite</p>
          </div>
          <div>
            <img class="pcb-thumb" src="img/pcb/nixie_pcb-2.png" alt="Nixie Display Board PCB Unterseite" data-lightbox="pcb">
            <p class="pcb-caption">PCB-Layout Unterseite</p>
          </div>
        </div>
        <div class="two-col-text">
          <h3>Direct Drive &mdash; kein Ghosting</h3>
          <p>Vier <strong>MCP23017</strong> I²C-Port-Expander (U1&ndash;U4, Adressen 0x20&ndash;0x23) stellen 64 digitale Ausgänge bereit. 60 davon steuern je einen <strong>SMBTA42</strong>-NPN-Transistor (300 V), der eine Nixie-Kathode direkt auf GND zieht.</p>
          <p>Da jede Kathode ihren eigenen Transistor besitzt, können mehrere Röhren gleichzeitig leuchten &mdash; kein Multiplexing, keine gemeinsamen Pfade, kein Ghosting.</p>
          <p>Sechs <strong>WS2812B-SMD-LEDs</strong> (Pixel 0&ndash;5, GRB) beleuchten die Röhren von hinten. Vier <strong>WS2812B-THT-LEDs YF923</strong> (Pixel 6&ndash;9, RGB) dienen als Trennpunkt-LEDs.</p>
        </div>
      </div>

      <div class="gear-divider">&#9881;</div>

      <!-- Spannungsversorgung -->
      <h2>Spannungsversorgung</h2>
      <div class="voltage-diagram">
        <svg width="640" height="130" viewBox="0 0 640 130" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="Spannungsfluss-Diagramm">
          <!-- Farben -->
          <!-- Boxen: fill=#1c1408, stroke=#6b4f12 -->
          <!-- Pfeile: stroke=#c8961a -->
          <!-- Text: fill=#e8d5a3, Labels: fill=#ff8c00 -->

          <!-- Zeile 1: USB → DC-DC → HV-MOD → Nixie -->
          <!-- USB Box -->
          <rect x="10" y="10" width="80" height="44" rx="2" fill="#1c1408" stroke="#6b4f12" stroke-width="1.5"/>
          <text x="50" y="28" font-family="Cinzel,serif" font-size="11" fill="#c8961a" text-anchor="middle">USB</text>
          <text x="50" y="46" font-family="Cinzel,serif" font-size="11" fill="#e8d5a3" text-anchor="middle">5 V</text>

          <!-- Pfeil USB → DC-DC -->
          <line x1="90" y1="32" x2="148" y2="32" stroke="#c8961a" stroke-width="1.5" marker-end="url(#arr)"/>

          <!-- DC-DC Box -->
          <rect x="150" y="10" width="90" height="44" rx="2" fill="#1c1408" stroke="#6b4f12" stroke-width="1.5"/>
          <text x="195" y="28" font-family="Cinzel,serif" font-size="10" fill="#c8961a" text-anchor="middle">DC-DC</text>
          <text x="195" y="46" font-family="Cinzel,serif" font-size="10" fill="#a08c6a" text-anchor="middle">U6</text>

          <!-- Label 10V -->
          <text x="270" y="26" font-family="serif" font-size="10" fill="#ff8c00" text-anchor="middle">10 V</text>
          <!-- Pfeil DC-DC → HV-MOD -->
          <line x1="240" y1="32" x2="298" y2="32" stroke="#c8961a" stroke-width="1.5" marker-end="url(#arr)"/>

          <!-- HV-MOD Box -->
          <rect x="300" y="10" width="90" height="44" rx="2" fill="#1c1408" stroke="#6b4f12" stroke-width="1.5"/>
          <text x="345" y="28" font-family="Cinzel,serif" font-size="10" fill="#c8961a" text-anchor="middle">HV-MOD</text>
          <text x="345" y="46" font-family="Cinzel,serif" font-size="10" fill="#a08c6a" text-anchor="middle">U4</text>

          <!-- Label ~170V -->
          <text x="420" y="26" font-family="serif" font-size="10" fill="#ff8c00" text-anchor="middle">~170 V</text>
          <!-- Pfeil HV-MOD → Nixie -->
          <line x1="390" y1="32" x2="448" y2="32" stroke="#c8961a" stroke-width="1.5" marker-end="url(#arr)"/>

          <!-- Nixie Box -->
          <rect x="450" y="10" width="90" height="44" rx="2" fill="#1c1408" stroke="#6b4f12" stroke-width="1.5"/>
          <text x="495" y="28" font-family="Cinzel,serif" font-size="10" fill="#c8961a" text-anchor="middle">Nixie</text>
          <text x="495" y="46" font-family="Cinzel,serif" font-size="10" fill="#e8d5a3" text-anchor="middle">Anoden</text>

          <!-- Zeile 2: USB → AMS1117 → Logik -->
          <!-- Linie von USB nach unten -->
          <line x1="50" y1="54" x2="50" y2="86" stroke="#c8961a" stroke-width="1.5"/>
          <line x1="50" y1="86" x2="148" y2="86" stroke="#c8961a" stroke-width="1.5" marker-end="url(#arr)"/>

          <!-- AMS1117 Box -->
          <rect x="150" y="64" width="90" height="44" rx="2" fill="#1c1408" stroke="#6b4f12" stroke-width="1.5"/>
          <text x="195" y="82" font-family="Cinzel,serif" font-size="10" fill="#c8961a" text-anchor="middle">AMS1117</text>
          <text x="195" y="100" font-family="Cinzel,serif" font-size="10" fill="#a08c6a" text-anchor="middle">U5</text>

          <!-- Label 3,3V -->
          <text x="270" y="80" font-family="serif" font-size="10" fill="#ff8c00" text-anchor="middle">3,3 V</text>
          <!-- Pfeil AMS1117 → Logik -->
          <line x1="240" y1="86" x2="298" y2="86" stroke="#c8961a" stroke-width="1.5" marker-end="url(#arr)"/>

          <!-- Logik Box -->
          <rect x="300" y="64" width="90" height="44" rx="2" fill="#1c1408" stroke="#6b4f12" stroke-width="1.5"/>
          <text x="345" y="82" font-family="Cinzel,serif" font-size="10" fill="#c8961a" text-anchor="middle">Logik</text>
          <text x="345" y="100" font-family="Cinzel,serif" font-size="10" fill="#a08c6a" text-anchor="middle">ESP32-S3</text>

          <!-- Pfeilspitze -->
          <defs>
            <marker id="arr" markerWidth="8" markerHeight="8" refX="6" refY="3" orient="auto">
              <path d="M0,0 L0,6 L8,3 Z" fill="#c8961a"/>
            </marker>
          </defs>
        </svg>
      </div>

    </div>
  </main>
  <footer id="site-footer"></footer>
  <script src="js/nav.js"></script>
  <script src="js/lightbox.js"></script>
</body>
</html>
```

- [ ] **Schritt 3: Browser prüfen**

Öffne http://localhost:8080/aufbau.html
Erwartetes Ergebnis: „Aufbau" im Nav aktiv, PCB-Thumbnails (grauer Platzhalter bis Bilder kopiert), SVG-Spannungsdiagramm mit goldenen Pfeilen und zwei Zeilen (5V→10V→170V oben, 5V→3,3V unten).

- [ ] **Schritt 4: Commit**

```bash
git add docs/website/aufbau.html docs/website/css/style.css
git commit -m "feat: aufbau.html mit PCB-Sektionen und SVG-Spannungsdiagramm"
```

---

### Task 6: geschichte.html — Timeline + CSS-Akkordeon

**Files:**
- Create: `docs/website/geschichte.html`
- Modify: `docs/website/css/style.css`

**Interfaces:**
- Consumes: `nav.js`, `css/style.css`
- Inhalt basiert auf `docs/ENTWICKLUNGSHISTORIE.md` (Phasen 1–7)

- [ ] **Schritt 1: Timeline + Akkordeon CSS am Ende von `style.css` ergänzen**

```css
/* ===== GESCHICHTE ===== */
.timeline {
  position: relative;
  padding-left: 2.8rem;
  margin: 2rem 0;
}
.timeline::before {
  content: '';
  position: absolute;
  left: 0.75rem;
  top: 0.5rem;
  bottom: 0.5rem;
  width: 2px;
  background: linear-gradient(to bottom, var(--accent-gold), var(--border-gold));
}
.timeline-item {
  position: relative;
  margin-bottom: 2.8rem;
}
.timeline-item::before {
  content: '';
  position: absolute;
  left: -2.3rem;
  top: 0.45rem;
  width: 14px;
  height: 14px;
  border-radius: 50%;
  background: var(--accent-gold);
  border: 3px solid var(--bg-base);
  box-shadow: 0 0 8px rgba(200,150,26,0.5);
}
.timeline-date {
  font-family: 'Cinzel', serif;
  font-size: 0.78rem;
  letter-spacing: 0.08em;
  color: var(--accent-nixie);
  margin-bottom: 0.2rem;
  text-transform: uppercase;
}
.timeline-title {
  font-family: 'Cinzel', serif;
  font-size: 1.05rem;
  color: var(--accent-gold);
  margin-bottom: 0.5rem;
}
.timeline-body { font-size: 0.98rem; color: var(--text-muted); }
.timeline-body p { margin-bottom: 0.5rem; }

/* CSS-only Akkordeon */
.akkordeon { margin-top: 0.6rem; }
.akkordeon input[type="checkbox"] { display: none; }
.akkordeon-label {
  display: flex;
  justify-content: space-between;
  align-items: center;
  cursor: pointer;
  padding: 0.6rem 1rem;
  background: #1a1008;
  border: 1px solid var(--border-dim);
  color: var(--text-primary);
  font-family: 'Cinzel', serif;
  font-size: 0.82rem;
  letter-spacing: 0.04em;
  margin-top: 0.4rem;
  user-select: none;
  transition: background 0.15s;
}
.akkordeon-label:hover { background: var(--bg-panel-hover); }
.akkordeon-arrow {
  color: var(--accent-gold);
  font-size: 0.7rem;
  transition: transform 0.2s;
  display: inline-block;
}
.akkordeon input:checked + .akkordeon-label .akkordeon-arrow { transform: rotate(90deg); }
.akkordeon-content {
  display: none;
  padding: 0.9rem 1rem;
  border: 1px solid var(--border-dim);
  border-top: none;
  background: var(--bg-panel);
  font-size: 0.95rem;
  color: var(--text-muted);
  line-height: 1.6;
}
.akkordeon input:checked ~ .akkordeon-content { display: block; }
```

- [ ] **Schritt 2: `docs/website/geschichte.html` erstellen**

```html
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Geschichte – Nixie Clock Ultra</title>
  <link rel="stylesheet" href="css/style.css">
</head>
<body data-page="geschichte">
  <header id="site-header"></header>
  <nav id="site-nav"></nav>
  <main>
    <div class="container">
      <div class="page-title"><h1>Entwicklungsgeschichte</h1></div>
      <div class="gear-divider">&#9881;</div>
      <div class="panel">
        <p>Die Nixie Clock Ultra entstand in sieben Entwicklungsphasen zwischen April und Juni 2026. Jede Phase brachte neue Erkenntnisse &mdash; und neue Probleme, die es zu lösen galt.</p>
      </div>

      <div class="timeline">

        <div class="timeline-item">
          <div class="timeline-date">2026-04-22</div>
          <div class="timeline-title">Phase 1 &mdash; Firmware-Grundgerüst</div>
          <div class="timeline-body">
            <p>Vollständige Basis-Firmware in einer einzigen <code>.ino</code>-Datei: Nixie-Ansteuerung via Multiplex-ISR, NeoPixel-Animationen, Web-Interface (WiFi AP, AsyncWebServer), IR-Fernbedienung mit 7 Funktionen, DS1302 RTC-Integration.</p>
          </div>
        </div>

        <div class="timeline-item">
          <div class="timeline-date">2026-04-23</div>
          <div class="timeline-title">Phase 2 &mdash; Modularisierung</div>
          <div class="timeline-body">
            <p>Die wachsende Datei wurde in sieben Module aufgeteilt: <code>rtc.ino</code>, <code>display.ino</code>, <code>neo_animation.ino</code>, <code>buttons.ino</code>, <code>ir_remote.ino</code>, <code>web_server.ino</code> und den Multiplex-Timer.</p>
            <div class="akkordeon">
              <input type="checkbox" id="acc-1">
              <label class="akkordeon-label" for="acc-1">Problem: Forward Declarations <span class="akkordeon-arrow">&#9654;</span></label>
              <div class="akkordeon-content">Der Arduino-Präprozessor erkennt Forward Declarations für <code>IRAM_ATTR</code>-Funktionen und Web-Server-Callbacks nicht automatisch. Lösung: Forward Declarations manuell in <code>NixieClockUltra.ino</code> ergänzt.</div>
            </div>
          </div>
        </div>

        <div class="timeline-item">
          <div class="timeline-date">2026-05-03</div>
          <div class="timeline-title">Phase 3 &mdash; Hardware-Inbetriebnahme</div>
          <div class="timeline-body">
            <p>Erste Tests an der echten Hardware brachten mehrere Korrekturen: RGB-Bytereihenfolge der THT-WS2812B (RGB statt GRB), Kathoden-Mapping-Korrektur, separate Helligkeitsregelung für Hintergrund und Trennpunkte.</p>
            <div class="akkordeon">
              <input type="checkbox" id="acc-2">
              <label class="akkordeon-label" for="acc-2">Problem: NeoPixel RGB-Swap <span class="akkordeon-arrow">&#9654;</span></label>
              <div class="akkordeon-content">Die 4 Trennpunkt-LEDs (THT WS2812B YF923) zeigten falsche Farben. Ursache: THT-Variante verwendet RGB-Bytereihenfolge statt GRB wie die SMD-Version. Lösung: <code>rgbSwap</code> für Pixel 6&ndash;9 in <code>neo_animation.ino</code>.</div>
            </div>
            <div class="akkordeon">
              <input type="checkbox" id="acc-3">
              <label class="akkordeon-label" for="acc-3">Problem: Ghosting (erste Versuche) <span class="akkordeon-arrow">&#9654;</span></label>
              <div class="akkordeon-content">Nixie-Röhren zeigten Geisterziffern durch den Multiplex-Betrieb. Zwei Versuche: (1) Anode vor Kathode schalten &mdash; kein Erfolg. (2) Blank-Phase verlängern &mdash; reduziert, aber nicht beseitigt. Strukturelle Lösung erst in Phase 4.</div>
            </div>
          </div>
        </div>

        <div class="timeline-item">
          <div class="timeline-date">2026-05-09</div>
          <div class="timeline-title">Phase 4 &mdash; Anti-Ghosting: MCP23017 Direct Drive</div>
          <div class="timeline-body">
            <p>Kompletter Umbau der Nixie-Ansteuerung: statt eines 74HC595-Schieberegisters mit Multiplex-ISR nun vier MCP23017 I²C-Port-Expander mit je eigenem SMBTA42-Transistor pro Kathode. Kein Multiplexing mehr &mdash; alle Röhren leuchten dauerhaft.</p>
            <div class="akkordeon">
              <input type="checkbox" id="acc-4">
              <label class="akkordeon-label" for="acc-4">Ergebnis (getestet 2026-06-15) <span class="akkordeon-arrow">&#9654;</span></label>
              <div class="akkordeon-content">Ghosting vollständig beseitigt. Die direkte Ansteuerung via MCP23017 löst das Problem strukturell: Da keine Kathode mehr gemeinsam genutzt wird, gibt es keine Übersprecher zwischen den Röhren.</div>
            </div>
          </div>
        </div>

        <div class="timeline-item">
          <div class="timeline-date">2026-06-15</div>
          <div class="timeline-title">Phase 5 &mdash; Dokumentation</div>
          <div class="timeline-body">
            <p>Vollständige Bedienungsanleitung im Steampunk-Design (HTML &rarr; PDF) sowie technische Systemdokumentation (ODT) mit eingebetteten Schaltplänen, API-Dokumentation und Bibliotheksübersicht.</p>
          </div>
        </div>

        <div class="timeline-item">
          <div class="timeline-date">2026-06-17</div>
          <div class="timeline-title">Phase 6 &mdash; Slot-Intervall &amp; Feinschliff</div>
          <div class="timeline-body">
            <p>Slot-Machine-Animation als eigenständige Einstellung (<code>SlotInterval</code>-Enum) ausgelagert. Mehrere Hardware-Bugs behoben: IR-Empfang nach <code>strip.show()</code>, Taster-Entprellung, Pin-Korrekturen, Warmweiß-Farbabgleich.</p>
            <div class="akkordeon">
              <input type="checkbox" id="acc-5">
              <label class="akkordeon-label" for="acc-5">Problem: IR-Empfang nach NeoPixel-Update <span class="akkordeon-arrow">&#9654;</span></label>
              <div class="akkordeon-content">IR-Codes wurden nach <code>strip.show()</code> nicht mehr empfangen. Ursache: <code>irrecv.pause()/resume()</code> um <code>strip.show()</code> resettete den RMT-Empfänger alle 20 ms. Lösung: pause/resume vollständig entfernt. Auf dem ESP32-S3 sind RMT-TX (NeoPixel) und RMT-RX (IR) unabhängige Kanäle &mdash; kein Konflikt.</div>
            </div>
            <div class="akkordeon">
              <input type="checkbox" id="acc-6">
              <label class="akkordeon-label" for="acc-6">Problem: Taster-Entprellung <span class="akkordeon-arrow">&#9654;</span></label>
              <div class="akkordeon-content"><code>pressed</code> wurde nie <code>true</code>. Ursache: Logikfehler in der Button-FSM &mdash; das <code>debounced</code>-Flag wurde nie gesetzt, weil die Bedingung <code>lastState == HIGH</code> innerhalb des Debounce-Fensters niemals zutraf. Lösung: <code>Button</code>-Struct um <code>debounced</code>-Flag erweitert.</div>
            </div>
          </div>
        </div>

        <div class="timeline-item">
          <div class="timeline-date">2026-06-20</div>
          <div class="timeline-title">Phase 7 &mdash; WiFi-Fixes &amp; mDNS</div>
          <div class="timeline-body">
            <p>WiFi-Scan entfernt (Blocking-Probleme mit ESPAsyncWebServer), STA-Timeout auf 20 s erhöht, DHCP-Hostname und mDNS implementiert. Die Uhr ist jetzt als <code>nixieclockcs.local</code> im Heimnetz erreichbar.</p>
            <div class="akkordeon">
              <input type="checkbox" id="acc-7">
              <label class="akkordeon-label" for="acc-7">Problem: WiFi-Scan lieferte keine Ergebnisse <span class="akkordeon-arrow">&#9654;</span></label>
              <div class="akkordeon-content">Zwei Ursachen: (1) <code>WiFi.mode(WIFI_AP)</code> deaktiviert das STA-Radio &mdash; kein Scan möglich. (2) Synchroner <code>scanNetworks()</code>-Aufruf im ESPAsyncWebServer-Handler blockiert den LWIP/WiFi-Task. Lösung: Scan-Funktion vollständig entfernt, SSID wird manuell eingegeben.</div>
            </div>
          </div>
        </div>

      </div><!-- /timeline -->
    </div>
  </main>
  <footer id="site-footer"></footer>
  <script src="js/nav.js"></script>
</body>
</html>
```

- [ ] **Schritt 3: Browser prüfen**

Öffne http://localhost:8080/geschichte.html
Erwartetes Ergebnis: Vertikale goldene Timeline-Linie, 7 Meilenstein-Einträge mit goldenen Punkten, klappbare Akkordeon-Elemente (Klick auf Label öffnet/schließt Problemtext).

- [ ] **Schritt 4: Commit**

```bash
git add docs/website/geschichte.html docs/website/css/style.css
git commit -m "feat: geschichte.html mit Timeline und CSS-Akkordeon"
```

---

### Task 7: dokumentation.html — Download-Karten

**Files:**
- Create: `docs/website/dokumentation.html`
- Modify: `docs/website/css/style.css`

**Interfaces:**
- Consumes: `nav.js`, `css/style.css`, `downloads/` (Dateien werden in Task 9 kopiert)

- [ ] **Schritt 1: Download-Karten-CSS am Ende von `style.css` ergänzen**

```css
/* ===== DOKUMENTATION ===== */
.download-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 2rem;
  margin: 2rem 0;
}
.download-card {
  background: var(--bg-panel);
  border: 1px solid var(--border-gold);
  border-radius: 2px;
  padding: 2rem 1.8rem;
  text-align: center;
  position: relative;
  display: flex;
  flex-direction: column;
  align-items: center;
}
.download-card::before { content: '⬤'; color: var(--border-gold); font-size: 0.35rem; position: absolute; top: 0.5rem; left: 0.5rem; }
.download-card::after  { content: '⬤'; color: var(--border-gold); font-size: 0.35rem; position: absolute; bottom: 0.5rem; right: 0.5rem; }
.download-icon { width: 52px; height: 52px; margin-bottom: 1rem; color: var(--accent-gold); }
.download-title {
  font-family: 'Cinzel', serif;
  font-size: 1.15rem;
  color: var(--accent-gold);
  margin-bottom: 0.7rem;
}
.download-desc {
  font-size: 0.95rem;
  color: var(--text-muted);
  margin-bottom: 1.5rem;
  line-height: 1.55;
  flex: 1;
}
@media (max-width: 768px) { .download-grid { grid-template-columns: 1fr; } }
```

- [ ] **Schritt 2: `docs/website/dokumentation.html` erstellen**

```html
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Dokumentation – Nixie Clock Ultra</title>
  <link rel="stylesheet" href="css/style.css">
</head>
<body data-page="dokumentation">
  <header id="site-header"></header>
  <nav id="site-nav"></nav>
  <main>
    <div class="container">
      <div class="page-title"><h1>Dokumentation &amp; Downloads</h1></div>
      <div class="gear-divider">&#9881;</div>

      <div class="download-grid">

        <div class="download-card">
          <!-- Zahnrad-Icon -->
          <svg class="download-icon" viewBox="0 0 100 100" fill="none" xmlns="http://www.w3.org/2000/svg" aria-hidden="true">
            <path d="M43 5h14l2 11a34 34 0 0 1 8 4.5l10-3.5 9.9 9.9-3.5 10A34 34 0 0 1 88 45l11 2v14l-11 2a34 34 0 0 1-4.5 8l3.5 10-9.9 9.9-10-3.5A34 34 0 0 1 59 91l-2 9H43l-2-9a34 34 0 0 1-8-4.5l-10 3.5-9.9-9.9 3.5-10A34 34 0 0 1 12 57L1 55V41l11-2a34 34 0 0 1 4.5-8l-3.5-10 9.9-9.9 10 3.5A34 34 0 0 1 41 10Z" fill="none" stroke="currentColor" stroke-width="5"/>
            <circle cx="50" cy="50" r="16" fill="none" stroke="currentColor" stroke-width="5"/>
          </svg>
          <div class="download-title">Systemdokumentation</div>
          <p class="download-desc">Vollständige technische Dokumentation: Hardware-Aufbau beider Platinen, Schaltpläne, Firmware-Architektur, API-Endpoints, NVS-Persistenz und Entwickler-Workflow.</p>
          <div class="btn-group">
            <a href="downloads/NixieClockUltra-Systemdokumentation.odt" class="btn" download>ODT</a>
            <a href="downloads/NixieClockUltra-Systemdokumentation.pdf" class="btn btn-primary" download>PDF</a>
          </div>
        </div>

        <div class="download-card">
          <!-- Buch-Icon -->
          <svg class="download-icon" viewBox="0 0 100 100" fill="none" xmlns="http://www.w3.org/2000/svg" aria-hidden="true">
            <rect x="15" y="10" width="55" height="75" rx="3" fill="none" stroke="currentColor" stroke-width="5"/>
            <line x1="15" y1="85" x2="75" y2="85" stroke="currentColor" stroke-width="5"/>
            <rect x="70" y="10" width="15" height="75" rx="2" fill="none" stroke="currentColor" stroke-width="4"/>
            <line x1="30" y1="30" x2="60" y2="30" stroke="currentColor" stroke-width="3.5"/>
            <line x1="30" y1="43" x2="60" y2="43" stroke="currentColor" stroke-width="3.5"/>
            <line x1="30" y1="56" x2="50" y2="56" stroke="currentColor" stroke-width="3.5"/>
          </svg>
          <div class="download-title">Bedienungsanleitung</div>
          <p class="download-desc">Vollständige Bedienungsanleitung im Steampunk-Design: Inbetriebnahme, Tastenbedienung, Zeiteinstellung, Beleuchtungssteuerung, IR-Fernbedienung, WLAN-Einrichtung und technische Daten.</p>
          <div class="btn-group">
            <a href="downloads/NixieClockUltra-Bedienungsanleitung.pdf" class="btn btn-primary" download>PDF</a>
          </div>
        </div>

      </div>

      <div class="note-box">
        &#9432; ODT-Dateien können mit <a href="https://www.libreoffice.org" target="_blank" rel="noopener">LibreOffice</a> (kostenlos) geöffnet werden.
      </div>

    </div>
  </main>
  <footer id="site-footer"></footer>
  <script src="js/nav.js"></script>
</body>
</html>
```

- [ ] **Schritt 3: Browser prüfen**

Öffne http://localhost:8080/dokumentation.html
Erwartetes Ergebnis: Zwei Download-Karten nebeneinander, SVG-Icons in Gold, Buttons mit Goldrahmen. Download-Links zeigen auf `downloads/` (Dateien noch nicht kopiert — das folgt in Task 9).

- [ ] **Schritt 4: Commit**

```bash
git add docs/website/dokumentation.html docs/website/css/style.css
git commit -m "feat: dokumentation.html mit Download-Karten"
```

---

### Task 8: galerie.html + lightbox.js

**Files:**
- Create: `docs/website/galerie.html`
- Create: `docs/website/js/lightbox.js`
- Modify: `docs/website/css/style.css`

**Interfaces:**
- Consumes: `nav.js`, `css/style.css`, `img/gallery/*.jpg` (vom Nutzer bereitzustellen)
- Produces: `lightbox.js` exportiert keine Funktionen — aktiviert sich per `data-lightbox`-Attribut auf `<img>`-Tags

- [ ] **Schritt 1: Galerie + Lightbox CSS am Ende von `style.css` ergänzen**

```css
/* ===== GALERIE ===== */
.gallery-grid {
  columns: 3;
  column-gap: 1rem;
  margin: 2rem 0;
}
.gallery-item {
  break-inside: avoid;
  margin-bottom: 1rem;
  cursor: pointer;
  display: block;
}
.gallery-item img {
  width: 100%;
  display: block;
  border: 2px solid var(--border-gold);
  transition: border-color 0.2s, box-shadow 0.2s;
}
.gallery-item:hover img {
  border-color: var(--accent-copper);
  box-shadow: 0 0 14px rgba(205,127,50,0.35);
}
.gallery-empty {
  color: var(--text-muted);
  font-style: italic;
  text-align: center;
  padding: 3rem 0;
}
/* Lightbox */
.lightbox {
  display: none;
  position: fixed;
  inset: 0;
  background: rgba(0,0,0,0.93);
  z-index: 1000;
  align-items: center;
  justify-content: center;
}
.lightbox.open { display: flex; }
.lightbox-img {
  max-width: 90vw;
  max-height: 88vh;
  object-fit: contain;
  border: 1px solid var(--border-gold);
  display: block;
}
.lightbox-close {
  position: absolute;
  top: 1rem;
  right: 1.5rem;
  font-size: 2.2rem;
  color: var(--accent-gold);
  cursor: pointer;
  background: none;
  border: none;
  line-height: 1;
  padding: 0.2rem 0.5rem;
}
.lightbox-close:hover { color: var(--accent-nixie); }
.lightbox-prev,
.lightbox-next {
  position: absolute;
  top: 50%;
  transform: translateY(-50%);
  font-size: 1.8rem;
  color: var(--accent-gold);
  cursor: pointer;
  background: rgba(14,10,5,0.75);
  border: 1px solid var(--border-gold);
  padding: 0.6rem 0.9rem;
  line-height: 1;
  transition: background 0.2s;
}
.lightbox-prev:hover,
.lightbox-next:hover { background: rgba(200,150,26,0.15); }
.lightbox-prev { left: 1rem; }
.lightbox-next { right: 1rem; }
@media (max-width: 1024px) { .gallery-grid { columns: 2; } }
@media (max-width: 600px)  { .gallery-grid { columns: 1; } }
```

- [ ] **Schritt 2: `docs/website/js/lightbox.js` erstellen**

```javascript
(function () {
  let images = [];
  let current = 0;

  const box   = document.createElement('div');
  box.className = 'lightbox';
  box.innerHTML = `
    <button class="lightbox-close" aria-label="Schließen">&times;</button>
    <button class="lightbox-prev" aria-label="Vorheriges Bild">&#8592;</button>
    <img class="lightbox-img" src="" alt="">
    <button class="lightbox-next" aria-label="Nächstes Bild">&#8594;</button>`;
  document.body.appendChild(box);

  const img  = box.querySelector('.lightbox-img');
  const prev = box.querySelector('.lightbox-prev');
  const next = box.querySelector('.lightbox-next');

  function show(index) {
    current = (index + images.length) % images.length;
    img.src = images[current].src;
    img.alt = images[current].alt;
    box.classList.add('open');
    document.body.style.overflow = 'hidden';
  }

  function hide() {
    box.classList.remove('open');
    document.body.style.overflow = '';
  }

  prev.addEventListener('click', () => show(current - 1));
  next.addEventListener('click', () => show(current + 1));
  box.querySelector('.lightbox-close').addEventListener('click', hide);
  box.addEventListener('click', (e) => { if (e.target === box) hide(); });

  document.addEventListener('keydown', (e) => {
    if (!box.classList.contains('open')) return;
    if (e.key === 'Escape')     hide();
    if (e.key === 'ArrowLeft')  show(current - 1);
    if (e.key === 'ArrowRight') show(current + 1);
  });

  document.addEventListener('DOMContentLoaded', () => {
    images = Array.from(document.querySelectorAll('[data-lightbox]'));
    images.forEach((el, i) => {
      el.style.cursor = 'pointer';
      el.addEventListener('click', () => show(i));
    });
  });
})();
```

- [ ] **Schritt 3: `docs/website/galerie.html` erstellen**

```html
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Galerie – Nixie Clock Ultra</title>
  <link rel="stylesheet" href="css/style.css">
</head>
<body data-page="galerie">
  <header id="site-header"></header>
  <nav id="site-nav"></nav>
  <main>
    <div class="container">
      <div class="page-title"><h1>Galerie</h1></div>
      <div class="gear-divider">&#9881;</div>

      <!-- Fotos hier eintragen sobald img/gallery/ befüllt ist.
           Jedes Bild als:
           <a class="gallery-item" href="img/gallery/dateiname.jpg">
             <img src="img/gallery/dateiname.jpg" alt="Kurze Bildbeschreibung" data-lightbox="gallery">
           </a>
      -->
      <div class="gallery-grid" id="gallery-grid">
        <p class="gallery-empty">Fotos werden hier eingetragen sobald img/gallery/ befüllt ist.<br>
        Jede Fotodatei als <code>&lt;img src="img/gallery/xxx.jpg" data-lightbox="gallery"&gt;</code> in die gallery-grid div einfügen.</p>
      </div>

    </div>
  </main>
  <footer id="site-footer"></footer>
  <script src="js/nav.js"></script>
  <script src="js/lightbox.js"></script>
</body>
</html>
```

- [ ] **Schritt 4: Lightbox auf aufbau.html testen**

Die PCB-Thumbnails in `aufbau.html` haben bereits `data-lightbox="pcb"`. Öffne http://localhost:8080/aufbau.html — sobald PCB-Bilder in Task 9 kopiert sind, ist die Lightbox dort funktionsfähig.

- [ ] **Schritt 5: Commit**

```bash
git add docs/website/galerie.html docs/website/js/lightbox.js docs/website/css/style.css
git commit -m "feat: galerie.html + lightbox.js (Vanilla JS)"
```

---

### Task 9: Assets kopieren + README für Nutzer-Fotos

**Files:**
- Copy: PCB-Bilder → `docs/website/img/pcb/`
- Copy: Downloads → `docs/website/downloads/`
- Create: `docs/website/img/gallery/README.md`

**Interfaces:**
- Versorgt `aufbau.html` (PCB-Bilder) und `dokumentation.html` (Downloads) mit echten Dateien

- [ ] **Schritt 1: PCB-Bilder kopieren**

```bash
cp docs/system/assets/logic_sch-1.png   docs/website/img/pcb/
cp docs/system/assets/logic_pcb-1.png   docs/website/img/pcb/
cp docs/system/assets/logic_pcb-2.png   docs/website/img/pcb/
cp docs/system/assets/nixie_sch-1.png   docs/website/img/pcb/
cp docs/system/assets/nixie_pcb-1.png   docs/website/img/pcb/
cp docs/system/assets/nixie_pcb-2.png   docs/website/img/pcb/
```

- [ ] **Schritt 2: Download-Dateien kopieren**

```bash
cp docs/system/NixieClockUltra-Systemdokumentation.odt  docs/website/downloads/
cp docs/system/NixieClockUltra-Systemdokumentation.pdf  docs/website/downloads/
cp "docs/manual/Nixie Clock Ultra – Bedienungsanleitung.pdf" docs/website/downloads/NixieClockUltra-Bedienungsanleitung.pdf
```

- [ ] **Schritt 3: README für Galerie-Fotos erstellen**

Erstelle `docs/website/img/gallery/README.md`:

```markdown
# Galerie-Fotos

Fotos der fertigen Nixie Clock Ultra hier ablegen (JPG, empfohlen min. 1200px breit).

Danach in `docs/website/galerie.html` im `<div id="gallery-grid">` eintragen:

```html
<a class="gallery-item" href="img/gallery/dateiname.jpg">
  <img src="img/gallery/dateiname.jpg"
       alt="Kurze Bildbeschreibung"
       data-lightbox="gallery">
</a>
```

Das Placeholder-Bild für die Hero-Sektion kommt nach `docs/website/img/hero.jpg`.
```

- [ ] **Schritt 4: Vollständig prüfen**

Öffne alle Seiten nacheinander:
- http://localhost:8080 — Hero (Placeholder), Kacheln, Teaser
- http://localhost:8080/features.html — Werbetext, 5 Feature-Gruppen
- http://localhost:8080/aufbau.html — PCB-Bilder sichtbar, SVG-Diagramm, Lightbox funktioniert
- http://localhost:8080/geschichte.html — Timeline, Akkordeon klappt auf/zu
- http://localhost:8080/dokumentation.html — Download-Buttons (Dateien vorhanden)
- http://localhost:8080/galerie.html — Placeholder-Text

- [ ] **Schritt 5: Commit**

```bash
git add docs/website/img/pcb/ docs/website/downloads/ docs/website/img/gallery/README.md
git commit -m "feat: website Assets (PCB-Bilder, Downloads, Gallery-README)"
```

---

## Nutzer-Schritte nach Implementierung

1. **Hero-Foto:** `docs/website/img/hero.jpg` mit einem Foto der fertigen Uhr ersetzen
2. **Galerie-Fotos:** JPGs in `docs/website/img/gallery/` legen, dann in `galerie.html` im `gallery-grid` als `<a class="gallery-item">...<img data-lightbox="gallery">...</a>` eintragen
3. **Lokal testen:** `python3 -m http.server 8080 --directory docs/website` → http://localhost:8080
