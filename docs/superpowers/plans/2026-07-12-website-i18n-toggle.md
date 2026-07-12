# Website Sprachumschalter DE/EN Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Client-seitiger Sprachumschalter Deutsch/Englisch für alle 6 Seiten von `docs/website/`, ohne Reload, mit `localStorage`-Persistenz.

**Architecture:** Zwei statische JS-Wörterbücher (`I18N_DE`, `I18N_EN`) plus eine kleine Kernlogik (`i18n.js`), die `[data-i18n]`-Attribute im DOM durch den passenden Text ersetzt. `nav.js` rendert zusätzlich einen DE/EN-Umschalter in der Nav-Leiste und ruft nach der Header/Nav/Footer-Injektion die Sprachanwendung an. Kein Build-Schritt, reine `<script>`-Tags.

**Tech Stack:** Vanilla JS (ES6), kein Framework, kein Bundler, `localStorage` für Persistenz. Getestet manuell im Browser (kein Test-Framework im Projekt vorhanden, siehe Spec).

**Referenz-Spec:** `docs/superpowers/specs/2026-07-12-website-i18n-toggle-design.md`

## Global Constraints

- Kein Build-Schritt — der GitHub-Pages-Workflow lädt `docs/website` unverändert 1:1 hoch, keine neue Tooling-Abhängigkeit einführen.
- Default-Sprache ist immer Deutsch (`localStorage['lang']` fehlt → `'de'`), keine `navigator.language`-Erkennung.
- Sprachwahl über `localStorage` persistiert, gilt seitenübergreifend, kein Reload beim Umschalten.
- Umschalter sitzt rechts in der bestehenden sticky Nav-Leiste, auf allen Breakpoints sichtbar (Desktop und Mobile/Hamburger-Ansicht).
- `<title>`-Tags werden mit übersetzt (`meta.title`-Key pro Seite).
- Downloads (PDF/ODT) bleiben ausschließlich Deutsch; auf Englisch erscheint der Hinweis „(German only)“ statt einer Übersetzung der Dokumente.
- Deutscher Text bleibt als Fallback direkt im HTML stehen (progressive enhancement — ohne JS bleibt die Seite auf Deutsch lesbar).
- Bestehendes Nav/Mobile-Verhalten (`nav-toggle`, `.nav-inner.open`) darf nicht verändert werden — der Umschalter wird zusätzlich, nicht anstelle davon eingefügt.

---

## Datei-Übersicht

```
docs/website/
├── js/
│   ├── i18n-de.js   NEU — deutsches Wörterbuch (I18N_DE)
│   ├── i18n-en.js   NEU — englisches Wörterbuch (I18N_EN)
│   ├── i18n.js      NEU — Kernlogik (applyLanguage/setLanguage/getLanguage)
│   └── nav.js       GEÄNDERT — Umschalter-Buttons, data-i18n auf Nav-Links/Footer
├── css/
│   └── style.css    GEÄNDERT — .lang-switch Styling
├── index.html        GEÄNDERT — data-i18n-Attribute + Script-Tags
├── features.html     GEÄNDERT — data-i18n-Attribute + Script-Tags
├── aufbau.html        GEÄNDERT — data-i18n-Attribute + Script-Tags (inkl. SVG)
├── geschichte.html    GEÄNDERT — data-i18n-Attribute + Script-Tags
├── dokumentation.html GEÄNDERT — data-i18n-Attribute + Script-Tags + German-only-Hinweis
└── galerie.html       GEÄNDERT — data-i18n-Attribute + Script-Tags
```

Jede Seite bekommt vor dem schließenden `</body>` diese Script-Reihenfolge (Reihenfolge ist wichtig — siehe Task 1):

```html
<script src="js/nav.js"></script>
<script src="js/i18n-de.js"></script>
<script src="js/i18n-en.js"></script>
<script src="js/i18n.js"></script>
```

(Seiten mit `lightbox.js` behalten dieses Script zusätzlich danach.)

---

### Task 1: Core i18n-Infrastruktur + index.html (Referenzimplementierung)

**Files:**
- Create: `docs/website/js/i18n-de.js`
- Create: `docs/website/js/i18n-en.js`
- Create: `docs/website/js/i18n.js`
- Modify: `docs/website/js/nav.js`
- Modify: `docs/website/css/style.css`
- Modify: `docs/website/index.html`

**Interfaces:**
- Produces: `I18N_DE` (object, Key→String), `I18N_EN` (object, Key→String) — top-level `const`, als klassische (nicht-Modul) Scripts im globalen Scope aller nachfolgenden Script-Tags sichtbar
- Produces: `getLanguage(): string`, `setLanguage(lang: string): void`, `applyLanguage(lang: string): void` — globale Funktionen in `i18n.js`
- Consumes (in `nav.js`): ruft `applyLanguage(getLanguage())` nach der Injektion von Header/Nav/Footer auf; Umschalter-Buttons rufen `setLanguage('de')` / `setLanguage('en')` per `onclick` auf
- Alle folgenden Tasks (2–6) konsumieren `I18N_DE`/`I18N_EN` (fügen dort neue Keys ein) und verlassen sich auf `data-i18n`-Attribute + `applyLanguage()`, ohne deren Signatur zu ändern

- [ ] **Step 1: `js/i18n-de.js` anlegen mit globalen Keys (nav/footer) + allen index.html-Keys**

```js
const I18N_DE = {
  "nav.index": "Startseite",
  "nav.features": "Features",
  "nav.aufbau": "Aufbau",
  "nav.geschichte": "Geschichte",
  "nav.dokumentation": "Dokumentation",
  "nav.galerie": "Galerie",
  "footer.copyright": "&copy; 2026 broed digital media &middot; Nixie Clock Ultra",

  "index.meta.title": "Nixie Clock Ultra",
  "index.hero.title": "NIXIE CLOCK ULTRA",
  "index.hero.tagline": "wenn Zeit leuchtet",
  "index.hero.cta": "Mehr erfahren &rarr;",
  "index.highlight1.title": "Direct Drive",
  "index.highlight1.body": "Jede Kathode hat ihren eigenen Transistor &mdash; kein Multiplexing, kein Ghosting, kein Flimmern.",
  "index.highlight2.title": "WiFi &amp; Web-Interface",
  "index.highlight2.body": "Vollständige Steuerung über jeden Browser &mdash; kein App-Download, keine Einrichtung.",
  "index.highlight3.title": "Nacht-Modus &amp; LDR",
  "index.highlight3.body": "Zeitgesteuert oder per Lichtsensor &mdash; die Uhr dimmt sich automatisch zur Ruhe.",
  "index.teaser.p1": "Nixie-Röhren gehören zu den faszinierendsten Anzeigetechnologien, die je entwickelt wurden: warmes, leicht diffuses Licht, das Ziffern plastisch aus dem Glas herausleuchten lässt &mdash; unverwechselbar, zeitlos und heute begehrter denn je. Die Nixie Clock Ultra bringt dieses Feeling ins 21. Jahrhundert, ohne seinen Charakter zu verlieren.",
  "index.teaser.p2": "Sechs IN-12A-Nixieröhren, RGB-Hintergrundbeleuchtung mit drei Animationsmodi, Slot-Machine-Effekt, vollständiges Web-Interface, IR-Fernbedienung und automatische NTP-Zeitsynchronisation &mdash; in einem handgefertigten Gehäuse.",
  "index.teaser.link": "Alle Features entdecken &rarr;",
};
```

- [ ] **Step 2: `js/i18n-en.js` anlegen, gleiche Keys, englische Werte**

```js
const I18N_EN = {
  "nav.index": "Home",
  "nav.features": "Features",
  "nav.aufbau": "Hardware",
  "nav.geschichte": "History",
  "nav.dokumentation": "Documentation",
  "nav.galerie": "Gallery",
  "footer.copyright": "&copy; 2026 broed digital media &middot; Nixie Clock Ultra",

  "index.meta.title": "Nixie Clock Ultra",
  "index.hero.title": "NIXIE CLOCK ULTRA",
  "index.hero.tagline": "when time glows",
  "index.hero.cta": "Learn more &rarr;",
  "index.highlight1.title": "Direct Drive",
  "index.highlight1.body": "Every cathode has its own transistor &mdash; no multiplexing, no ghosting, no flicker.",
  "index.highlight2.title": "WiFi &amp; Web Interface",
  "index.highlight2.body": "Full control from any browser &mdash; no app download, no setup.",
  "index.highlight3.title": "Night Mode &amp; LDR",
  "index.highlight3.body": "Time-scheduled or via light sensor &mdash; the clock dims itself automatically for the night.",
  "index.teaser.p1": "Nixie tubes are among the most fascinating display technologies ever developed: warm, softly diffused light that makes digits glow with real depth out of the glass &mdash; unmistakable, timeless, and more sought-after today than ever. The Nixie Clock Ultra brings this feeling into the 21st century without losing its character.",
  "index.teaser.p2": "Six IN-12A Nixie tubes, RGB backlighting with three animation modes, a slot-machine effect, a full web interface, IR remote control, and automatic NTP time sync &mdash; all in a handcrafted case.",
  "index.teaser.link": "Discover all features &rarr;",
};
```

- [ ] **Step 3: `js/i18n.js` anlegen mit Kernlogik**

```js
function getLanguage() {
  return localStorage.getItem('lang') || 'de';
}

function applyLanguage(lang) {
  const dict = lang === 'en' ? I18N_EN : I18N_DE;
  document.documentElement.lang = lang;

  const page = document.body.dataset.page || '';
  const titleKey = page + '.meta.title';
  if (dict[titleKey]) {
    document.title = dict[titleKey];
  }

  document.querySelectorAll('[data-i18n]').forEach(function (el) {
    const key = el.dataset.i18n;
    if (dict[key] !== undefined) {
      el.innerHTML = dict[key];
    }
  });

  document.querySelectorAll('.lang-switch button').forEach(function (btn) {
    btn.classList.toggle('active', btn.dataset.lang === lang);
  });
}

function setLanguage(lang) {
  localStorage.setItem('lang', lang);
  applyLanguage(lang);
}

document.addEventListener('DOMContentLoaded', function () {
  applyLanguage(getLanguage());
});
```

- [ ] **Step 4: `js/nav.js` ändern — Umschalter rendern, `data-i18n` auf Nav-Links/Footer setzen**

Ersetze den kompletten Inhalt von `docs/website/js/nav.js` mit:

```js
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
      `<a href="${p.href}" data-i18n="nav.${p.key}"${p.key === page ? ' class="active"' : ''}>${p.label}</a>`
    ).join('');
    nav.innerHTML = `
      <div class="container">
        <button class="nav-toggle" aria-label="Navigation" onclick="this.nextElementSibling.classList.toggle('open')">&#9776;</button>
        <div class="nav-inner">${links}</div>
        <div class="lang-switch">
          <button type="button" data-lang="de" onclick="setLanguage('de')">DE</button>
          <button type="button" data-lang="en" onclick="setLanguage('en')">EN</button>
        </div>
      </div>`;
  }

  const footer = document.getElementById('site-footer');
  if (footer) {
    footer.innerHTML = `
      <div class="container">
        <p class="footer-text" data-i18n="footer.copyright">&copy; 2026 broed digital media &middot; Nixie Clock Ultra</p>
      </div>`;
  }
}

document.addEventListener('DOMContentLoaded', injectNav);
```

(Änderungen gegenüber dem Original: `data-i18n="nav.${p.key}"` auf den Nav-Links, neuer `.lang-switch`-Block mit zwei Buttons, `data-i18n="footer.copyright"` auf dem Footer-Text.)

- [ ] **Step 5: `css/style.css` — Styling für `.lang-switch` ergänzen**

Füge am Ende der Datei `docs/website/css/style.css` an:

```css
/* ===== SPRACHUMSCHALTER ===== */
#site-nav .container {
  position: relative;
}
.lang-switch {
  position: absolute;
  right: 2rem;
  top: 50%;
  transform: translateY(-50%);
  display: flex;
  gap: 0.3rem;
}
.lang-switch button {
  background: none;
  border: 1px solid var(--border-gold);
  color: var(--text-muted);
  font-family: 'Cinzel', serif;
  font-size: 0.75rem;
  letter-spacing: 0.05em;
  padding: 0.3rem 0.6rem;
  cursor: pointer;
  transition: background 0.2s, color 0.2s;
}
.lang-switch button:hover {
  background: var(--bg-panel-hover);
  color: var(--accent-copper);
}
.lang-switch button.active {
  background: var(--accent-gold);
  color: #0e0a05;
  font-weight: 700;
}
@media (max-width: 768px) {
  .lang-switch { right: 1rem; }
}
```

- [ ] **Step 6: `index.html` — Script-Tags und `data-i18n`-Attribute einfügen**

Ersetze den kompletten Inhalt von `docs/website/index.html` mit:

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
        <h1 class="hero-title" data-i18n="index.hero.title">NIXIE CLOCK ULTRA</h1>
        <p class="hero-tagline" data-i18n="index.hero.tagline">wenn Zeit leuchtet</p>
        <a href="features.html" class="btn btn-primary" data-i18n="index.hero.cta">Mehr erfahren &rarr;</a>
      </div>
    </section>

    <div class="container">
      <div class="highlight-grid" style="margin-top: 2.5rem;">
        <div class="panel highlight-card">
          <h3 data-i18n="index.highlight1.title">Direct Drive</h3>
          <p data-i18n="index.highlight1.body">Jede Kathode hat ihren eigenen Transistor &mdash; kein Multiplexing, kein Ghosting, kein Flimmern.</p>
        </div>
        <div class="panel highlight-card">
          <h3 data-i18n="index.highlight2.title">WiFi &amp; Web-Interface</h3>
          <p data-i18n="index.highlight2.body">Vollständige Steuerung über jeden Browser &mdash; kein App-Download, keine Einrichtung.</p>
        </div>
        <div class="panel highlight-card">
          <h3 data-i18n="index.highlight3.title">Nacht-Modus &amp; LDR</h3>
          <p data-i18n="index.highlight3.body">Zeitgesteuert oder per Lichtsensor &mdash; die Uhr dimmt sich automatisch zur Ruhe.</p>
        </div>
      </div>

      <div class="gear-divider">&#9881;</div>

      <div class="panel teaser-section">
        <p data-i18n="index.teaser.p1">Nixie-Röhren gehören zu den faszinierendsten Anzeigetechnologien, die je entwickelt wurden: warmes, leicht diffuses Licht, das Ziffern plastisch aus dem Glas herausleuchten lässt &mdash; unverwechselbar, zeitlos und heute begehrter denn je. Die Nixie Clock Ultra bringt dieses Feeling ins 21. Jahrhundert, ohne seinen Charakter zu verlieren.</p>
        <p data-i18n="index.teaser.p2">Sechs IN-12A-Nixieröhren, RGB-Hintergrundbeleuchtung mit drei Animationsmodi, Slot-Machine-Effekt, vollständiges Web-Interface, IR-Fernbedienung und automatische NTP-Zeitsynchronisation &mdash; in einem handgefertigten Gehäuse.</p>
        <p><a href="features.html" data-i18n="index.teaser.link">Alle Features entdecken &rarr;</a></p>
      </div>
    </div>

  </main>
  <footer id="site-footer"></footer>
  <script src="js/nav.js"></script>
  <script src="js/i18n-de.js"></script>
  <script src="js/i18n-en.js"></script>
  <script src="js/i18n.js"></script>
</body>
</html>
```

- [ ] **Step 7: Manuelle Verifikation im Browser**

Run: `cd docs/website && python3 -m http.server 8080` (im Hintergrund), dann `http://localhost:8080/index.html` öffnen.

Prüfen:
- Seite lädt auf Deutsch (Nav „Startseite/Features/…“, Hero „wenn Zeit leuchtet“, Tab-Titel „Nixie Clock Ultra“)
- Klick auf „EN“ im Umschalter: Nav wird zu „Home/Features/Hardware/…“, Hero zu „when time glows“, Tab-Titel bleibt „Nixie Clock Ultra“ (Meta-Title ist auf dieser Seite in beiden Sprachen identisch), kein Reload sichtbar
- Aktiver Button (`DE`/`EN`) ist optisch hervorgehoben (`.active`-Klasse)
- Seite neu laden (F5): Sprache bleibt Englisch (aus `localStorage` gelesen)
- Browser-Fenster auf < 768px verkleinern: Hamburger-Menü funktioniert weiterhin wie vorher, Umschalter bleibt rechts sichtbar
- `localStorage.clear()` in der Konsole, Seite neu laden: zurück auf Deutsch (Default)

Expected: Alle Punkte treffen zu.

- [ ] **Step 8: Commit**

```bash
git add docs/website/js/i18n-de.js docs/website/js/i18n-en.js docs/website/js/i18n.js docs/website/js/nav.js docs/website/css/style.css docs/website/index.html
git commit -m "feat: Sprachumschalter DE/EN – Core-Infrastruktur + Startseite"
```

---

### Task 2: features.html

**Files:**
- Modify: `docs/website/js/i18n-de.js`
- Modify: `docs/website/js/i18n-en.js`
- Modify: `docs/website/features.html`

**Interfaces:**
- Consumes: `I18N_DE`/`I18N_EN` (aus Task 1), `data-i18n`-Attribut-Konvention, Script-Tag-Reihenfolge aus Task 1
- Produces: `features.*`-Keys für spätere Referenz (keine anderen Tasks hängen davon ab)

- [ ] **Step 1: Keys zu `js/i18n-de.js` hinzufügen**

Füge vor der schließenden `};`-Zeile in `docs/website/js/i18n-de.js` ein:

```js

  "features.meta.title": "Features – Nixie Clock Ultra",
  "features.h1": "Features",
  "features.werbetext.p1": "Nixie-Röhren gehören zu den faszinierendsten Anzeigetechnologien, die je entwickelt wurden: warmes, leicht diffuses Licht, das Ziffern plastisch aus dem Glas herausleuchten lässt &mdash; unverwechselbar, zeitlos und heute begehrter denn je. Die Nixie Clock Ultra bringt dieses Feeling ins 21. Jahrhundert, ohne seinen Charakter zu verlieren.",
  "features.werbetext.p2": "Sechs IN-12A-Nixieröhren zeigen Stunden, Minuten und Sekunden &mdash; und auf Wunsch auch das Datum. Hinter jeder Röhre sitzt eine individuell ansteuerbare RGB-LED, die das Glas von innen in warmem Licht erstrahlen lässt: statisch, im sanften Atemrhythmus oder als fließendes Farbspektrum. Vier weitere LEDs beleuchten die Trennpunkte zwischen den Zifferngruppen.",
  "features.werbetext.p3": "Was diese Uhr von den meisten anderen Nixie-Projekten unterscheidet, ist die Art der Röhrenansteuerung: Kein Multiplexing, keine Kompromisse. Jede der 60 Kathoden besitzt ihren eigenen Hochvolt-Transistor &mdash; die Ziffern leuchten dauerhaft, ohne jegliches Ghosting oder Flimmern. Das Ergebnis ist ein sauberes, stabiles Bild, das man sofort sieht und lange nicht vergisst.",
  "features.werbetext.p4": "Die Uhr denkt mit: Ein ESP32-S3 verbindet sich mit dem Heimnetz, synchronisiert die Uhrzeit vollautomatisch per NTP aus dem Internet und ist über jeden Browser erreichbar &mdash; kein App-Download, kein Pairing. Wer lieber zur Fernbedienung greift: fast jede handelsübliche IR-Fernbedienung lässt sich anlernen.",
  "features.werbetext.p5": "Die Nixie Clock Ultra ist ein Unikat &mdash; handgefertigte Einzelplatinen, sorgfältig bestückt und getestet. Kein Bausatz, keine Kompromisse.",
  "features.h2": "Features auf einen Blick",
  "features.group.anzeige": "Anzeige",
  "features.anzeige.li1": "6 &times; IN-12A Nixie-Röhren &mdash; direkte Kathodensteuerung ohne Multiplexing, kein Ghosting, kein Flimmern",
  "features.anzeige.li2": "Stunden &middot; Minuten &middot; Sekunden sowie Datumsanzeige (TT MM JJ)",
  "features.anzeige.li3": "Sanfter Fade-In beim Einschalten",
  "features.anzeige.li4": "Optional weicher Ziffernwechsel (Crossfade) &mdash; einzeln zuschaltbar für Sekundentakt und für den Wechsel zwischen Uhrzeit und Datum",
  "features.group.beleuchtung": "Beleuchtung",
  "features.beleuchtung.li1": "6 &times; WS2812B RGB-LEDs als Röhrenhintergrundbeleuchtung (individuell pro Röhre)",
  "features.beleuchtung.li2": "4 &times; WS2812B RGB-LEDs für die Trennpunkt-Beleuchtung",
  "features.beleuchtung.li3": "Animationsmodi: Rainbow, Statisch (warmweiß), Puls",
  "features.beleuchtung.li4": "Slot-Machine-Effekt: konfigurierbar (aus / alle 10 s / 1 min / 15 min / 1 h), zeigt danach automatisch das Datum",
  "features.beleuchtung.li5": "Rollgeschwindigkeit der Slot-Machine-Animation stufenlos im Web-Interface einstellbar",
  "features.group.konnektivitaet": "Konnektivität &amp; Bedienung",
  "features.konnektivitaet.li1": "ESP32-S3 mit integriertem WLAN (2,4 GHz)",
  "features.konnektivitaet.li2": "Gleichzeitig eigener WLAN-Accesspoint (kein Heimnetz notwendig) und WLAN-Client",
  "features.konnektivitaet.li3": "Automatische Zeitsynchronisation per NTP",
  "features.konnektivitaet.li4": "Vollständiges Web-Interface im Browser &mdash; kein App-Download, funktioniert auf Smartphone, Tablet und PC",
  "features.konnektivitaet.li5": "Erreichbar per IP oder als <code>nixieclockcs.local</code> im Heimnetz (mDNS)",
  "features.konnektivitaet.li6": "IR-Fernbedienung: 8 Funktionen frei auf jede handelsübliche Fernbedienung anlernbar",
  "features.konnektivitaet.li7": "4 Bedientaster direkt an der Uhr",
  "features.group.nachtmodus": "Nacht-Modus",
  "features.nachtmodus.li1": "Zeitgesteuertes Dimmen oder vollständiges Ausschalten der Röhren",
  "features.nachtmodus.li2": "Automatische Helligkeitsanpassung über eingebauten Lichtsensor (LDR)",
  "features.nachtmodus.li3": "Röhren-Dimmhelligkeit stufenlos von 2&ndash;60&nbsp;% einstellbar &mdash; echte Hardware-Dimmung statt Blinken",
  "features.nachtmodus.li4": "NeoPixel-Helligkeit wird im Nacht-Modus proportional mitgedimmt",
  "features.group.technik": "Technik",
  "features.technik.li1": "Zweiplatinen-Design: Logic Board (Rev 2.1) + Nixie Display Board (Rev 2.01)",
  "features.technik.li2": "DS1302 Echtzeituhr mit CR2032-Batterie-Backup &mdash; hält Zeit auch ohne Strom",
  "features.technik.li3": "Hochvolt-Versorgung: 5 V USB &rarr; DC-DC-Konverter &rarr; 10 V &rarr; HV-Modul &rarr; ~170 V &rarr; TLP627-Hardware-Dimmer &rarr; Nixie-Anoden",
  "features.technik.li4": "60 diskrete SMBTA42-Hochvolt-Transistoren (je eine Kathode, je eine Röhrenziffer)",
  "features.technik.li5": "Alle Einstellungen bleiben nach Stromunterbrechung erhalten (NVS-Flash)",
  "features.technik.li6": "Stromversorgung: Micro-USB, 5 V",
```

- [ ] **Step 2: Keys zu `js/i18n-en.js` hinzufügen**

Füge vor der schließenden `};`-Zeile in `docs/website/js/i18n-en.js` ein:

```js

  "features.meta.title": "Features – Nixie Clock Ultra",
  "features.h1": "Features",
  "features.werbetext.p1": "Nixie tubes are among the most fascinating display technologies ever developed: warm, softly diffused light that makes digits glow with real depth out of the glass &mdash; unmistakable, timeless, and more sought-after today than ever. The Nixie Clock Ultra brings this feeling into the 21st century without losing its character.",
  "features.werbetext.p2": "Six IN-12A Nixie tubes display hours, minutes, and seconds &mdash; and the date on request. Behind each tube sits an individually addressable RGB LED that lights the glass from within with warm light: static, in a gentle breathing rhythm, or as a flowing color spectrum. Four more LEDs illuminate the separator dots between digit groups.",
  "features.werbetext.p3": "What sets this clock apart from most other Nixie projects is how the tubes are driven: no multiplexing, no compromises. Each of the 60 cathodes has its own high-voltage transistor &mdash; the digits glow continuously, with no ghosting or flicker whatsoever. The result is a clean, stable image that you notice immediately and won't forget for a long time.",
  "features.werbetext.p4": "The clock thinks along with you: an ESP32-S3 connects to your home network, synchronizes the time fully automatically via NTP from the internet, and is reachable from any browser &mdash; no app download, no pairing. If you'd rather use a remote: almost any off-the-shelf IR remote can be taught to it.",
  "features.werbetext.p5": "The Nixie Clock Ultra is a one-of-a-kind piece &mdash; hand-built individual boards, carefully assembled and tested. No kit, no compromises.",
  "features.h2": "Features at a Glance",
  "features.group.anzeige": "Display",
  "features.anzeige.li1": "6 &times; IN-12A Nixie tubes &mdash; direct cathode drive with no multiplexing, no ghosting, no flicker",
  "features.anzeige.li2": "Hours &middot; minutes &middot; seconds plus date display (DD MM YY)",
  "features.anzeige.li3": "Gentle fade-in on power-up",
  "features.anzeige.li4": "Optional soft digit transition (crossfade) &mdash; independently toggleable for the seconds tick and for switching between time and date",
  "features.group.beleuchtung": "Lighting",
  "features.beleuchtung.li1": "6 &times; WS2812B RGB LEDs as tube backlighting (individually addressable per tube)",
  "features.beleuchtung.li2": "4 &times; WS2812B RGB LEDs for separator-dot lighting",
  "features.beleuchtung.li3": "Animation modes: rainbow, static (warm white), pulse",
  "features.beleuchtung.li4": "Slot-machine effect: configurable (off / every 10 s / 1 min / 15 min / 1 h), automatically shows the date afterwards",
  "features.beleuchtung.li5": "Slot-machine animation roll speed continuously adjustable in the web interface",
  "features.group.konnektivitaet": "Connectivity &amp; Controls",
  "features.konnektivitaet.li1": "ESP32-S3 with built-in WiFi (2.4 GHz)",
  "features.konnektivitaet.li2": "Simultaneously its own WiFi access point (no home network required) and WiFi client",
  "features.konnektivitaet.li3": "Automatic time synchronization via NTP",
  "features.konnektivitaet.li4": "Full web interface in the browser &mdash; no app download, works on smartphone, tablet, and PC",
  "features.konnektivitaet.li5": "Reachable by IP or as <code>nixieclockcs.local</code> on the home network (mDNS)",
  "features.konnektivitaet.li6": "IR remote control: 8 functions freely trainable onto any off-the-shelf remote",
  "features.konnektivitaet.li7": "4 control buttons directly on the clock",
  "features.group.nachtmodus": "Night Mode",
  "features.nachtmodus.li1": "Time-scheduled dimming or complete shutdown of the tubes",
  "features.nachtmodus.li2": "Automatic brightness adjustment via built-in light sensor (LDR)",
  "features.nachtmodus.li3": "Tube dimming brightness continuously adjustable from 2&ndash;60&nbsp;% &mdash; real hardware dimming instead of flicker-blinking",
  "features.nachtmodus.li4": "NeoPixel brightness is dimmed proportionally along with night mode",
  "features.group.technik": "Technical",
  "features.technik.li1": "Two-board design: Logic Board (Rev 2.1) + Nixie Display Board (Rev 2.01)",
  "features.technik.li2": "DS1302 real-time clock with CR2032 battery backup &mdash; keeps time even without power",
  "features.technik.li3": "High-voltage supply: 5 V USB &rarr; DC-DC converter &rarr; 10 V &rarr; HV module &rarr; ~170 V &rarr; TLP627 hardware dimmer &rarr; Nixie anodes",
  "features.technik.li4": "60 discrete SMBTA42 high-voltage transistors (one per cathode, one per tube digit)",
  "features.technik.li5": "All settings are retained after a power interruption (NVS flash)",
  "features.technik.li6": "Power supply: Micro-USB, 5 V",
```

- [ ] **Step 3: `features.html` — Script-Tags und `data-i18n`-Attribute einfügen**

Ersetze den kompletten Inhalt von `docs/website/features.html` mit:

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
      <div class="page-title"><h1 data-i18n="features.h1">Features</h1></div>
      <div class="gear-divider">&#9881;</div>

      <div class="panel werbetext">
        <p data-i18n="features.werbetext.p1">Nixie-Röhren gehören zu den faszinierendsten Anzeigetechnologien, die je entwickelt wurden: warmes, leicht diffuses Licht, das Ziffern plastisch aus dem Glas herausleuchten lässt &mdash; unverwechselbar, zeitlos und heute begehrter denn je. Die Nixie Clock Ultra bringt dieses Feeling ins 21. Jahrhundert, ohne seinen Charakter zu verlieren.</p>
        <p data-i18n="features.werbetext.p2">Sechs IN-12A-Nixieröhren zeigen Stunden, Minuten und Sekunden &mdash; und auf Wunsch auch das Datum. Hinter jeder Röhre sitzt eine individuell ansteuerbare RGB-LED, die das Glas von innen in warmem Licht erstrahlen lässt: statisch, im sanften Atemrhythmus oder als fließendes Farbspektrum. Vier weitere LEDs beleuchten die Trennpunkte zwischen den Zifferngruppen.</p>
        <p data-i18n="features.werbetext.p3">Was diese Uhr von den meisten anderen Nixie-Projekten unterscheidet, ist die Art der Röhrenansteuerung: Kein Multiplexing, keine Kompromisse. Jede der 60 Kathoden besitzt ihren eigenen Hochvolt-Transistor &mdash; die Ziffern leuchten dauerhaft, ohne jegliches Ghosting oder Flimmern. Das Ergebnis ist ein sauberes, stabiles Bild, das man sofort sieht und lange nicht vergisst.</p>
        <p data-i18n="features.werbetext.p4">Die Uhr denkt mit: Ein ESP32-S3 verbindet sich mit dem Heimnetz, synchronisiert die Uhrzeit vollautomatisch per NTP aus dem Internet und ist über jeden Browser erreichbar &mdash; kein App-Download, kein Pairing. Wer lieber zur Fernbedienung greift: fast jede handelsübliche IR-Fernbedienung lässt sich anlernen.</p>
        <p data-i18n="features.werbetext.p5">Die Nixie Clock Ultra ist ein Unikat &mdash; handgefertigte Einzelplatinen, sorgfältig bestückt und getestet. Kein Bausatz, keine Kompromisse.</p>
      </div>

      <div class="gear-divider">&#9881;</div>
      <h2 data-i18n="features.h2">Features auf einen Blick</h2>

      <div class="feature-groups">

        <div class="panel">
          <div class="feature-group-label" data-i18n="features.group.anzeige">Anzeige</div>
          <ul class="feature-list">
            <li data-i18n="features.anzeige.li1">6 &times; IN-12A Nixie-Röhren &mdash; direkte Kathodensteuerung ohne Multiplexing, kein Ghosting, kein Flimmern</li>
            <li data-i18n="features.anzeige.li2">Stunden &middot; Minuten &middot; Sekunden sowie Datumsanzeige (TT MM JJ)</li>
            <li data-i18n="features.anzeige.li3">Sanfter Fade-In beim Einschalten</li>
            <li data-i18n="features.anzeige.li4">Optional weicher Ziffernwechsel (Crossfade) &mdash; einzeln zuschaltbar für Sekundentakt und für den Wechsel zwischen Uhrzeit und Datum</li>
          </ul>
        </div>

        <div class="panel">
          <div class="feature-group-label" data-i18n="features.group.beleuchtung">Beleuchtung</div>
          <ul class="feature-list">
            <li data-i18n="features.beleuchtung.li1">6 &times; WS2812B RGB-LEDs als Röhrenhintergrundbeleuchtung (individuell pro Röhre)</li>
            <li data-i18n="features.beleuchtung.li2">4 &times; WS2812B RGB-LEDs für die Trennpunkt-Beleuchtung</li>
            <li data-i18n="features.beleuchtung.li3">Animationsmodi: Rainbow, Statisch (warmweiß), Puls</li>
            <li data-i18n="features.beleuchtung.li4">Slot-Machine-Effekt: konfigurierbar (aus / alle 10 s / 1 min / 15 min / 1 h), zeigt danach automatisch das Datum</li>
            <li data-i18n="features.beleuchtung.li5">Rollgeschwindigkeit der Slot-Machine-Animation stufenlos im Web-Interface einstellbar</li>
          </ul>
        </div>

        <div class="panel">
          <div class="feature-group-label" data-i18n="features.group.konnektivitaet">Konnektivität &amp; Bedienung</div>
          <ul class="feature-list">
            <li data-i18n="features.konnektivitaet.li1">ESP32-S3 mit integriertem WLAN (2,4 GHz)</li>
            <li data-i18n="features.konnektivitaet.li2">Gleichzeitig eigener WLAN-Accesspoint (kein Heimnetz notwendig) und WLAN-Client</li>
            <li data-i18n="features.konnektivitaet.li3">Automatische Zeitsynchronisation per NTP</li>
            <li data-i18n="features.konnektivitaet.li4">Vollständiges Web-Interface im Browser &mdash; kein App-Download, funktioniert auf Smartphone, Tablet und PC</li>
            <li data-i18n="features.konnektivitaet.li5">Erreichbar per IP oder als <code>nixieclockcs.local</code> im Heimnetz (mDNS)</li>
            <li data-i18n="features.konnektivitaet.li6">IR-Fernbedienung: 8 Funktionen frei auf jede handelsübliche Fernbedienung anlernbar</li>
            <li data-i18n="features.konnektivitaet.li7">4 Bedientaster direkt an der Uhr</li>
          </ul>
        </div>

        <div class="panel">
          <div class="feature-group-label" data-i18n="features.group.nachtmodus">Nacht-Modus</div>
          <ul class="feature-list">
            <li data-i18n="features.nachtmodus.li1">Zeitgesteuertes Dimmen oder vollständiges Ausschalten der Röhren</li>
            <li data-i18n="features.nachtmodus.li2">Automatische Helligkeitsanpassung über eingebauten Lichtsensor (LDR)</li>
            <li data-i18n="features.nachtmodus.li3">Röhren-Dimmhelligkeit stufenlos von 2&ndash;60&nbsp;% einstellbar &mdash; echte Hardware-Dimmung statt Blinken</li>
            <li data-i18n="features.nachtmodus.li4">NeoPixel-Helligkeit wird im Nacht-Modus proportional mitgedimmt</li>
          </ul>
        </div>

        <div class="panel">
          <div class="feature-group-label" data-i18n="features.group.technik">Technik</div>
          <ul class="feature-list">
            <li data-i18n="features.technik.li1">Zweiplatinen-Design: Logic Board (Rev 2.1) + Nixie Display Board (Rev 2.01)</li>
            <li data-i18n="features.technik.li2">DS1302 Echtzeituhr mit CR2032-Batterie-Backup &mdash; hält Zeit auch ohne Strom</li>
            <li data-i18n="features.technik.li3">Hochvolt-Versorgung: 5 V USB &rarr; DC-DC-Konverter &rarr; 10 V &rarr; HV-Modul &rarr; ~170 V &rarr; TLP627-Hardware-Dimmer &rarr; Nixie-Anoden</li>
            <li data-i18n="features.technik.li4">60 diskrete SMBTA42-Hochvolt-Transistoren (je eine Kathode, je eine Röhrenziffer)</li>
            <li data-i18n="features.technik.li5">Alle Einstellungen bleiben nach Stromunterbrechung erhalten (NVS-Flash)</li>
            <li data-i18n="features.technik.li6">Stromversorgung: Micro-USB, 5 V</li>
          </ul>
        </div>

      </div>
    </div>
  </main>
  <footer id="site-footer"></footer>
  <script src="js/nav.js"></script>
  <script src="js/i18n-de.js"></script>
  <script src="js/i18n-en.js"></script>
  <script src="js/i18n.js"></script>
</body>
</html>
```

- [ ] **Step 4: Manuelle Verifikation im Browser**

Run (Server aus Task 1 läuft ggf. schon weiter, sonst neu starten): `http://localhost:8080/features.html`

Prüfen:
- DE: Alle Absätze, Feature-Listen und Gruppen-Labels wie im Original
- Umschalten auf EN: „Features at a Glance“, alle 5 Gruppen übersetzt (Display/Lighting/Connectivity & Controls/Night Mode/Technical), `<code>nixieclockcs.local</code>` bleibt als Code-Element erhalten (nicht als Text escaped)
- Navigation zu `index.html` und zurück: Sprache (EN) bleibt erhalten

Expected: Alle Punkte treffen zu.

- [ ] **Step 5: Commit**

```bash
git add docs/website/js/i18n-de.js docs/website/js/i18n-en.js docs/website/features.html
git commit -m "feat: Sprachumschalter DE/EN – Features-Seite übersetzt"
```

---

### Task 3: aufbau.html

**Files:**
- Modify: `docs/website/js/i18n-de.js`
- Modify: `docs/website/js/i18n-en.js`
- Modify: `docs/website/aufbau.html`

**Interfaces:**
- Consumes: `I18N_DE`/`I18N_EN`, `data-i18n`-Konvention aus Task 1
- Produces: `aufbau.*`-Keys (keine anderen Tasks hängen davon ab)

- [ ] **Step 1: Keys zu `js/i18n-de.js` hinzufügen**

Füge vor der schließenden `};`-Zeile in `docs/website/js/i18n-de.js` ein:

```js

  "aufbau.meta.title": "Aufbau – Nixie Clock Ultra",
  "aufbau.h1": "Aufbau &amp; Hardware",
  "aufbau.intro": "Die Nixie Clock Ultra besteht aus zwei Platinen, die über zwei Steckverbinder verbunden sind: Das <strong>Logic Board</strong> beherbergt den Mikrocontroller, die Echtzeituhr und die Spannungsversorgung. Das <strong>Nixie Display Board</strong> enthält die Anzeigeelektronik mit vier MCP23017 Port-Expandern, 60 Hochvolt-Transistoren und den WS2812B RGB-LEDs.",
  "aufbau.caption.schematic": "Schaltplan",
  "aufbau.caption.pcbtop": "PCB-Layout Oberseite",
  "aufbau.caption.pcbbottom": "PCB-Layout Unterseite",
  "aufbau.h3.komponenten": "Komponenten",
  "aufbau.logic.p1": "Herzstück ist der <strong>ESP32-S3-WROOM-1</strong> (U1) mit integriertem WLAN. Die batteriegepufferte <strong>DS1302 Echtzeituhr</strong> (U2) hält die Zeit auch bei Stromausfall. Der <strong>VS1838B</strong> (U3) empfängt und demoduliert IR-Signale.",
  "aufbau.logic.p2": "Die Spannungsversorgung erfolgt dreistufig: <strong>AMS1117-3.3</strong> (U5) liefert 3,3 V für die Logik; der <strong>DC-DC-Konverter</strong> (U6) hebt 5 V auf 10 V an; das <strong>HV-Modul</strong> (U4) erzeugt daraus ~170 V, die über einen <strong>TLP627</strong>-Optokoppler für den Nacht-Modus dimmbar an die Nixie-Anoden weitergegeben werden.",
  "aufbau.logic.p3": "Neu in Rev 2.1: Lichtsensor-Anschluss <strong>J5</strong> (LDR, GPIO6) für den automatischen Nacht-Modus. Neu ergänzt: <strong>TLP627</strong>-Optokoppler (GPIO7) dimmt die Anodenspannung per Hardware-PWM (2&ndash;60&nbsp;%) &mdash; aktuell als Erweiterung handverdrahtet, noch nicht mit eigener Referenz im Schaltplan.",
  "aufbau.warning.usbcdc": "<strong>USB-CDC-Hinweis:</strong> Der ESP32-S3 verwendet native USB (kein UART-Chip). In der Arduino IDE muss <strong>USB CDC On Boot: Enabled</strong> gesetzt sein &mdash; sonst ist nach dem ersten Flash keine erneute Programmierung über USB möglich (BOOT-Taster nicht bestückt).",
  "aufbau.h3.directdrive": "Direct Drive &mdash; kein Ghosting",
  "aufbau.nixieboard.p1": "Vier <strong>MCP23017</strong> I&sup2;C-Port-Expander (U1&ndash;U4, Adressen 0x20&ndash;0x23) stellen 64 digitale Ausgänge bereit. 60 davon steuern je einen <strong>SMBTA42</strong>-NPN-Transistor (300 V), der eine Nixie-Kathode direkt auf GND zieht.",
  "aufbau.nixieboard.p2": "Da jede Kathode ihren eigenen Transistor besitzt, können mehrere Röhren gleichzeitig leuchten &mdash; kein Multiplexing, keine gemeinsamen Pfade, kein Ghosting.",
  "aufbau.nixieboard.p3": "Sechs <strong>WS2812B-SMD-LEDs</strong> (Pixel 0&ndash;5, GRB) beleuchten die Röhren von hinten. Vier <strong>WS2812B-THT-LEDs YF923</strong> (Pixel 6&ndash;9, RGB) dienen als Trennpunkt-LEDs.",
  "aufbau.h2.spannung": "Spannungsversorgung",
  "aufbau.svg.dimmbar": "dimmbar",
  "aufbau.svg.nixie": "Nixie",
  "aufbau.svg.anoden": "Anoden",
  "aufbau.svg.logik": "Logik",
  "aufbau.svg.voltage33": "3,3 V",
```

- [ ] **Step 2: Keys zu `js/i18n-en.js` hinzufügen**

Füge vor der schließenden `};`-Zeile in `docs/website/js/i18n-en.js` ein:

```js

  "aufbau.meta.title": "Design – Nixie Clock Ultra",
  "aufbau.h1": "Design &amp; Hardware",
  "aufbau.intro": "The Nixie Clock Ultra consists of two boards connected via two board-to-board connectors: the <strong>Logic Board</strong> houses the microcontroller, the real-time clock, and the power supply. The <strong>Nixie Display Board</strong> contains the display electronics with four MCP23017 port expanders, 60 high-voltage transistors, and the WS2812B RGB LEDs.",
  "aufbau.caption.schematic": "Schematic",
  "aufbau.caption.pcbtop": "PCB Layout Top",
  "aufbau.caption.pcbbottom": "PCB Layout Bottom",
  "aufbau.h3.komponenten": "Components",
  "aufbau.logic.p1": "At the heart is the <strong>ESP32-S3-WROOM-1</strong> (U1) with built-in WiFi. The battery-backed <strong>DS1302 real-time clock</strong> (U2) keeps time even during a power outage. The <strong>VS1838B</strong> (U3) receives and demodulates IR signals.",
  "aufbau.logic.p2": "Power supply happens in three stages: the <strong>AMS1117-3.3</strong> (U5) supplies 3.3 V for the logic; the <strong>DC-DC converter</strong> (U6) steps 5 V up to 10 V; the <strong>HV module</strong> (U4) generates ~170 V from that, which is passed to the Nixie anodes via a <strong>TLP627</strong> optocoupler, dimmable for night mode.",
  "aufbau.logic.p3": "New in Rev 2.1: light-sensor connector <strong>J5</strong> (LDR, GPIO6) for automatic night mode. Newly added: <strong>TLP627</strong> optocoupler (GPIO7) dims the anode voltage via hardware PWM (2&ndash;60&nbsp;%) &mdash; currently hand-wired as an add-on, not yet given its own reference designator in the schematic.",
  "aufbau.warning.usbcdc": "<strong>USB CDC note:</strong> The ESP32-S3 uses native USB (no UART chip). In the Arduino IDE, <strong>USB CDC On Boot: Enabled</strong> must be set &mdash; otherwise, after the first flash, no further programming over USB is possible (the BOOT button is not populated).",
  "aufbau.h3.directdrive": "Direct Drive &mdash; No Ghosting",
  "aufbau.nixieboard.p1": "Four <strong>MCP23017</strong> I&sup2;C port expanders (U1&ndash;U4, addresses 0x20&ndash;0x23) provide 64 digital outputs. 60 of them each drive an <strong>SMBTA42</strong> NPN transistor (300 V) that pulls one Nixie cathode directly to GND.",
  "aufbau.nixieboard.p2": "Since every cathode has its own transistor, multiple tubes can glow at the same time &mdash; no multiplexing, no shared paths, no ghosting.",
  "aufbau.nixieboard.p3": "Six <strong>WS2812B SMD LEDs</strong> (pixels 0&ndash;5, GRB) light the tubes from behind. Four <strong>WS2812B THT LEDs YF923</strong> (pixels 6&ndash;9, RGB) serve as separator-dot LEDs.",
  "aufbau.h2.spannung": "Power Supply",
  "aufbau.svg.dimmbar": "dimmable",
  "aufbau.svg.nixie": "Nixie",
  "aufbau.svg.anoden": "Anodes",
  "aufbau.svg.logik": "Logic",
  "aufbau.svg.voltage33": "3.3 V",
```

- [ ] **Step 3: `aufbau.html` — Script-Tags und `data-i18n`-Attribute einfügen**

Ersetze den kompletten Inhalt von `docs/website/aufbau.html` mit:

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
      <div class="page-title"><h1 data-i18n="aufbau.h1">Aufbau &amp; Hardware</h1></div>
      <div class="gear-divider">&#9881;</div>

      <div class="panel">
        <p data-i18n="aufbau.intro">Die Nixie Clock Ultra besteht aus zwei Platinen, die über zwei Steckverbinder verbunden sind: Das <strong>Logic Board</strong> beherbergt den Mikrocontroller, die Echtzeituhr und die Spannungsversorgung. Das <strong>Nixie Display Board</strong> enthält die Anzeigeelektronik mit vier MCP23017 Port-Expandern, 60 Hochvolt-Transistoren und den WS2812B RGB-LEDs.</p>
      </div>

      <!-- Logic Board -->
      <h2>Logic Board (Rev 2.1)</h2>
      <div class="two-col">
        <div class="pcb-thumbs">
          <div>
            <img class="pcb-thumb" src="img/pcb/logic_sch-1.png" alt="Logic Board Schaltplan" data-lightbox="pcb">
            <p class="pcb-caption" data-i18n="aufbau.caption.schematic">Schaltplan</p>
          </div>
          <div>
            <img class="pcb-thumb" src="img/pcb/logic_pcb-1.png" alt="Logic Board PCB Oberseite" data-lightbox="pcb">
            <p class="pcb-caption" data-i18n="aufbau.caption.pcbtop">PCB-Layout Oberseite</p>
          </div>
          <div>
            <img class="pcb-thumb" src="img/pcb/logic_pcb-2.png" alt="Logic Board PCB Unterseite" data-lightbox="pcb">
            <p class="pcb-caption" data-i18n="aufbau.caption.pcbbottom">PCB-Layout Unterseite</p>
          </div>
        </div>
        <div class="two-col-text">
          <h3 data-i18n="aufbau.h3.komponenten">Komponenten</h3>
          <p data-i18n="aufbau.logic.p1">Herzstück ist der <strong>ESP32-S3-WROOM-1</strong> (U1) mit integriertem WLAN. Die batteriegepufferte <strong>DS1302 Echtzeituhr</strong> (U2) hält die Zeit auch bei Stromausfall. Der <strong>VS1838B</strong> (U3) empfängt und demoduliert IR-Signale.</p>
          <p data-i18n="aufbau.logic.p2">Die Spannungsversorgung erfolgt dreistufig: <strong>AMS1117-3.3</strong> (U5) liefert 3,3 V für die Logik; der <strong>DC-DC-Konverter</strong> (U6) hebt 5 V auf 10 V an; das <strong>HV-Modul</strong> (U4) erzeugt daraus ~170 V, die über einen <strong>TLP627</strong>-Optokoppler für den Nacht-Modus dimmbar an die Nixie-Anoden weitergegeben werden.</p>
          <p data-i18n="aufbau.logic.p3">Neu in Rev 2.1: Lichtsensor-Anschluss <strong>J5</strong> (LDR, GPIO6) für den automatischen Nacht-Modus. Neu ergänzt: <strong>TLP627</strong>-Optokoppler (GPIO7) dimmt die Anodenspannung per Hardware-PWM (2&ndash;60&nbsp;%) &mdash; aktuell als Erweiterung handverdrahtet, noch nicht mit eigener Referenz im Schaltplan.</p>
          <div class="warning-box" data-i18n="aufbau.warning.usbcdc">
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
            <p class="pcb-caption" data-i18n="aufbau.caption.schematic">Schaltplan</p>
          </div>
          <div>
            <img class="pcb-thumb" src="img/pcb/nixie_pcb-1.png" alt="Nixie Display Board PCB Oberseite" data-lightbox="pcb">
            <p class="pcb-caption" data-i18n="aufbau.caption.pcbtop">PCB-Layout Oberseite</p>
          </div>
          <div>
            <img class="pcb-thumb" src="img/pcb/nixie_pcb-2.png" alt="Nixie Display Board PCB Unterseite" data-lightbox="pcb">
            <p class="pcb-caption" data-i18n="aufbau.caption.pcbbottom">PCB-Layout Unterseite</p>
          </div>
        </div>
        <div class="two-col-text">
          <h3 data-i18n="aufbau.h3.directdrive">Direct Drive &mdash; kein Ghosting</h3>
          <p data-i18n="aufbau.nixieboard.p1">Vier <strong>MCP23017</strong> I²C-Port-Expander (U1&ndash;U4, Adressen 0x20&ndash;0x23) stellen 64 digitale Ausgänge bereit. 60 davon steuern je einen <strong>SMBTA42</strong>-NPN-Transistor (300 V), der eine Nixie-Kathode direkt auf GND zieht.</p>
          <p data-i18n="aufbau.nixieboard.p2">Da jede Kathode ihren eigenen Transistor besitzt, können mehrere Röhren gleichzeitig leuchten &mdash; kein Multiplexing, keine gemeinsamen Pfade, kein Ghosting.</p>
          <p data-i18n="aufbau.nixieboard.p3">Sechs <strong>WS2812B-SMD-LEDs</strong> (Pixel 0&ndash;5, GRB) beleuchten die Röhren von hinten. Vier <strong>WS2812B-THT-LEDs YF923</strong> (Pixel 6&ndash;9, RGB) dienen als Trennpunkt-LEDs.</p>
        </div>
      </div>

      <div class="gear-divider">&#9881;</div>

      <!-- Spannungsversorgung -->
      <h2 data-i18n="aufbau.h2.spannung">Spannungsversorgung</h2>
      <div class="voltage-diagram">
        <svg width="760" height="130" viewBox="0 0 760 130" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="Spannungsfluss-Diagramm">
          <rect x="10" y="10" width="80" height="44" rx="2" fill="#1c1408" stroke="#6b4f12" stroke-width="1.5"/>
          <text x="50" y="28" font-family="Cinzel,serif" font-size="11" fill="#c8961a" text-anchor="middle">USB</text>
          <text x="50" y="46" font-family="Cinzel,serif" font-size="11" fill="#e8d5a3" text-anchor="middle">5 V</text>

          <line x1="90" y1="32" x2="148" y2="32" stroke="#c8961a" stroke-width="1.5" marker-end="url(#arr)"/>

          <rect x="150" y="10" width="90" height="44" rx="2" fill="#1c1408" stroke="#6b4f12" stroke-width="1.5"/>
          <text x="195" y="28" font-family="Cinzel,serif" font-size="10" fill="#c8961a" text-anchor="middle">DC-DC</text>
          <text x="195" y="46" font-family="Cinzel,serif" font-size="10" fill="#a08c6a" text-anchor="middle">U6</text>

          <text x="270" y="26" font-family="serif" font-size="10" fill="#ff8c00" text-anchor="middle">10 V</text>
          <line x1="240" y1="32" x2="298" y2="32" stroke="#c8961a" stroke-width="1.5" marker-end="url(#arr)"/>

          <rect x="300" y="10" width="90" height="44" rx="2" fill="#1c1408" stroke="#6b4f12" stroke-width="1.5"/>
          <text x="345" y="28" font-family="Cinzel,serif" font-size="10" fill="#c8961a" text-anchor="middle">HV-MOD</text>
          <text x="345" y="46" font-family="Cinzel,serif" font-size="10" fill="#a08c6a" text-anchor="middle">U4</text>

          <text x="420" y="26" font-family="serif" font-size="10" fill="#ff8c00" text-anchor="middle">~170 V</text>
          <line x1="390" y1="32" x2="448" y2="32" stroke="#c8961a" stroke-width="1.5" marker-end="url(#arr)"/>

          <rect x="450" y="10" width="90" height="44" rx="2" fill="#1c1408" stroke="#6b4f12" stroke-width="1.5"/>
          <text x="495" y="28" font-family="Cinzel,serif" font-size="10" fill="#c8961a" text-anchor="middle">TLP627</text>
          <text x="495" y="46" font-family="Cinzel,serif" font-size="10" fill="#a08c6a" text-anchor="middle">Dimmer</text>

          <text x="570" y="26" font-family="serif" font-size="9" fill="#ff8c00" text-anchor="middle" data-i18n="aufbau.svg.dimmbar">dimmbar</text>
          <line x1="540" y1="32" x2="598" y2="32" stroke="#c8961a" stroke-width="1.5" marker-end="url(#arr)"/>

          <rect x="600" y="10" width="90" height="44" rx="2" fill="#1c1408" stroke="#6b4f12" stroke-width="1.5"/>
          <text x="645" y="28" font-family="Cinzel,serif" font-size="10" fill="#c8961a" text-anchor="middle" data-i18n="aufbau.svg.nixie">Nixie</text>
          <text x="645" y="46" font-family="Cinzel,serif" font-size="10" fill="#e8d5a3" text-anchor="middle" data-i18n="aufbau.svg.anoden">Anoden</text>

          <line x1="50" y1="54" x2="50" y2="86" stroke="#c8961a" stroke-width="1.5"/>
          <line x1="50" y1="86" x2="148" y2="86" stroke="#c8961a" stroke-width="1.5" marker-end="url(#arr)"/>

          <rect x="150" y="64" width="90" height="44" rx="2" fill="#1c1408" stroke="#6b4f12" stroke-width="1.5"/>
          <text x="195" y="82" font-family="Cinzel,serif" font-size="10" fill="#c8961a" text-anchor="middle">AMS1117</text>
          <text x="195" y="100" font-family="Cinzel,serif" font-size="10" fill="#a08c6a" text-anchor="middle">U5</text>

          <text x="270" y="80" font-family="serif" font-size="10" fill="#ff8c00" text-anchor="middle" data-i18n="aufbau.svg.voltage33">3,3 V</text>
          <line x1="240" y1="86" x2="298" y2="86" stroke="#c8961a" stroke-width="1.5" marker-end="url(#arr)"/>

          <rect x="300" y="64" width="90" height="44" rx="2" fill="#1c1408" stroke="#6b4f12" stroke-width="1.5"/>
          <text x="345" y="82" font-family="Cinzel,serif" font-size="10" fill="#c8961a" text-anchor="middle" data-i18n="aufbau.svg.logik">Logik</text>
          <text x="345" y="100" font-family="Cinzel,serif" font-size="10" fill="#a08c6a" text-anchor="middle">ESP32-S3</text>

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
  <script src="js/i18n-de.js"></script>
  <script src="js/i18n-en.js"></script>
  <script src="js/i18n.js"></script>
  <script src="js/lightbox.js"></script>
</body>
</html>
```

**Hinweis:** `aufbau.warning.usbcdc` wird per `innerHTML` auf das `<div class="warning-box">` gesetzt — dabei geht kein Markup verloren, da der Key selbst das `<strong>`-Tag enthält.

- [ ] **Step 4: Manuelle Verifikation im Browser**

`http://localhost:8080/aufbau.html` öffnen.

Prüfen:
- DE: Intro, Komponenten-Texte, Warnbox, SVG-Diagramm-Labels wie im Original
- EN: Alle Fließtexte übersetzt, SVG-Labels „dimmable“/„Nixie“/„Anodes“/„Logic“/„3.3 V“ korrekt, restliche SVG-Boxen (USB, DC-DC, HV-MOD, TLP627, AMS1117, ESP32-S3, U-Nummern) bleiben unverändert (keine Übersetzung nötig)
- `<strong>`-Hervorhebungen in Fließtext und Warnbox bleiben in beiden Sprachen erhalten (kein escaped HTML sichtbar)
- Lightbox-Klick auf PCB-Bilder funktioniert weiterhin unverändert (von i18n unberührt)

Expected: Alle Punkte treffen zu.

- [ ] **Step 5: Commit**

```bash
git add docs/website/js/i18n-de.js docs/website/js/i18n-en.js docs/website/aufbau.html
git commit -m "feat: Sprachumschalter DE/EN – Aufbau-Seite übersetzt"
```

---

### Task 4: geschichte.html

**Files:**
- Modify: `docs/website/js/i18n-de.js`
- Modify: `docs/website/js/i18n-en.js`
- Modify: `docs/website/geschichte.html`

**Interfaces:**
- Consumes: `I18N_DE`/`I18N_EN`, `data-i18n`-Konvention aus Task 1
- Produces: `geschichte.*`-Keys (keine anderen Tasks hängen davon ab)

- [ ] **Step 1: Keys zu `js/i18n-de.js` hinzufügen**

Füge vor der schließenden `};`-Zeile in `docs/website/js/i18n-de.js` ein:

```js

  "geschichte.meta.title": "Geschichte – Nixie Clock Ultra",
  "geschichte.h1": "Entwicklungsgeschichte",
  "geschichte.intro": "Die Nixie Clock Ultra entstand in acht Entwicklungsphasen zwischen April und Juli 2026. Jede Phase brachte neue Erkenntnisse &mdash; und neue Probleme, die es zu lösen galt.",

  "geschichte.phase1.title": "Phase 1 &mdash; Firmware-Grundgerüst",
  "geschichte.phase1.body": "Vollständige Basis-Firmware in einer einzigen <code>.ino</code>-Datei: Nixie-Ansteuerung via Multiplex-ISR, NeoPixel-Animationen, Web-Interface (WiFi AP, AsyncWebServer), IR-Fernbedienung mit 7 Funktionen, DS1302 RTC-Integration.",

  "geschichte.phase2.title": "Phase 2 &mdash; Modularisierung",
  "geschichte.phase2.body": "Die wachsende Datei wurde in sieben Module aufgeteilt: <code>rtc.ino</code>, <code>display.ino</code>, <code>neo_animation.ino</code>, <code>buttons.ino</code>, <code>ir_remote.ino</code>, <code>web_server.ino</code> und den Multiplex-Timer.",
  "geschichte.acc1.label": "Problem: Forward Declarations",
  "geschichte.acc1.content": "Der Arduino-Präprozessor erkennt Forward Declarations für <code>IRAM_ATTR</code>-Funktionen und Web-Server-Callbacks nicht automatisch. Lösung: Forward Declarations manuell in <code>NixieClockUltra.ino</code> ergänzt.",

  "geschichte.phase3.title": "Phase 3 &mdash; Hardware-Inbetriebnahme",
  "geschichte.phase3.body": "Erste Tests an der echten Hardware brachten mehrere Korrekturen: RGB-Bytereihenfolge der THT-WS2812B (RGB statt GRB), Kathoden-Mapping-Korrektur, separate Helligkeitsregelung für Hintergrund und Trennpunkte.",
  "geschichte.acc2.label": "Problem: NeoPixel RGB-Swap",
  "geschichte.acc2.content": "Die 4 Trennpunkt-LEDs (THT WS2812B YF923) zeigten falsche Farben. Ursache: THT-Variante verwendet RGB-Bytereihenfolge statt GRB wie die SMD-Version. Lösung: <code>rgbSwap</code> für Pixel 6&ndash;9 in <code>neo_animation.ino</code>.",
  "geschichte.acc3.label": "Problem: Ghosting (erste Versuche)",
  "geschichte.acc3.content": "Nixie-Röhren zeigten Geisterziffern durch den Multiplex-Betrieb. Zwei Versuche: (1) Anode vor Kathode schalten &mdash; kein Erfolg. (2) Blank-Phase verlängern &mdash; reduziert, aber nicht beseitigt. Strukturelle Lösung erst in Phase 4.",

  "geschichte.phase4.title": "Phase 4 &mdash; Anti-Ghosting: MCP23017 Direct Drive",
  "geschichte.phase4.body": "Kompletter Umbau der Nixie-Ansteuerung: statt eines 74HC595-Schieberegisters mit Multiplex-ISR nun vier MCP23017 I²C-Port-Expander mit je eigenem SMBTA42-Transistor pro Kathode. Kein Multiplexing mehr &mdash; alle Röhren leuchten dauerhaft.",
  "geschichte.acc4.label": "Ergebnis (getestet 2026-06-15)",
  "geschichte.acc4.content": "Ghosting vollständig beseitigt. Die direkte Ansteuerung via MCP23017 löst das Problem strukturell: Da keine Kathode mehr gemeinsam genutzt wird, gibt es keine Übersprecher zwischen den Röhren.",

  "geschichte.phase5.title": "Phase 5 &mdash; Dokumentation",
  "geschichte.phase5.body": "Vollständige Bedienungsanleitung im Steampunk-Design (HTML &rarr; PDF) sowie technische Systemdokumentation (ODT) mit eingebetteten Schaltplänen, API-Dokumentation und Bibliotheksübersicht.",

  "geschichte.phase6.title": "Phase 6 &mdash; Slot-Intervall &amp; Feinschliff",
  "geschichte.phase6.body": "Slot-Machine-Animation als eigenständige Einstellung (<code>SlotInterval</code>-Enum) ausgelagert. Mehrere Hardware-Bugs behoben: IR-Empfang nach <code>strip.show()</code>, Taster-Entprellung, Pin-Korrekturen, Warmweiß-Farbabgleich.",
  "geschichte.acc5.label": "Problem: IR-Empfang nach NeoPixel-Update",
  "geschichte.acc5.content": "IR-Codes wurden nach <code>strip.show()</code> nicht mehr empfangen. Ursache: <code>irrecv.pause()/resume()</code> um <code>strip.show()</code> resettete den RMT-Empfänger alle 20 ms. Lösung: pause/resume vollständig entfernt. Auf dem ESP32-S3 sind RMT-TX (NeoPixel) und RMT-RX (IR) unabhängige Kanäle &mdash; kein Konflikt.",
  "geschichte.acc6.label": "Problem: Taster-Entprellung",
  "geschichte.acc6.content": "<code>pressed</code> wurde nie <code>true</code>. Ursache: Logikfehler in der Button-FSM &mdash; das <code>debounced</code>-Flag wurde nie gesetzt, weil die Bedingung <code>lastState == HIGH</code> innerhalb des Debounce-Fensters niemals zutraf. Lösung: <code>Button</code>-Struct um <code>debounced</code>-Flag erweitert.",

  "geschichte.phase7.title": "Phase 7 &mdash; WiFi-Fixes &amp; mDNS",
  "geschichte.phase7.body": "WiFi-Scan entfernt (Blocking-Probleme mit ESPAsyncWebServer), STA-Timeout auf 20 s erhöht, DHCP-Hostname und mDNS implementiert. Die Uhr ist jetzt als <code>nixieclockcs.local</code> im Heimnetz erreichbar.",
  "geschichte.acc7.label": "Problem: WiFi-Scan lieferte keine Ergebnisse",
  "geschichte.acc7.content": "Zwei Ursachen: (1) <code>WiFi.mode(WIFI_AP)</code> deaktiviert das STA-Radio &mdash; kein Scan möglich. (2) Synchroner <code>scanNetworks()</code>-Aufruf im ESPAsyncWebServer-Handler blockiert den LWIP/WiFi-Task. Lösung: Scan-Funktion vollständig entfernt, SSID wird manuell eingegeben.",

  "geschichte.phase8.title": "Phase 8 &mdash; HV-Anodendimmung",
  "geschichte.phase8.body": "Der Nacht-Modus dimmt die Röhren jetzt nicht mehr per Software-PWM auf den Kathoden, sondern über einen TLP627-Optokoppler direkt auf der ~170-V-Anodenspannung (<code>hv_dimmer.ino</code>, LEDC-Hardware-PWM ~200 Hz). Dazu: Web-UI-Regler für die Dimm-Helligkeit (stufenlos 2&ndash;60&nbsp;%) mit sofortiger AJAX-Übernahme, veraltetes Nixie-Helligkeit-Dropdown entfernt, NeoPixel-Helligkeitsstufen für Taster/IR auf 10/40/80/200 nachjustiert.",
  "geschichte.acc8.label": "Beobachtung: Web-UI-Verzögerung nach dem Umbau",
  "geschichte.acc8.content": "Nach dem Umbau wirkte die Web-UI sowohl im AP- als auch im WLAN-Modus spürbar verzögert. Erste Hypothese: Das Schalten der vollen Anodenlast mit 200 Hz koppelt Störungen in den WLAN-Funkteil ein. Berechtigter Einwand: Sowohl beim alten Kathoden-Software-PWM als auch beim neuen Anoden-Hardware-PWM wird pro leuchtender Ziffer dieselbe Strommenge geschaltet &mdash; die Störquelle wäre also nicht grundsätzlich neu. Bei einem erneuten Test trat die Verzögerung nicht mehr auf; mangels Reproduzierbarkeit bleibt die genaue Ursache ungeklärt.",

  "geschichte.phase9.title": "Phase 9 &mdash; Weicher Ziffernwechsel",
  "geschichte.phase9.body": "Der Wechsel zwischen zwei Ziffern erfolgt jetzt optional nicht mehr schlagartig, sondern als sanfter Crossfade &mdash; getrennt zuschaltbar für den Sekundentakt und für den Wechsel zwischen Uhrzeit- und Datumsanzeige. Technisch über den vorhandenen HV-Dimmer realisiert (<code>digit_fade.ino</code>): kurz abblenden, Ziffern bei Minimalhelligkeit umschalten, wieder aufblenden &mdash; als non-blocking State-Machine, angetrieben aus <code>loop()</code>. Dauer nach einem Hardware-Test auf 400&nbsp;ms festgelegt. Zusätzlich wurde die Rollgeschwindigkeit der Slot-Machine-Animation (<code>slotSpeedPct</code>) über einen eigenen Web-UI-Regler einstellbar gemacht.",
```

- [ ] **Step 2: Keys zu `js/i18n-en.js` hinzufügen**

Füge vor der schließenden `};`-Zeile in `docs/website/js/i18n-en.js` ein:

```js

  "geschichte.meta.title": "History – Nixie Clock Ultra",
  "geschichte.h1": "Development History",
  "geschichte.intro": "The Nixie Clock Ultra was developed in eight phases between April and July 2026. Each phase brought new insights &mdash; and new problems that needed solving.",

  "geschichte.phase1.title": "Phase 1 &mdash; Firmware Skeleton",
  "geschichte.phase1.body": "Complete baseline firmware in a single <code>.ino</code> file: Nixie driving via multiplex ISR, NeoPixel animations, web interface (WiFi AP, AsyncWebServer), IR remote control with 7 functions, DS1302 RTC integration.",

  "geschichte.phase2.title": "Phase 2 &mdash; Modularization",
  "geschichte.phase2.body": "The growing file was split into seven modules: <code>rtc.ino</code>, <code>display.ino</code>, <code>neo_animation.ino</code>, <code>buttons.ino</code>, <code>ir_remote.ino</code>, <code>web_server.ino</code>, and the multiplex timer.",
  "geschichte.acc1.label": "Problem: Forward Declarations",
  "geschichte.acc1.content": "The Arduino preprocessor doesn't automatically detect forward declarations for <code>IRAM_ATTR</code> functions and web-server callbacks. Fix: forward declarations added manually in <code>NixieClockUltra.ino</code>.",

  "geschichte.phase3.title": "Phase 3 &mdash; Hardware Bring-Up",
  "geschichte.phase3.body": "First tests on real hardware led to several corrections: RGB byte order of the THT WS2812B (RGB instead of GRB), a cathode-mapping fix, and separate brightness control for background and separator LEDs.",
  "geschichte.acc2.label": "Problem: NeoPixel RGB Swap",
  "geschichte.acc2.content": "The 4 separator-dot LEDs (THT WS2812B YF923) showed the wrong colors. Cause: the THT variant uses RGB byte order instead of GRB like the SMD version. Fix: <code>rgbSwap</code> for pixels 6&ndash;9 in <code>neo_animation.ino</code>.",
  "geschichte.acc3.label": "Problem: Ghosting (First Attempts)",
  "geschichte.acc3.content": "Nixie tubes showed ghost digits due to multiplexed operation. Two attempts: (1) switching anode before cathode &mdash; no success. (2) lengthening the blanking phase &mdash; reduced but not eliminated. A structural fix only came in Phase 4.",

  "geschichte.phase4.title": "Phase 4 &mdash; Anti-Ghosting: MCP23017 Direct Drive",
  "geschichte.phase4.body": "Complete rework of the Nixie drive: instead of a 74HC595 shift register with a multiplex ISR, now four MCP23017 I²C port expanders with their own SMBTA42 transistor per cathode. No more multiplexing &mdash; all tubes glow continuously.",
  "geschichte.acc4.label": "Result (tested 2026-06-15)",
  "geschichte.acc4.content": "Ghosting completely eliminated. Direct drive via MCP23017 solves the problem structurally: since no cathode is shared anymore, there's no crosstalk between tubes.",

  "geschichte.phase5.title": "Phase 5 &mdash; Documentation",
  "geschichte.phase5.body": "Complete user manual in steampunk design (HTML &rarr; PDF) plus technical system documentation (ODT) with embedded schematics, API documentation, and a library overview.",

  "geschichte.phase6.title": "Phase 6 &mdash; Slot Interval &amp; Polish",
  "geschichte.phase6.body": "Slot-machine animation extracted as an independent setting (<code>SlotInterval</code> enum). Several hardware bugs fixed: IR reception after <code>strip.show()</code>, button debouncing, pin corrections, warm-white color calibration.",
  "geschichte.acc5.label": "Problem: IR Reception After NeoPixel Update",
  "geschichte.acc5.content": "IR codes were no longer received after <code>strip.show()</code>. Cause: <code>irrecv.pause()/resume()</code> around <code>strip.show()</code> reset the RMT receiver every 20 ms. Fix: pause/resume removed entirely. On the ESP32-S3, RMT-TX (NeoPixel) and RMT-RX (IR) are independent channels &mdash; no conflict.",
  "geschichte.acc6.label": "Problem: Button Debouncing",
  "geschichte.acc6.content": "<code>pressed</code> was never <code>true</code>. Cause: a logic error in the button FSM &mdash; the <code>debounced</code> flag was never set because the condition <code>lastState == HIGH</code> within the debounce window was never true. Fix: the <code>Button</code> struct was extended with a <code>debounced</code> flag.",

  "geschichte.phase7.title": "Phase 7 &mdash; WiFi Fixes &amp; mDNS",
  "geschichte.phase7.body": "WiFi scan removed (blocking problems with ESPAsyncWebServer), STA timeout increased to 20 s, DHCP hostname and mDNS implemented. The clock is now reachable as <code>nixieclockcs.local</code> on the home network.",
  "geschichte.acc7.label": "Problem: WiFi Scan Returned No Results",
  "geschichte.acc7.content": "Two causes: (1) <code>WiFi.mode(WIFI_AP)</code> disables the STA radio &mdash; no scan possible. (2) a synchronous <code>scanNetworks()</code> call inside the ESPAsyncWebServer handler blocks the LWIP/WiFi task. Fix: the scan function was removed entirely; the SSID is entered manually.",

  "geschichte.phase8.title": "Phase 8 &mdash; HV Anode Dimming",
  "geschichte.phase8.body": "Night mode no longer dims the tubes via software PWM on the cathodes, but instead via a TLP627 optocoupler directly on the ~170 V anode voltage (<code>hv_dimmer.ino</code>, LEDC hardware PWM ~200 Hz). Also: a web UI slider for dimming brightness (continuously 2&ndash;60&nbsp;%) with instant AJAX apply, the outdated Nixie-brightness dropdown removed, NeoPixel brightness steps for buttons/IR readjusted to 10/40/80/200.",
  "geschichte.acc8.label": "Observation: Web UI Delay After the Rework",
  "geschichte.acc8.content": "After the rework, the web UI felt noticeably delayed in both AP and WiFi client mode. Initial hypothesis: switching the full anode load at 200 Hz couples interference into the WiFi radio. Valid objection: both the old cathode software PWM and the new anode hardware PWM switch the same amount of current per lit digit &mdash; so the interference source wouldn't be fundamentally new. On a repeat test, the delay no longer occurred; without reproducibility, the exact cause remains unresolved.",

  "geschichte.phase9.title": "Phase 9 &mdash; Soft Digit Transition",
  "geschichte.phase9.body": "The transition between two digits can now optionally happen not abruptly but as a smooth crossfade &mdash; independently toggleable for the seconds tick and for switching between time and date display. Implemented technically via the existing HV dimmer (<code>digit_fade.ino</code>): briefly dim down, switch digits at minimum brightness, fade back up &mdash; as a non-blocking state machine, driven from <code>loop()</code>. Duration set to 400&nbsp;ms after a hardware test. Additionally, the slot-machine animation's roll speed (<code>slotSpeedPct</code>) was made adjustable via its own web UI slider.",
```

- [ ] **Step 3: `geschichte.html` — Script-Tags und `data-i18n`-Attribute einfügen**

Ersetze den kompletten Inhalt von `docs/website/geschichte.html` mit:

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
      <div class="page-title"><h1 data-i18n="geschichte.h1">Entwicklungsgeschichte</h1></div>
      <div class="gear-divider">&#9881;</div>
      <div class="panel">
        <p data-i18n="geschichte.intro">Die Nixie Clock Ultra entstand in acht Entwicklungsphasen zwischen April und Juli 2026. Jede Phase brachte neue Erkenntnisse &mdash; und neue Probleme, die es zu lösen galt.</p>
      </div>

      <div class="timeline">

        <div class="timeline-item">
          <div class="timeline-date">2026-04-22</div>
          <div class="timeline-title" data-i18n="geschichte.phase1.title">Phase 1 &mdash; Firmware-Grundgerüst</div>
          <div class="timeline-body">
            <p data-i18n="geschichte.phase1.body">Vollständige Basis-Firmware in einer einzigen <code>.ino</code>-Datei: Nixie-Ansteuerung via Multiplex-ISR, NeoPixel-Animationen, Web-Interface (WiFi AP, AsyncWebServer), IR-Fernbedienung mit 7 Funktionen, DS1302 RTC-Integration.</p>
          </div>
        </div>

        <div class="timeline-item">
          <div class="timeline-date">2026-04-23</div>
          <div class="timeline-title" data-i18n="geschichte.phase2.title">Phase 2 &mdash; Modularisierung</div>
          <div class="timeline-body">
            <p data-i18n="geschichte.phase2.body">Die wachsende Datei wurde in sieben Module aufgeteilt: <code>rtc.ino</code>, <code>display.ino</code>, <code>neo_animation.ino</code>, <code>buttons.ino</code>, <code>ir_remote.ino</code>, <code>web_server.ino</code> und den Multiplex-Timer.</p>
            <div class="akkordeon">
              <input type="checkbox" id="acc-1">
              <label class="akkordeon-label" for="acc-1"><span data-i18n="geschichte.acc1.label">Problem: Forward Declarations</span> <span class="akkordeon-arrow">&#9654;</span></label>
              <div class="akkordeon-content" data-i18n="geschichte.acc1.content">Der Arduino-Präprozessor erkennt Forward Declarations für <code>IRAM_ATTR</code>-Funktionen und Web-Server-Callbacks nicht automatisch. Lösung: Forward Declarations manuell in <code>NixieClockUltra.ino</code> ergänzt.</div>
            </div>
          </div>
        </div>

        <div class="timeline-item">
          <div class="timeline-date">2026-05-03</div>
          <div class="timeline-title" data-i18n="geschichte.phase3.title">Phase 3 &mdash; Hardware-Inbetriebnahme</div>
          <div class="timeline-body">
            <p data-i18n="geschichte.phase3.body">Erste Tests an der echten Hardware brachten mehrere Korrekturen: RGB-Bytereihenfolge der THT-WS2812B (RGB statt GRB), Kathoden-Mapping-Korrektur, separate Helligkeitsregelung für Hintergrund und Trennpunkte.</p>
            <div class="akkordeon">
              <input type="checkbox" id="acc-2">
              <label class="akkordeon-label" for="acc-2"><span data-i18n="geschichte.acc2.label">Problem: NeoPixel RGB-Swap</span> <span class="akkordeon-arrow">&#9654;</span></label>
              <div class="akkordeon-content" data-i18n="geschichte.acc2.content">Die 4 Trennpunkt-LEDs (THT WS2812B YF923) zeigten falsche Farben. Ursache: THT-Variante verwendet RGB-Bytereihenfolge statt GRB wie die SMD-Version. Lösung: <code>rgbSwap</code> für Pixel 6&ndash;9 in <code>neo_animation.ino</code>.</div>
            </div>
            <div class="akkordeon">
              <input type="checkbox" id="acc-3">
              <label class="akkordeon-label" for="acc-3"><span data-i18n="geschichte.acc3.label">Problem: Ghosting (erste Versuche)</span> <span class="akkordeon-arrow">&#9654;</span></label>
              <div class="akkordeon-content" data-i18n="geschichte.acc3.content">Nixie-Röhren zeigten Geisterziffern durch den Multiplex-Betrieb. Zwei Versuche: (1) Anode vor Kathode schalten &mdash; kein Erfolg. (2) Blank-Phase verlängern &mdash; reduziert, aber nicht beseitigt. Strukturelle Lösung erst in Phase 4.</div>
            </div>
          </div>
        </div>

        <div class="timeline-item">
          <div class="timeline-date">2026-05-09</div>
          <div class="timeline-title" data-i18n="geschichte.phase4.title">Phase 4 &mdash; Anti-Ghosting: MCP23017 Direct Drive</div>
          <div class="timeline-body">
            <p data-i18n="geschichte.phase4.body">Kompletter Umbau der Nixie-Ansteuerung: statt eines 74HC595-Schieberegisters mit Multiplex-ISR nun vier MCP23017 I²C-Port-Expander mit je eigenem SMBTA42-Transistor pro Kathode. Kein Multiplexing mehr &mdash; alle Röhren leuchten dauerhaft.</p>
            <div class="akkordeon">
              <input type="checkbox" id="acc-4">
              <label class="akkordeon-label" for="acc-4"><span data-i18n="geschichte.acc4.label">Ergebnis (getestet 2026-06-15)</span> <span class="akkordeon-arrow">&#9654;</span></label>
              <div class="akkordeon-content" data-i18n="geschichte.acc4.content">Ghosting vollständig beseitigt. Die direkte Ansteuerung via MCP23017 löst das Problem strukturell: Da keine Kathode mehr gemeinsam genutzt wird, gibt es keine Übersprecher zwischen den Röhren.</div>
            </div>
          </div>
        </div>

        <div class="timeline-item">
          <div class="timeline-date">2026-06-15</div>
          <div class="timeline-title" data-i18n="geschichte.phase5.title">Phase 5 &mdash; Dokumentation</div>
          <div class="timeline-body">
            <p data-i18n="geschichte.phase5.body">Vollständige Bedienungsanleitung im Steampunk-Design (HTML &rarr; PDF) sowie technische Systemdokumentation (ODT) mit eingebetteten Schaltplänen, API-Dokumentation und Bibliotheksübersicht.</p>
          </div>
        </div>

        <div class="timeline-item">
          <div class="timeline-date">2026-06-17</div>
          <div class="timeline-title" data-i18n="geschichte.phase6.title">Phase 6 &mdash; Slot-Intervall &amp; Feinschliff</div>
          <div class="timeline-body">
            <p data-i18n="geschichte.phase6.body">Slot-Machine-Animation als eigenständige Einstellung (<code>SlotInterval</code>-Enum) ausgelagert. Mehrere Hardware-Bugs behoben: IR-Empfang nach <code>strip.show()</code>, Taster-Entprellung, Pin-Korrekturen, Warmweiß-Farbabgleich.</p>
            <div class="akkordeon">
              <input type="checkbox" id="acc-5">
              <label class="akkordeon-label" for="acc-5"><span data-i18n="geschichte.acc5.label">Problem: IR-Empfang nach NeoPixel-Update</span> <span class="akkordeon-arrow">&#9654;</span></label>
              <div class="akkordeon-content" data-i18n="geschichte.acc5.content">IR-Codes wurden nach <code>strip.show()</code> nicht mehr empfangen. Ursache: <code>irrecv.pause()/resume()</code> um <code>strip.show()</code> resettete den RMT-Empfänger alle 20 ms. Lösung: pause/resume vollständig entfernt. Auf dem ESP32-S3 sind RMT-TX (NeoPixel) und RMT-RX (IR) unabhängige Kanäle &mdash; kein Konflikt.</div>
            </div>
            <div class="akkordeon">
              <input type="checkbox" id="acc-6">
              <label class="akkordeon-label" for="acc-6"><span data-i18n="geschichte.acc6.label">Problem: Taster-Entprellung</span> <span class="akkordeon-arrow">&#9654;</span></label>
              <div class="akkordeon-content" data-i18n="geschichte.acc6.content"><code>pressed</code> wurde nie <code>true</code>. Ursache: Logikfehler in der Button-FSM &mdash; das <code>debounced</code>-Flag wurde nie gesetzt, weil die Bedingung <code>lastState == HIGH</code> innerhalb des Debounce-Fensters niemals zutraf. Lösung: <code>Button</code>-Struct um <code>debounced</code>-Flag erweitert.</div>
            </div>
          </div>
        </div>

        <div class="timeline-item">
          <div class="timeline-date">2026-06-20</div>
          <div class="timeline-title" data-i18n="geschichte.phase7.title">Phase 7 &mdash; WiFi-Fixes &amp; mDNS</div>
          <div class="timeline-body">
            <p data-i18n="geschichte.phase7.body">WiFi-Scan entfernt (Blocking-Probleme mit ESPAsyncWebServer), STA-Timeout auf 20 s erhöht, DHCP-Hostname und mDNS implementiert. Die Uhr ist jetzt als <code>nixieclockcs.local</code> im Heimnetz erreichbar.</p>
            <div class="akkordeon">
              <input type="checkbox" id="acc-7">
              <label class="akkordeon-label" for="acc-7"><span data-i18n="geschichte.acc7.label">Problem: WiFi-Scan lieferte keine Ergebnisse</span> <span class="akkordeon-arrow">&#9654;</span></label>
              <div class="akkordeon-content" data-i18n="geschichte.acc7.content">Zwei Ursachen: (1) <code>WiFi.mode(WIFI_AP)</code> deaktiviert das STA-Radio &mdash; kein Scan möglich. (2) Synchroner <code>scanNetworks()</code>-Aufruf im ESPAsyncWebServer-Handler blockiert den LWIP/WiFi-Task. Lösung: Scan-Funktion vollständig entfernt, SSID wird manuell eingegeben.</div>
            </div>
          </div>
        </div>

        <div class="timeline-item">
          <div class="timeline-date">2026-07-07</div>
          <div class="timeline-title" data-i18n="geschichte.phase8.title">Phase 8 &mdash; HV-Anodendimmung</div>
          <div class="timeline-body">
            <p data-i18n="geschichte.phase8.body">Der Nacht-Modus dimmt die Röhren jetzt nicht mehr per Software-PWM auf den Kathoden, sondern über einen TLP627-Optokoppler direkt auf der ~170-V-Anodenspannung (<code>hv_dimmer.ino</code>, LEDC-Hardware-PWM ~200 Hz). Dazu: Web-UI-Regler für die Dimm-Helligkeit (stufenlos 2&ndash;60&nbsp;%) mit sofortiger AJAX-Übernahme, veraltetes Nixie-Helligkeit-Dropdown entfernt, NeoPixel-Helligkeitsstufen für Taster/IR auf 10/40/80/200 nachjustiert.</p>
            <div class="akkordeon">
              <input type="checkbox" id="acc-8">
              <label class="akkordeon-label" for="acc-8"><span data-i18n="geschichte.acc8.label">Beobachtung: Web-UI-Verzögerung nach dem Umbau</span> <span class="akkordeon-arrow">&#9654;</span></label>
              <div class="akkordeon-content" data-i18n="geschichte.acc8.content">Nach dem Umbau wirkte die Web-UI sowohl im AP- als auch im WLAN-Modus spürbar verzögert. Erste Hypothese: Das Schalten der vollen Anodenlast mit 200 Hz koppelt Störungen in den WLAN-Funkteil ein. Berechtigter Einwand: Sowohl beim alten Kathoden-Software-PWM als auch beim neuen Anoden-Hardware-PWM wird pro leuchtender Ziffer dieselbe Strommenge geschaltet &mdash; die Störquelle wäre also nicht grundsätzlich neu. Bei einem erneuten Test trat die Verzögerung nicht mehr auf; mangels Reproduzierbarkeit bleibt die genaue Ursache ungeklärt.</div>
            </div>
          </div>
        </div>

        <div class="timeline-item">
          <div class="timeline-date">2026-07-11</div>
          <div class="timeline-title" data-i18n="geschichte.phase9.title">Phase 9 &mdash; Weicher Ziffernwechsel</div>
          <div class="timeline-body">
            <p data-i18n="geschichte.phase9.body">Der Wechsel zwischen zwei Ziffern erfolgt jetzt optional nicht mehr schlagartig, sondern als sanfter Crossfade &mdash; getrennt zuschaltbar für den Sekundentakt und für den Wechsel zwischen Uhrzeit- und Datumsanzeige. Technisch über den vorhandenen HV-Dimmer realisiert (<code>digit_fade.ino</code>): kurz abblenden, Ziffern bei Minimalhelligkeit umschalten, wieder aufblenden &mdash; als non-blocking State-Machine, angetrieben aus <code>loop()</code>. Dauer nach einem Hardware-Test auf 400&nbsp;ms festgelegt. Zusätzlich wurde die Rollgeschwindigkeit der Slot-Machine-Animation (<code>slotSpeedPct</code>) über einen eigenen Web-UI-Regler einstellbar gemacht.</p>
          </div>
        </div>

      </div><!-- /timeline -->
    </div>
  </main>
  <footer id="site-footer"></footer>
  <script src="js/nav.js"></script>
  <script src="js/i18n-de.js"></script>
  <script src="js/i18n-en.js"></script>
  <script src="js/i18n.js"></script>
</body>
</html>
```

**Hinweis:** Die Akkordeon-`<label>`s enthalten neben dem `data-i18n`-`<span>` noch den (nicht übersetzten) Pfeil-`<span class="akkordeon-arrow">`. Da `applyLanguage()` nur den `<span data-i18n="...">` per `innerHTML` ersetzt, bleibt der Pfeil unberührt — das Akkordeon-Label wurde dafür von reinem Text auf ein verschachteltes `<span>` umgestellt.

- [ ] **Step 4: Manuelle Verifikation im Browser**

`http://localhost:8080/geschichte.html` öffnen.

Prüfen:
- DE: Alle 9 Phasen-Titel, Body-Texte und alle 8 Akkordeons (Labels + Inhalte) wie im Original
- Akkordeons auf-/zuklappen funktioniert weiterhin (CSS-only, von i18n unberührt), Pfeil-Rotation bleibt erhalten
- EN: Alle Titel/Texte/Akkordeons übersetzt, Datumsangaben (z. B. „2026-04-22“) bleiben unverändert (nicht Teil eines `data-i18n`), `<code>`-Snippets bleiben als Code formatiert

Expected: Alle Punkte treffen zu.

- [ ] **Step 5: Commit**

```bash
git add docs/website/js/i18n-de.js docs/website/js/i18n-en.js docs/website/geschichte.html
git commit -m "feat: Sprachumschalter DE/EN – Geschichte-Seite übersetzt"
```

---

### Task 5: dokumentation.html

**Files:**
- Modify: `docs/website/js/i18n-de.js`
- Modify: `docs/website/js/i18n-en.js`
- Modify: `docs/website/dokumentation.html`

**Interfaces:**
- Consumes: `I18N_DE`/`I18N_EN`, `data-i18n`-Konvention aus Task 1
- Produces: `dokumentation.*`-Keys (keine anderen Tasks hängen davon ab)

- [ ] **Step 1: Keys zu `js/i18n-de.js` hinzufügen**

Füge vor der schließenden `};`-Zeile in `docs/website/js/i18n-de.js` ein:

```js

  "dokumentation.meta.title": "Dokumentation – Nixie Clock Ultra",
  "dokumentation.h1": "Dokumentation &amp; Downloads",
  "dokumentation.systemdoku.title": "Systemdokumentation",
  "dokumentation.systemdoku.desc": "Vollständige technische Dokumentation: Hardware-Aufbau beider Platinen, Schaltpläne, Firmware-Architektur, API-Endpoints, NVS-Persistenz und Entwickler-Workflow.",
  "dokumentation.anleitung.title": "Bedienungsanleitung",
  "dokumentation.anleitung.desc": "Vollständige Bedienungsanleitung im Steampunk-Design: Inbetriebnahme, Tastenbedienung, Zeiteinstellung, Beleuchtungssteuerung, IR-Fernbedienung, WLAN-Einrichtung und technische Daten.",
  "dokumentation.germanonly": "(nur auf Deutsch verfügbar)",
  "dokumentation.note.libreoffice": "&#9432; ODT-Dateien können mit <a href=\"https://www.libreoffice.org\" target=\"_blank\" rel=\"noopener\">LibreOffice</a> (kostenlos) geöffnet werden.",
```

- [ ] **Step 2: Keys zu `js/i18n-en.js` hinzufügen**

Füge vor der schließenden `};`-Zeile in `docs/website/js/i18n-en.js` ein:

```js

  "dokumentation.meta.title": "Documentation – Nixie Clock Ultra",
  "dokumentation.h1": "Documentation &amp; Downloads",
  "dokumentation.systemdoku.title": "System Documentation",
  "dokumentation.systemdoku.desc": "Complete technical documentation: hardware design of both boards, schematics, firmware architecture, API endpoints, NVS persistence, and developer workflow.",
  "dokumentation.anleitung.title": "User Manual",
  "dokumentation.anleitung.desc": "Complete user manual in steampunk design: setup, button operation, time setting, lighting control, IR remote, WiFi setup, and technical specifications.",
  "dokumentation.germanonly": "(German only)",
  "dokumentation.note.libreoffice": "&#9432; ODT files can be opened with <a href=\"https://www.libreoffice.org\" target=\"_blank\" rel=\"noopener\">LibreOffice</a> (free).",
```

- [ ] **Step 3: `dokumentation.html` — Script-Tags, `data-i18n`-Attribute und German-only-Hinweise einfügen**

Ersetze den kompletten Inhalt von `docs/website/dokumentation.html` mit:

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
      <div class="page-title"><h1 data-i18n="dokumentation.h1">Dokumentation &amp; Downloads</h1></div>
      <div class="gear-divider">&#9881;</div>

      <div class="download-grid">

        <div class="download-card">
          <svg class="download-icon" viewBox="0 0 100 100" fill="none" xmlns="http://www.w3.org/2000/svg" aria-hidden="true">
            <path d="M43 5h14l2 11a34 34 0 0 1 8 4.5l10-3.5 9.9 9.9-3.5 10A34 34 0 0 1 88 45l11 2v14l-11 2a34 34 0 0 1-4.5 8l3.5 10-9.9 9.9-10-3.5A34 34 0 0 1 59 91l-2 9H43l-2-9a34 34 0 0 1-8-4.5l-10 3.5-9.9-9.9 3.5-10A34 34 0 0 1 12 57L1 55V41l11-2a34 34 0 0 1 4.5-8l-3.5-10 9.9-9.9 10 3.5A34 34 0 0 1 41 10Z" fill="none" stroke="currentColor" stroke-width="5"/>
            <circle cx="50" cy="50" r="16" fill="none" stroke="currentColor" stroke-width="5"/>
          </svg>
          <div class="download-title" data-i18n="dokumentation.systemdoku.title">Systemdokumentation</div>
          <p class="download-desc" data-i18n="dokumentation.systemdoku.desc">Vollständige technische Dokumentation: Hardware-Aufbau beider Platinen, Schaltpläne, Firmware-Architektur, API-Endpoints, NVS-Persistenz und Entwickler-Workflow.</p>
          <div class="btn-group">
            <a href="downloads/NixieClockUltra-Systemdokumentation.odt" class="btn" download>ODT</a>
            <a href="downloads/NixieClockUltra-Systemdokumentation.pdf" class="btn btn-primary" download>PDF</a>
          </div>
          <p class="pcb-caption" data-i18n="dokumentation.germanonly">(nur auf Deutsch verfügbar)</p>
        </div>

        <div class="download-card">
          <svg class="download-icon" viewBox="0 0 100 100" fill="none" xmlns="http://www.w3.org/2000/svg" aria-hidden="true">
            <rect x="15" y="10" width="55" height="75" rx="3" fill="none" stroke="currentColor" stroke-width="5"/>
            <line x1="15" y1="85" x2="75" y2="85" stroke="currentColor" stroke-width="5"/>
            <rect x="70" y="10" width="15" height="75" rx="2" fill="none" stroke="currentColor" stroke-width="4"/>
            <line x1="30" y1="30" x2="60" y2="30" stroke="currentColor" stroke-width="3.5"/>
            <line x1="30" y1="43" x2="60" y2="43" stroke="currentColor" stroke-width="3.5"/>
            <line x1="30" y1="56" x2="50" y2="56" stroke="currentColor" stroke-width="3.5"/>
          </svg>
          <div class="download-title" data-i18n="dokumentation.anleitung.title">Bedienungsanleitung</div>
          <p class="download-desc" data-i18n="dokumentation.anleitung.desc">Vollständige Bedienungsanleitung im Steampunk-Design: Inbetriebnahme, Tastenbedienung, Zeiteinstellung, Beleuchtungssteuerung, IR-Fernbedienung, WLAN-Einrichtung und technische Daten.</p>
          <div class="btn-group">
            <a href="downloads/NixieClockUltra-Bedienungsanleitung.pdf" class="btn btn-primary" download>PDF</a>
          </div>
          <p class="pcb-caption" data-i18n="dokumentation.germanonly">(nur auf Deutsch verfügbar)</p>
        </div>

      </div>

      <div class="note-box" data-i18n="dokumentation.note.libreoffice">
        &#9432; ODT-Dateien können mit <a href="https://www.libreoffice.org" target="_blank" rel="noopener">LibreOffice</a> (kostenlos) geöffnet werden.
      </div>

    </div>
  </main>
  <footer id="site-footer"></footer>
  <script src="js/nav.js"></script>
  <script src="js/i18n-de.js"></script>
  <script src="js/i18n-en.js"></script>
  <script src="js/i18n.js"></script>
</body>
</html>
```

- [ ] **Step 4: Manuelle Verifikation im Browser**

`http://localhost:8080/dokumentation.html` öffnen.

Prüfen:
- DE: Beide Download-Karten wie im Original, kein „(nur auf Deutsch verfügbar)“-Text auffällig störend, LibreOffice-Hinweis mit funktionierendem Link
- EN: Karten-Titel/-Beschreibungen übersetzt, unter beiden Karten erscheint „(German only)“, LibreOffice-Hinweis übersetzt, Link bleibt funktionsfähig (`href` unverändert)
- Download-Buttons (ODT/PDF) bleiben in beiden Sprachen klickbar und zeigen auf dieselben (deutschen) Dateien

Expected: Alle Punkte treffen zu.

- [ ] **Step 5: Commit**

```bash
git add docs/website/js/i18n-de.js docs/website/js/i18n-en.js docs/website/dokumentation.html
git commit -m "feat: Sprachumschalter DE/EN – Dokumentation-Seite uebersetzt (Downloads bleiben Deutsch)"
```

---

### Task 6: galerie.html

**Files:**
- Modify: `docs/website/js/i18n-de.js`
- Modify: `docs/website/js/i18n-en.js`
- Modify: `docs/website/galerie.html`

**Interfaces:**
- Consumes: `I18N_DE`/`I18N_EN`, `data-i18n`-Konvention aus Task 1
- Produces: `galerie.*`-Keys (keine anderen Tasks hängen davon ab)

- [ ] **Step 1: Keys zu `js/i18n-de.js` hinzufügen**

Füge vor der schließenden `};`-Zeile in `docs/website/js/i18n-de.js` ein:

```js

  "galerie.meta.title": "Galerie – Nixie Clock Ultra",
  "galerie.h1": "Galerie",
  "galerie.empty": "Fotos werden hier eingetragen sobald img/gallery/ befüllt ist.<br>\n        Jede Fotodatei als <code>&lt;img src=\"img/gallery/xxx.jpg\" data-lightbox=\"gallery\"&gt;</code> in die gallery-grid div einfügen.",
```

- [ ] **Step 2: Keys zu `js/i18n-en.js` hinzufügen**

Füge vor der schließenden `};`-Zeile in `docs/website/js/i18n-en.js` ein:

```js

  "galerie.meta.title": "Gallery – Nixie Clock Ultra",
  "galerie.h1": "Gallery",
  "galerie.empty": "Photos will be added here once img/gallery/ is populated.<br>\n        Add each photo file as <code>&lt;img src=\"img/gallery/xxx.jpg\" data-lightbox=\"gallery\"&gt;</code> inside the gallery-grid div.",
```

- [ ] **Step 3: `galerie.html` — Script-Tags und `data-i18n`-Attribute einfügen**

Ersetze den kompletten Inhalt von `docs/website/galerie.html` mit:

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
      <div class="page-title"><h1 data-i18n="galerie.h1">Galerie</h1></div>
      <div class="gear-divider">&#9881;</div>

      <!-- Fotos hier eintragen sobald img/gallery/ befüllt ist.
           Jedes Bild als:
           <a class="gallery-item" href="img/gallery/dateiname.jpg">
             <img src="img/gallery/dateiname.jpg" alt="Kurze Bildbeschreibung" data-lightbox="gallery">
           </a>
      -->
      <div class="gallery-grid" id="gallery-grid">
        <p class="gallery-empty" data-i18n="galerie.empty">Fotos werden hier eingetragen sobald img/gallery/ befüllt ist.<br>
        Jede Fotodatei als <code>&lt;img src="img/gallery/xxx.jpg" data-lightbox="gallery"&gt;</code> in die gallery-grid div einfügen.</p>
      </div>

    </div>
  </main>
  <footer id="site-footer"></footer>
  <script src="js/nav.js"></script>
  <script src="js/i18n-de.js"></script>
  <script src="js/i18n-en.js"></script>
  <script src="js/i18n.js"></script>
  <script src="js/lightbox.js"></script>
</body>
</html>
```

- [ ] **Step 4: Manuelle Verifikation im Browser**

`http://localhost:8080/galerie.html` öffnen.

Prüfen:
- DE: Platzhaltertext wie im Original, `<code>`-Snippet und `<br>`-Zeilenumbruch korrekt dargestellt
- EN: Platzhaltertext übersetzt, gleiche Formatierung erhalten

Expected: Alle Punkte treffen zu.

- [ ] **Step 5: Commit**

```bash
git add docs/website/js/i18n-de.js docs/website/js/i18n-en.js docs/website/galerie.html
git commit -m "feat: Sprachumschalter DE/EN – Galerie-Seite uebersetzt"
```

---

### Task 7: Finale seitenübergreifende Verifikation

**Files:** Keine Änderungen erwartet — reiner Verifikationsdurchlauf. Nur bei gefundenen Problemen: Korrektur in der jeweils betroffenen Datei aus Task 1–6.

**Interfaces:**
- Consumes: Alle Artefakte aus Task 1–6 (alle 6 HTML-Seiten, `i18n.js`, `i18n-de.js`, `i18n-en.js`, `nav.js`, `style.css`)
- Produces: Nichts Neues — Abschlussbestätigung, dass das Gesamtsystem seitenübergreifend funktioniert

- [ ] **Step 1: Server starten (falls nicht mehr aktiv)**

Run: `cd docs/website && python3 -m http.server 8080`

- [ ] **Step 2: Persistenz über alle 6 Seiten prüfen**

Browser: `http://localhost:8080/index.html` öffnen, `localStorage.clear()` in der Konsole ausführen, Seite neu laden (Start: Deutsch). Auf „EN“ klicken, dann nacheinander über die Nav zu Features, Aufbau, Geschichte, Dokumentation, Galerie navigieren.

Expected: Jede Seite lädt sofort auf Englisch (kein Zurückfallen auf Deutsch zwischendurch), Tab-Titel wechselt pro Seite korrekt (z. B. „Documentation – Nixie Clock Ultra“ auf `dokumentation.html`), aktiver Nav-Link (`.active`-Klasse) bleibt weiterhin korrekt markiert.

- [ ] **Step 3: Zurück auf Deutsch, gleicher Rundgang**

Auf „DE“ klicken (auf einer beliebigen Seite), erneut alle 6 Seiten durchklicken.

Expected: Alle Seiten zeigen wieder den deutschen Originaltext, keine gemischten Sprachreste (z. B. englischer Fließtext mit deutschem Seitentitel oder umgekehrt).

- [ ] **Step 4: Mobile-Ansicht prüfen**

Browser-Fenster auf < 768px verkleinern (oder DevTools-Gerätesimulation), auf 2–3 der Seiten das Hamburger-Menü öffnen/schließen und den Sprachumschalter betätigen.

Expected: Hamburger-Menü funktioniert wie vor der Änderung (Off-Canvas-Liste öffnet/schließt), Sprachumschalter bleibt rechts sichtbar und bedienbar, keine Überlappung mit dem Hamburger-Button.

- [ ] **Step 5: JS-Fallback prüfen**

DevTools → JavaScript deaktivieren (oder `<script>`-Tags temporär im DOM-Inspector deaktivieren), eine Seite neu laden.

Expected: Seite bleibt vollständig auf Deutsch lesbar (Fallback-Inhalt aus dem HTML), keine leeren oder kaputten Textstellen. JavaScript danach wieder aktivieren.

- [ ] **Step 6: Falls alle Punkte bestehen — kein weiterer Commit nötig**

Falls Schritt 2–5 Abweichungen zeigen: Ursache in der betroffenen Datei beheben und committen:

```bash
git add -A
git commit -m "fix: Nachbesserung Sprachumschalter nach seitenübergreifender Verifikation"
```

Falls keine Abweichungen gefunden wurden, ist die Implementierung abgeschlossen — kein zusätzlicher Commit nötig.

---

## Plan Self-Review

**Spec-Abdeckung:** Alle 6 Seiten (Task 1–6), Umschalter in Nav rechts (Task 1, CSS + nav.js), `localStorage`-Persistenz mit Default Deutsch (Task 1, `i18n.js`), Titel-Übersetzung (`meta.title`-Keys, jede Seiten-Task), Downloads bleiben Deutsch + Hinweis (Task 5), Fallback ohne JS (alle Tasks, deutscher Text bleibt im HTML), manuelle Verifikation statt automatisierter Tests (jede Task, Task 7 zusätzlich seitenübergreifend) — alle Spec-Abschnitte sind abgedeckt.

**Platzhalter-Scan:** Keine TBD/TODO-Marker, jeder Übersetzungs-Key hat einen vollständig ausformulierten deutschen und englischen Wert, keine „ähnlich wie Task N“-Verweise ohne Code.

**Typ-/Namenskonsistenz:** `getLanguage()`, `setLanguage(lang)`, `applyLanguage(lang)`, `I18N_DE`, `I18N_EN` werden in Task 1 definiert und in Task 2–7 unverändert weiterverwendet, keine abweichenden Funktionsnamen. Key-Schema `<seite>.<bereich>.<element>` durchgängig eingehalten.
