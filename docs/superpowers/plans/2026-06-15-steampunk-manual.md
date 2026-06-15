# Steampunk Bedienungsanleitung – Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Einzelne selbst-enthaltene HTML-Datei erzeugen, die die vollständige Bedienungsanleitung der Nixie Clock Ultra im Steampunk-Stil (Elegant & Zeitlos) darstellt und per Browser-Druck (Strg+P → Als PDF speichern) als druckfertiges A4-PDF exportiert werden kann.

**Architecture:** Eine einzige HTML-Datei mit eingebetteten CSS-Custom-Properties, inline SVG-Dekorationen (keine externen Abhängigkeiten), @media print-Regeln für A4-Paginierung und seitenweise `<div class="page">`-Blöcke. Kein JavaScript, keine externen Fonts, keine Bilder.

**Tech Stack:** HTML5, CSS3 (Custom Properties, @media print, @page), inline SVG

---

## Dateistruktur

| Datei | Aktion | Beschreibung |
|-------|--------|--------------|
| `docs/manual/nixie-clock-ultra-bedienungsanleitung.html` | Neu erstellen | Vollständiges HTML-Dokument |

---

## Wiederverwendete SVG-Snippets (Referenz für alle Tasks)

Diese SVG-Bausteine werden in mehreren Tasks verwendet. Hier zentral definiert:

**Corner-Ornament oben-links** (analog für TR/BL/BR mit gespiegelten Koordinaten):
```html
<svg class="corner-tl" width="36" height="36" viewBox="0 0 36 36">
  <path d="M2,2 L2,16 M2,2 L16,2" stroke="#b8860b" stroke-width="1.4" fill="none"/>
  <circle cx="2" cy="2" r="2" fill="#b8860b"/>
</svg>
<svg class="corner-tr" width="36" height="36" viewBox="0 0 36 36">
  <path d="M34,2 L34,16 M34,2 L20,2" stroke="#b8860b" stroke-width="1.4" fill="none"/>
  <circle cx="34" cy="2" r="2" fill="#b8860b"/>
</svg>
<svg class="corner-bl" width="36" height="36" viewBox="0 0 36 36">
  <path d="M2,34 L2,20 M2,34 L16,34" stroke="#b8860b" stroke-width="1.4" fill="none"/>
  <circle cx="2" cy="34" r="2" fill="#b8860b"/>
</svg>
<svg class="corner-br" width="36" height="36" viewBox="0 0 36 36">
  <path d="M34,34 L34,20 M34,34 L20,34" stroke="#b8860b" stroke-width="1.4" fill="none"/>
  <circle cx="34" cy="34" r="2" fill="#b8860b"/>
</svg>
```

**Ornamentlinie (groß):**
```html
<svg class="ornament-line" width="220" height="14" viewBox="0 0 220 14">
  <line x1="0" y1="7" x2="88" y2="7" stroke="#b8860b" stroke-width="1"/>
  <polygon points="100,7 106,3 112,7 106,11" fill="none" stroke="#b8860b" stroke-width="1"/>
  <line x1="122" y1="7" x2="220" y2="7" stroke="#b8860b" stroke-width="1"/>
</svg>
```

**Ornamentlinie (klein, Fußlinie):**
```html
<svg class="ornament-small" width="80" height="10" viewBox="0 0 80 10">
  <line x1="0" y1="5" x2="28" y2="5" stroke="#b8860b" stroke-width="0.8"/>
  <circle cx="40" cy="5" r="3" fill="none" stroke="#b8860b" stroke-width="0.8"/>
  <line x1="52" y1="5" x2="80" y2="5" stroke="#b8860b" stroke-width="0.8"/>
</svg>
```

**Zahnrad-Icon (Kapitelheader):**
```html
<svg width="30" height="30" viewBox="0 0 30 30" style="flex-shrink:0">
  <g transform="translate(15,15)">
    <circle r="5.5" fill="var(--pg-light)" stroke="var(--gold)" stroke-width="1.1"/>
    <circle r="2.2" fill="var(--gold)"/>
    <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(0)"/>
    <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(45)"/>
    <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(90)"/>
    <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(135)"/>
    <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(180)"/>
    <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(225)"/>
    <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(270)"/>
    <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(315)"/>
  </g>
</svg>
```

**Warn-Dreieck:**
```html
<svg width="20" height="20" viewBox="0 0 20 20" style="flex-shrink:0;margin-top:2px">
  <polygon points="10,2 18,17 2,17" fill="none" stroke="#8b0000" stroke-width="1.3" stroke-linejoin="round"/>
  <line x1="10" y1="8" x2="10" y2="13" stroke="#8b0000" stroke-width="1.3"/>
  <circle cx="10" cy="15" r="0.9" fill="#8b0000"/>
</svg>
```

**Laufender Kopfbereich (alle Kapitelseiten):**
```html
<div class="running-header">
  <span>Nixie Clock Ultra · Bedienungsanleitung</span>
  <span>⚙ [SEITENNUMMER]</span>
</div>
```

**Kapitelheader-Block:**
```html
<div class="chapter-title">
  [Zahnrad-SVG]
  <div>
    <div class="chapter-label">Kapitel [RÖMISCH]</div>
    <h1>[TITEL]</h1>
  </div>
</div>
<div class="chapter-rule"></div>
```

---

## Task 1: HTML-Grundgerüst & vollständiges CSS

**Files:**
- Erstellen: `docs/manual/nixie-clock-ultra-bedienungsanleitung.html`

- [ ] **Schritt 1.1: Datei mit HTML-Skeleton und CSS anlegen**

Erstelle `docs/manual/nixie-clock-ultra-bedienungsanleitung.html` mit folgendem Inhalt:

```html
<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Nixie Clock Ultra – Bedienungsanleitung</title>
<style>
/* ── Custom Properties ─────────────────────────────────────────── */
:root {
  --pg-light: #f7f2e8;
  --pg-dark:  #f0ebe0;
  --text-h:   #2c1810;
  --text-b:   #3d2b00;
  --gold:     #b8860b;
  --gold-dk:  #8b6914;
  --gold-lt:  #d4af37;
  --red-warn: #8b0000;
  --line:     #d3c9a8;
  --line-med: #c8b878;
  --hdr-txt:  #a08040;
  --warn-bg:  #fff8f0;
}

/* ── Reset ─────────────────────────────────────────────────────── */
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

/* ── Body & Screen Layout ──────────────────────────────────────── */
body {
  font-family: Georgia, 'Times New Roman', serif;
  background: #b0a898;
  color: var(--text-b);
}

/* ── Page Block ────────────────────────────────────────────────── */
.page {
  width: 210mm;
  min-height: 297mm;
  margin: 10mm auto;
  background: linear-gradient(180deg, var(--pg-light) 0%, var(--pg-dark) 100%);
  padding: 18mm 22mm 16mm;
  position: relative;
  box-shadow: 0 4px 28px rgba(0,0,0,0.22);
}

/* ── Corner Ornaments ──────────────────────────────────────────── */
.corner-tl { position: absolute; top: 8mm; left: 8mm; opacity: 0.55; }
.corner-tr { position: absolute; top: 8mm; right: 8mm; opacity: 0.55; }
.corner-bl { position: absolute; bottom: 8mm; left: 8mm; opacity: 0.55; }
.corner-br { position: absolute; bottom: 8mm; right: 8mm; opacity: 0.55; }

/* ── Running Header ────────────────────────────────────────────── */
.running-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 10mm;
  padding-bottom: 2.5mm;
  border-bottom: 0.3mm solid var(--line-med);
  font-size: 7.5pt;
  color: var(--hdr-txt);
  font-style: italic;
}

/* ── Chapter Title ─────────────────────────────────────────────── */
.chapter-title {
  display: flex;
  align-items: center;
  gap: 10px;
  margin-bottom: 3mm;
}
.chapter-label {
  font-size: 7.5pt;
  color: var(--gold-dk);
  letter-spacing: 4px;
  text-transform: uppercase;
  margin-bottom: 1mm;
}
.chapter-title h1 {
  font-size: 18pt;
  color: var(--text-h);
  letter-spacing: 0.5px;
  font-weight: bold;
  line-height: 1.2;
}
.chapter-rule {
  height: 0.3mm;
  background: linear-gradient(90deg, var(--gold), transparent);
  margin-bottom: 6mm;
}

/* ── Typography ────────────────────────────────────────────────── */
p  { font-size: 10.5pt; line-height: 1.75; margin-bottom: 3mm; color: var(--text-b); }
h2 { font-size: 11pt; color: var(--gold-dk); letter-spacing: 2px;
     text-transform: uppercase; margin: 5mm 0 2mm; font-weight: bold; }

/* ── Section Label ─────────────────────────────────────────────── */
.section-label {
  font-size: 8.5pt;
  color: var(--gold-dk);
  font-weight: bold;
  letter-spacing: 2px;
  text-transform: uppercase;
  margin: 5mm 0 2mm;
  border-bottom: 0.2mm solid var(--line);
  padding-bottom: 1mm;
}

/* ── Data Table ────────────────────────────────────────────────── */
.data-table { width: 100%; border-collapse: collapse; font-size: 9.5pt; margin: 2mm 0 5mm; }
.data-table thead tr { background: var(--gold-dk); }
.data-table thead th {
  color: var(--pg-light); padding: 5px 9px;
  text-align: left; font-weight: normal; letter-spacing: 0.5px;
}
.data-table tbody tr:nth-child(odd)  { background: var(--pg-light); }
.data-table tbody tr:nth-child(even) { background: var(--pg-dark); }
.data-table tbody td {
  padding: 5px 9px; border-bottom: 0.2mm solid var(--line); color: var(--text-b);
}
.data-table tbody td strong { color: var(--text-h); }
.data-table tbody td em { color: #777; font-style: normal; }

/* ── Warning Box ───────────────────────────────────────────────── */
.warning-box {
  display: flex; gap: 9px; align-items: flex-start;
  border: 0.3mm solid var(--gold);
  border-left: 1.2mm solid var(--red-warn);
  background: var(--warn-bg);
  padding: 7px 11px;
  border-radius: 0 3px 3px 0;
  margin: 4mm 0;
}
.warn-title {
  font-size: 8pt; color: var(--red-warn); font-weight: bold;
  letter-spacing: 1px; text-transform: uppercase; margin-bottom: 2px;
}
.warning-box p { font-size: 8.5pt; color: #5a2000; margin: 0; line-height: 1.5; }

/* ── Numbered Steps ────────────────────────────────────────────── */
.steps { display: flex; flex-direction: column; gap: 2.5mm; margin: 3mm 0 5mm; }
.step  { display: flex; gap: 9px; align-items: flex-start; }
.step-num {
  width: 19px; height: 19px; flex-shrink: 0; border-radius: 50%;
  background: var(--gold-dk); color: var(--pg-light);
  font-size: 8pt; display: flex; align-items: center; justify-content: center;
  margin-top: 1px;
}
.step p { margin: 0; font-size: 10pt; line-height: 1.65; }

/* ── Info Box ──────────────────────────────────────────────────── */
.info-box {
  border: 0.3mm solid var(--gold);
  border-left: 1.2mm solid var(--gold-dk);
  background: #faf7ef;
  padding: 7px 11px;
  border-radius: 0 3px 3px 0;
  margin: 4mm 0;
  font-size: 9.5pt;
  color: var(--text-b);
  line-height: 1.6;
}

/* ── Ornament Lines ────────────────────────────────────────────── */
.ornament-line  { display: block; margin: 5mm auto; }
.ornament-small { display: block; margin: 6mm auto; }

/* ── TOC ───────────────────────────────────────────────────────── */
.toc-heading {
  text-align: center;
  font-size: 10.5pt;
  color: var(--gold-dk);
  letter-spacing: 6px;
  text-transform: uppercase;
  margin-bottom: 6mm;
}
.toc-entry {
  display: flex; align-items: baseline; gap: 4px;
  padding: 5px 0; border-bottom: 0.2mm dotted var(--line-med);
  font-size: 10.5pt;
}
.toc-entry:last-child { border-bottom: none; }
.toc-num   { color: var(--gold-dk); min-width: 32px; font-style: italic; font-size: 9pt; }
.toc-title { color: var(--text-h); flex: 1; }
.toc-page  { color: var(--hdr-txt); font-size: 9pt; text-align: right; min-width: 18px; }

/* ── Connectivity Info Box ─────────────────────────────────────── */
.conn-table { width: 100%; border-collapse: collapse; font-size: 9.5pt; margin: 2mm 0 4mm; }
.conn-table td { padding: 4px 8px; border-bottom: 0.2mm solid var(--line); }
.conn-table td:first-child { color: var(--gold-dk); font-weight: bold; width: 40%; }
.conn-table td:last-child  { color: var(--text-b); font-family: 'Courier New', monospace; font-size: 9pt; }

/* ── Print ─────────────────────────────────────────────────────── */
@media print {
  @page { size: A4 portrait; margin: 0; }
  body  { background: none; }
  .page {
    margin: 0; box-shadow: none;
    width: 100%; min-height: 100vh;
    page-break-after: always;
  }
  .no-break { page-break-inside: avoid; }
  thead { display: table-header-group; }
}
</style>
</head>
<body>

<!-- ═══════════ TITELSEITE ═══════════ -->
<!-- Task 2 -->

<!-- ═══════════ INHALTSVERZEICHNIS ═══════════ -->
<!-- Task 3 -->

<!-- ═══════════ KAPITEL I ═══════════ -->
<!-- Task 4 -->

<!-- ═══════════ KAPITEL II ═══════════ -->
<!-- Task 4 -->

<!-- ═══════════ KAPITEL III ═══════════ -->
<!-- Task 5 -->

<!-- ═══════════ KAPITEL IV ═══════════ -->
<!-- Task 5 -->

<!-- ═══════════ KAPITEL V ═══════════ -->
<!-- Task 6 -->

<!-- ═══════════ KAPITEL VI ═══════════ -->
<!-- Task 7 -->

<!-- ═══════════ KAPITEL VII ═══════════ -->
<!-- Task 8 -->

<!-- ═══════════ KAPITEL VIII ═══════════ -->
<!-- Task 9 -->

<!-- ═══════════ KAPITEL IX ═══════════ -->
<!-- Task 9 -->

</body>
</html>
```

- [ ] **Schritt 1.2: Im Browser prüfen**

Datei öffnen: `docs/manual/nixie-clock-ultra-bedienungsanleitung.html`

Erwartetes Ergebnis: Leere Seite mit Pergament-Hintergrund auf grauem Körper.

- [ ] **Schritt 1.3: Commit**

```bash
git add docs/manual/nixie-clock-ultra-bedienungsanleitung.html
git commit -m "feat: HTML-Grundgerüst und CSS für Steampunk-Bedienungsanleitung"
```

---

## Task 2: Titelseite

**Files:**
- Modify: `docs/manual/nixie-clock-ultra-bedienungsanleitung.html` — `<!-- Task 2 -->` ersetzen

- [ ] **Schritt 2.1: Titelseite einfügen**

Ersetze `<!-- Task 2 -->` durch:

```html
<div class="page">
  <!-- Eckverzierungen -->
  <svg class="corner-tl" width="36" height="36" viewBox="0 0 36 36">
    <path d="M2,2 L2,16 M2,2 L16,2" stroke="#b8860b" stroke-width="1.4" fill="none"/>
    <circle cx="2" cy="2" r="2" fill="#b8860b"/>
  </svg>
  <svg class="corner-tr" width="36" height="36" viewBox="0 0 36 36">
    <path d="M34,2 L34,16 M34,2 L20,2" stroke="#b8860b" stroke-width="1.4" fill="none"/>
    <circle cx="34" cy="2" r="2" fill="#b8860b"/>
  </svg>
  <svg class="corner-bl" width="36" height="36" viewBox="0 0 36 36">
    <path d="M2,34 L2,20 M2,34 L16,34" stroke="#b8860b" stroke-width="1.4" fill="none"/>
    <circle cx="2" cy="34" r="2" fill="#b8860b"/>
  </svg>
  <svg class="corner-br" width="36" height="36" viewBox="0 0 36 36">
    <path d="M34,34 L34,20 M34,34 L20,34" stroke="#b8860b" stroke-width="1.4" fill="none"/>
    <circle cx="34" cy="34" r="2" fill="#b8860b"/>
  </svg>

  <!-- Ornamentlinie oben -->
  <svg class="ornament-line" width="220" height="14" viewBox="0 0 220 14">
    <line x1="0" y1="7" x2="88" y2="7" stroke="#b8860b" stroke-width="1"/>
    <polygon points="100,7 106,3 112,7 106,11" fill="none" stroke="#b8860b" stroke-width="1"/>
    <line x1="122" y1="7" x2="220" y2="7" stroke="#b8860b" stroke-width="1"/>
  </svg>

  <!-- Titelblock -->
  <div style="text-align:center;margin:10mm 0 8mm">
    <!-- Zahnrad -->
    <svg width="52" height="52" viewBox="0 0 52 52" style="display:block;margin:0 auto 6mm">
      <g transform="translate(26,26)">
        <circle r="10" fill="var(--pg-light)" stroke="var(--gold)" stroke-width="1.4"/>
        <circle r="4"  fill="var(--gold)"/>
        <rect x="-3" y="-18" width="6" height="7.5" rx="1" fill="var(--gold)" transform="rotate(0)"/>
        <rect x="-3" y="-18" width="6" height="7.5" rx="1" fill="var(--gold)" transform="rotate(45)"/>
        <rect x="-3" y="-18" width="6" height="7.5" rx="1" fill="var(--gold)" transform="rotate(90)"/>
        <rect x="-3" y="-18" width="6" height="7.5" rx="1" fill="var(--gold)" transform="rotate(135)"/>
        <rect x="-3" y="-18" width="6" height="7.5" rx="1" fill="var(--gold)" transform="rotate(180)"/>
        <rect x="-3" y="-18" width="6" height="7.5" rx="1" fill="var(--gold)" transform="rotate(225)"/>
        <rect x="-3" y="-18" width="6" height="7.5" rx="1" fill="var(--gold)" transform="rotate(270)"/>
        <rect x="-3" y="-18" width="6" height="7.5" rx="1" fill="var(--gold)" transform="rotate(315)"/>
      </g>
    </svg>

    <div style="font-size:26pt;font-weight:bold;color:var(--text-h);letter-spacing:3px;text-transform:uppercase">
      Nixie Clock Ultra
    </div>

    <!-- Doppelgoldlinie mit eingebettetem Text -->
    <div style="margin:6mm auto;width:80%;border-top:0.4mm solid var(--gold);border-bottom:0.4mm solid var(--gold);padding:3mm 0">
      <div style="font-size:11pt;color:var(--gold-dk);letter-spacing:7px;text-transform:uppercase">
        Bedienungsanleitung
      </div>
    </div>

    <div style="font-size:10pt;color:var(--hdr-txt);letter-spacing:2px;font-style:italic">
      Modell ESP32-S3 &middot; 6 Nixie-Röhren
    </div>
  </div>

  <!-- Nixie-Röhren-Illustration -->
  <div style="text-align:center;margin:10mm 0">
    <svg width="240" height="70" viewBox="0 0 240 70">
      <defs>
        <radialGradient id="nixieGlow" cx="50%" cy="50%" r="50%">
          <stop offset="0%"   stop-color="#ff9500" stop-opacity="0.55"/>
          <stop offset="100%" stop-color="#ff9500" stop-opacity="0"/>
        </radialGradient>
      </defs>
      <!-- Röhre 1 -->
      <rect x="2"   y="6" width="28" height="52" rx="5" fill="#0d0500" stroke="#b8860b" stroke-width="1.3"/>
      <ellipse cx="16"  cy="32" rx="10" ry="14" fill="url(#nixieGlow)"/>
      <text x="16"  y="40" text-anchor="middle" font-size="22" font-family="Georgia,serif" fill="#ff9500">8</text>
      <!-- Röhre 2 -->
      <rect x="34"  y="6" width="28" height="52" rx="5" fill="#0d0500" stroke="#b8860b" stroke-width="1.3"/>
      <ellipse cx="48"  cy="32" rx="10" ry="14" fill="url(#nixieGlow)"/>
      <text x="48"  y="40" text-anchor="middle" font-size="22" font-family="Georgia,serif" fill="#ff9500">8</text>
      <!-- Doppelpunkte -->
      <circle cx="74"  cy="23" r="3.5" fill="#d4af37" opacity="0.85"/>
      <circle cx="74"  cy="41" r="3.5" fill="#d4af37" opacity="0.85"/>
      <!-- Röhre 3 -->
      <rect x="82"  y="6" width="28" height="52" rx="5" fill="#0d0500" stroke="#b8860b" stroke-width="1.3"/>
      <ellipse cx="96"  cy="32" rx="10" ry="14" fill="url(#nixieGlow)"/>
      <text x="96"  y="40" text-anchor="middle" font-size="22" font-family="Georgia,serif" fill="#ff9500">8</text>
      <!-- Röhre 4 -->
      <rect x="114" y="6" width="28" height="52" rx="5" fill="#0d0500" stroke="#b8860b" stroke-width="1.3"/>
      <ellipse cx="128" cy="32" rx="10" ry="14" fill="url(#nixieGlow)"/>
      <text x="128" y="40" text-anchor="middle" font-size="22" font-family="Georgia,serif" fill="#ff9500">8</text>
      <!-- Doppelpunkte -->
      <circle cx="152" cy="23" r="3.5" fill="#d4af37" opacity="0.85"/>
      <circle cx="152" cy="41" r="3.5" fill="#d4af37" opacity="0.85"/>
      <!-- Röhre 5 -->
      <rect x="160" y="6" width="28" height="52" rx="5" fill="#0d0500" stroke="#b8860b" stroke-width="1.3"/>
      <ellipse cx="174" cy="32" rx="10" ry="14" fill="url(#nixieGlow)"/>
      <text x="174" y="40" text-anchor="middle" font-size="22" font-family="Georgia,serif" fill="#ff9500">8</text>
      <!-- Röhre 6 -->
      <rect x="192" y="6" width="28" height="52" rx="5" fill="#0d0500" stroke="#b8860b" stroke-width="1.3"/>
      <ellipse cx="206" cy="32" rx="10" ry="14" fill="url(#nixieGlow)"/>
      <text x="206" y="40" text-anchor="middle" font-size="22" font-family="Georgia,serif" fill="#ff9500">8</text>
    </svg>
  </div>

  <!-- Ornamentlinie unten -->
  <svg class="ornament-line" width="220" height="14" viewBox="0 0 220 14">
    <line x1="0" y1="7" x2="88" y2="7" stroke="#b8860b" stroke-width="1"/>
    <polygon points="100,7 106,3 112,7 106,11" fill="none" stroke="#b8860b" stroke-width="1"/>
    <line x1="122" y1="7" x2="220" y2="7" stroke="#b8860b" stroke-width="1"/>
  </svg>

  <!-- Versionszeile -->
  <div style="text-align:center;margin-top:5mm">
    <div style="font-size:9pt;color:var(--hdr-txt);letter-spacing:2px">Version 2.0 &middot; Juni 2026</div>
  </div>
</div>
```

- [ ] **Schritt 2.2: Im Browser prüfen**

Datei neu laden. Erwartetes Ergebnis: Titelseite mit Eckverzierungen, Zahnrad, Nixie-Röhren-Illustration und Versionzeile sichtbar.

- [ ] **Schritt 2.3: Commit**

```bash
git add docs/manual/nixie-clock-ultra-bedienungsanleitung.html
git commit -m "feat: Titelseite mit Steampunk-SVG-Ornamente und Nixie-Illustration"
```

---

## Task 3: Inhaltsverzeichnis

**Files:**
- Modify: `docs/manual/nixie-clock-ultra-bedienungsanleitung.html` — `<!-- Task 3 -->` ersetzen

- [ ] **Schritt 3.1: Inhaltsverzeichnis einfügen**

Ersetze `<!-- Task 3 -->` durch:

```html
<div class="page">
  <!-- Eckverzierungen -->
  <svg class="corner-tl" width="36" height="36" viewBox="0 0 36 36">
    <path d="M2,2 L2,16 M2,2 L16,2" stroke="#b8860b" stroke-width="1.4" fill="none"/>
    <circle cx="2" cy="2" r="2" fill="#b8860b"/>
  </svg>
  <svg class="corner-tr" width="36" height="36" viewBox="0 0 36 36">
    <path d="M34,2 L34,16 M34,2 L20,2" stroke="#b8860b" stroke-width="1.4" fill="none"/>
    <circle cx="34" cy="2" r="2" fill="#b8860b"/>
  </svg>
  <svg class="corner-bl" width="36" height="36" viewBox="0 0 36 36">
    <path d="M2,34 L2,20 M2,34 L16,34" stroke="#b8860b" stroke-width="1.4" fill="none"/>
    <circle cx="2" cy="34" r="2" fill="#b8860b"/>
  </svg>
  <svg class="corner-br" width="36" height="36" viewBox="0 0 36 36">
    <path d="M34,34 L34,20 M34,34 L20,34" stroke="#b8860b" stroke-width="1.4" fill="none"/>
    <circle cx="34" cy="34" r="2" fill="#b8860b"/>
  </svg>

  <div style="max-width:140mm;margin:0 auto">
    <!-- Überschrift -->
    <div class="toc-heading">Inhalt</div>
    <svg class="ornament-line" width="160" height="12" viewBox="0 0 160 12">
      <line x1="0" y1="6" x2="62" y2="6" stroke="#b8860b" stroke-width="0.9"/>
      <circle cx="80" cy="6" r="4" fill="none" stroke="#b8860b" stroke-width="0.9"/>
      <line x1="98" y1="6" x2="160" y2="6" stroke="#b8860b" stroke-width="0.9"/>
    </svg>

    <div style="margin-top:6mm">
      <div class="toc-entry">
        <span class="toc-num">I.</span>
        <span class="toc-title">Übersicht &amp; Sicherheit</span>
        <span class="toc-page">3</span>
      </div>
      <div class="toc-entry">
        <span class="toc-num">II.</span>
        <span class="toc-title">Inbetriebnahme</span>
        <span class="toc-page">4</span>
      </div>
      <div class="toc-entry">
        <span class="toc-num">III.</span>
        <span class="toc-title">Tasten &amp; Bedienung</span>
        <span class="toc-page">5</span>
      </div>
      <div class="toc-entry">
        <span class="toc-num">IV.</span>
        <span class="toc-title">Zeit einstellen</span>
        <span class="toc-page">6</span>
      </div>
      <div class="toc-entry">
        <span class="toc-num">V.</span>
        <span class="toc-title">Beleuchtung &amp; Animationen</span>
        <span class="toc-page">7</span>
      </div>
      <div class="toc-entry">
        <span class="toc-num">VI.</span>
        <span class="toc-title">IR-Fernbedienung</span>
        <span class="toc-page">9</span>
      </div>
      <div class="toc-entry">
        <span class="toc-num">VII.</span>
        <span class="toc-title">WLAN &amp; Web-Interface</span>
        <span class="toc-page">11</span>
      </div>
      <div class="toc-entry">
        <span class="toc-num">VIII.</span>
        <span class="toc-title">Einstellungen &amp; Persistenz</span>
        <span class="toc-page">13</span>
      </div>
      <div class="toc-entry">
        <span class="toc-num">IX.</span>
        <span class="toc-title">Technische Daten</span>
        <span class="toc-page">14</span>
      </div>
    </div>

    <div style="margin-top:8mm;text-align:center">
      <svg class="ornament-small" width="80" height="10" viewBox="0 0 80 10">
        <line x1="0" y1="5" x2="28" y2="5" stroke="#b8860b" stroke-width="0.8"/>
        <circle cx="40" cy="5" r="3" fill="none" stroke="#b8860b" stroke-width="0.8"/>
        <line x1="52" y1="5" x2="80" y2="5" stroke="#b8860b" stroke-width="0.8"/>
      </svg>
    </div>
  </div>
</div>
```

- [ ] **Schritt 3.2: Im Browser prüfen**

Erwartetes Ergebnis: Zweite Seite mit gepunktetem Inhaltsverzeichnis, Seitenzahlen rechts.

- [ ] **Schritt 3.3: Commit**

```bash
git add docs/manual/nixie-clock-ultra-bedienungsanleitung.html
git commit -m "feat: Inhaltsverzeichnis mit gepunkteten Leitlinien"
```

---

## Task 4: Kapitel I & II

**Files:**
- Modify: `docs/manual/nixie-clock-ultra-bedienungsanleitung.html` — beide `<!-- Task 4 -->` ersetzen

**Hinweis:** Das Kapitel-Header-Muster (running-header + chapter-title + chapter-rule + Fußornament) wiederholt sich in Tasks 4–9 für jedes Kapitel. Vollständiger Code jeweils unten.

- [ ] **Schritt 4.1: Kapitel I einfügen**

Ersetze erstes `<!-- Task 4 -->` durch:

```html
<div class="page">
  <div class="running-header">
    <span>Nixie Clock Ultra &middot; Bedienungsanleitung</span>
    <span>⚙ 3</span>
  </div>

  <div class="chapter-title">
    <svg width="30" height="30" viewBox="0 0 30 30" style="flex-shrink:0">
      <g transform="translate(15,15)">
        <circle r="5.5" fill="var(--pg-light)" stroke="var(--gold)" stroke-width="1.1"/>
        <circle r="2.2" fill="var(--gold)"/>
        <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(0)"/>
        <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(45)"/>
        <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(90)"/>
        <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(135)"/>
        <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(180)"/>
        <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(225)"/>
        <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(270)"/>
        <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(315)"/>
      </g>
    </svg>
    <div>
      <div class="chapter-label">Kapitel I</div>
      <h1>Übersicht &amp; Sicherheit</h1>
    </div>
  </div>
  <div class="chapter-rule"></div>

  <p>Die Nixie Clock Ultra ist eine Röhrenuhr auf Basis des ESP32-S3 mit 6 Nixie-Röhren, 10 NeoPixel-LEDs, einem Web-Interface sowie einem IR-Empfänger. Die Röhren zeigen die aktuelle Uhrzeit in direkter Ansteuerung ohne Multiplexing — für maximale Helligkeit und ghostingfreie Darstellung.</p>

  <div class="warning-box no-break">
    <svg width="20" height="20" viewBox="0 0 20 20" style="flex-shrink:0;margin-top:2px">
      <polygon points="10,2 18,17 2,17" fill="none" stroke="#8b0000" stroke-width="1.3" stroke-linejoin="round"/>
      <line x1="10" y1="8" x2="10" y2="13" stroke="#8b0000" stroke-width="1.3"/>
      <circle cx="10" cy="15" r="0.9" fill="#8b0000"/>
    </svg>
    <div>
      <div class="warn-title">Sicherheitshinweis – Hochspannung</div>
      <p>Die Nixie-Röhren werden mit <strong>170–180 V DC</strong> betrieben. Diese Spannung kann bei Berührung lebensgefährlich sein. Vor jeglichen Arbeiten an der Schaltung die Versorgungsspannung trennen und die Kondensatoren vollständig entladen. Nie unter Spannung an der Schaltung arbeiten.</p>
    </div>
  </div>

  <h2>Lieferumfang</h2>
  <p>Die Uhr wird betriebsbereit geliefert. Eine Infrarot-Fernbedienung ist nicht im Lieferumfang enthalten — jede handelsübliche IR-Fernbedienung (NEC, Samsung, Sony, RC5 u.v.m.) kann angelernt werden.</p>

  <div style="margin-top:auto;padding-top:6mm;text-align:center">
    <svg class="ornament-small" width="80" height="10" viewBox="0 0 80 10">
      <line x1="0" y1="5" x2="28" y2="5" stroke="#b8860b" stroke-width="0.8"/>
      <circle cx="40" cy="5" r="3" fill="none" stroke="#b8860b" stroke-width="0.8"/>
      <line x1="52" y1="5" x2="80" y2="5" stroke="#b8860b" stroke-width="0.8"/>
    </svg>
  </div>
</div>
```

- [ ] **Schritt 4.2: Kapitel II einfügen**

Ersetze zweites `<!-- Task 4 -->` durch:

```html
<div class="page">
  <div class="running-header">
    <span>Nixie Clock Ultra &middot; Bedienungsanleitung</span>
    <span>⚙ 4</span>
  </div>

  <div class="chapter-title">
    <svg width="30" height="30" viewBox="0 0 30 30" style="flex-shrink:0">
      <g transform="translate(15,15)">
        <circle r="5.5" fill="var(--pg-light)" stroke="var(--gold)" stroke-width="1.1"/>
        <circle r="2.2" fill="var(--gold)"/>
        <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(0)"/>
        <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(45)"/>
        <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(90)"/>
        <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(135)"/>
        <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(180)"/>
        <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(225)"/>
        <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(270)"/>
        <rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(315)"/>
      </g>
    </svg>
    <div>
      <div class="chapter-label">Kapitel II</div>
      <h1>Inbetriebnahme</h1>
    </div>
  </div>
  <div class="chapter-rule"></div>

  <p>Die Uhr ist nach dem Anschließen an die Spannungsversorgung (5 V, Logik + Hochspannungsversorgung für die Nixie-Röhren) sofort betriebsbereit.</p>

  <div class="steps">
    <div class="step">
      <div class="step-num">1</div>
      <p>Spannungsversorgung anschließen. Die Nixie-Röhren starten mit einem sanften <strong>Fade-In</strong> der NeoPixel-Beleuchtung.</p>
    </div>
    <div class="step">
      <div class="step-num">2</div>
      <p>Die Uhr zeigt sofort die in der RTC (DS1302) gespeicherte Uhrzeit an — auch ohne WLAN-Verbindung.</p>
    </div>
    <div class="step">
      <div class="step-num">3</div>
      <p>Alle gespeicherten Einstellungen (Helligkeit, Animation, IR-Codes, WLAN-Zugangsdaten) werden automatisch aus dem Flash-Speicher geladen.</p>
    </div>
    <div class="step">
      <div class="step-num">4</div>
      <p>Das WLAN-Hotspot <strong>NixieClock</strong> (Passwort: <code>nixie1234</code>) ist sofort aktiv und unter <strong>192.168.4.1</strong> erreichbar.</p>
    </div>
  </div>

  <div class="info-box no-break">
    Ist die Uhrzeit nach dem ersten Start falsch, wurde die RTC noch nicht eingestellt. Die Zeit lässt sich per Tastendruck (Kapitel IV) oder bequem über das Web-Interface (Kapitel VII) korrigieren. Sobald die Uhr mit einem Heimnetz verbunden ist, synchronisiert sie die Uhrzeit automatisch per NTP.
  </div>

  <div style="margin-top:auto;padding-top:6mm;text-align:center">
    <svg class="ornament-small" width="80" height="10" viewBox="0 0 80 10">
      <line x1="0" y1="5" x2="28" y2="5" stroke="#b8860b" stroke-width="0.8"/>
      <circle cx="40" cy="5" r="3" fill="none" stroke="#b8860b" stroke-width="0.8"/>
      <line x1="52" y1="5" x2="80" y2="5" stroke="#b8860b" stroke-width="0.8"/>
    </svg>
  </div>
</div>
```

- [ ] **Schritt 4.3: Im Browser prüfen**

Erwartetes Ergebnis: Seiten 3 und 4 mit Kapitelheader, Warnbox (Kap. I) und nummerierten Schritten (Kap. II).

- [ ] **Schritt 4.4: Commit**

```bash
git add docs/manual/nixie-clock-ultra-bedienungsanleitung.html
git commit -m "feat: Kapitel I Übersicht & Sicherheit, Kapitel II Inbetriebnahme"
```

---

## Task 5: Kapitel III & IV

**Files:**
- Modify: `docs/manual/nixie-clock-ultra-bedienungsanleitung.html` — beide `<!-- Task 5 -->` ersetzen

- [ ] **Schritt 5.1: Kapitel III einfügen**

Ersetze erstes `<!-- Task 5 -->` durch:

```html
<div class="page">
  <div class="running-header">
    <span>Nixie Clock Ultra &middot; Bedienungsanleitung</span>
    <span>⚙ 5</span>
  </div>
  <div class="chapter-title">
    <svg width="30" height="30" viewBox="0 0 30 30" style="flex-shrink:0"><g transform="translate(15,15)"><circle r="5.5" fill="var(--pg-light)" stroke="var(--gold)" stroke-width="1.1"/><circle r="2.2" fill="var(--gold)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(0)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(45)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(90)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(135)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(180)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(225)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(270)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(315)"/></g></svg>
    <div>
      <div class="chapter-label">Kapitel III</div>
      <h1>Tasten &amp; Bedienung</h1>
    </div>
  </div>
  <div class="chapter-rule"></div>

  <p>Das Gehäuse beherbergt vier Drucktaster, die alle wesentlichen Einstellungen direkt am Gerät ermöglichen. Jede Taste reagiert auf <strong>kurzen</strong> und — wo angegeben — auf <strong>langen Druck</strong> (≥ 600 ms gehalten).</p>

  <table class="data-table no-break">
    <thead>
      <tr>
        <th>Taste</th>
        <th>Kurzer Druck</th>
        <th>Langer Druck</th>
      </tr>
    </thead>
    <tbody>
      <tr>
        <td><strong>SET</strong></td>
        <td>Einstellmodus starten · nächste Stelle · Zeit speichern</td>
        <td><em>—</em></td>
      </tr>
      <tr>
        <td><strong>UP</strong></td>
        <td>Wert erhöhen</td>
        <td>Wert erhöhen (Auto-Repeat)</td>
      </tr>
      <tr>
        <td><strong>DOWN</strong></td>
        <td>Wert verringern</td>
        <td>Wert verringern (Auto-Repeat)</td>
      </tr>
      <tr>
        <td><strong>LIGHT</strong></td>
        <td>Helligkeit weiterschalten (4 Stufen)</td>
        <td>Trennpunkt-LEDs dauerhaft an/aus</td>
      </tr>
    </tbody>
  </table>

  <p>Alle Tasteneinstellungen wirken sofort und werden dauerhaft gespeichert. Dieselben Funktionen stehen alternativ über die IR-Fernbedienung (Kapitel VI) und das Web-Interface (Kapitel VII) zur Verfügung.</p>

  <div style="margin-top:auto;padding-top:6mm;text-align:center">
    <svg class="ornament-small" width="80" height="10" viewBox="0 0 80 10"><line x1="0" y1="5" x2="28" y2="5" stroke="#b8860b" stroke-width="0.8"/><circle cx="40" cy="5" r="3" fill="none" stroke="#b8860b" stroke-width="0.8"/><line x1="52" y1="5" x2="80" y2="5" stroke="#b8860b" stroke-width="0.8"/></svg>
  </div>
</div>
```

- [ ] **Schritt 5.2: Kapitel IV einfügen**

Ersetze zweites `<!-- Task 5 -->` durch:

```html
<div class="page">
  <div class="running-header">
    <span>Nixie Clock Ultra &middot; Bedienungsanleitung</span>
    <span>⚙ 6</span>
  </div>
  <div class="chapter-title">
    <svg width="30" height="30" viewBox="0 0 30 30" style="flex-shrink:0"><g transform="translate(15,15)"><circle r="5.5" fill="var(--pg-light)" stroke="var(--gold)" stroke-width="1.1"/><circle r="2.2" fill="var(--gold)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(0)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(45)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(90)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(135)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(180)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(225)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(270)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(315)"/></g></svg>
    <div>
      <div class="chapter-label">Kapitel IV</div>
      <h1>Zeit einstellen</h1>
    </div>
  </div>
  <div class="chapter-rule"></div>

  <p>Die Uhrzeit lässt sich direkt am Gerät per Tastendruck einstellen. Die blinkende Stelle zeigt an, welcher Wert gerade bearbeitet wird.</p>

  <div class="steps">
    <div class="step"><div class="step-num">1</div><p><strong>SET</strong> drücken → Stunden blinken auf.</p></div>
    <div class="step"><div class="step-num">2</div><p>Mit <strong>UP</strong> / <strong>DOWN</strong> die Stunden einstellen (0–23). Langer Druck aktiviert Auto-Repeat.</p></div>
    <div class="step"><div class="step-num">3</div><p><strong>SET</strong> drücken → Minuten blinken auf.</p></div>
    <div class="step"><div class="step-num">4</div><p>Mit <strong>UP</strong> / <strong>DOWN</strong> die Minuten einstellen (0–59).</p></div>
    <div class="step"><div class="step-num">5</div><p><strong>SET</strong> drücken → Sekunden blinken auf.</p></div>
    <div class="step"><div class="step-num">6</div><p>Mit <strong>UP</strong> / <strong>DOWN</strong> die Sekunden einstellen (0–59).</p></div>
    <div class="step"><div class="step-num">7</div><p><strong>SET</strong> drücken → Zeit wird in der RTC gespeichert, Normalanzeige kehrt zurück.</p></div>
  </div>

  <div class="info-box no-break">
    <strong>Timeout:</strong> Erfolgt <strong>15 Sekunden</strong> lang keine Eingabe, wird der Einstellmodus automatisch beendet und die bis dahin eingestellten Werte werden gespeichert.
  </div>

  <div style="margin-top:auto;padding-top:6mm;text-align:center">
    <svg class="ornament-small" width="80" height="10" viewBox="0 0 80 10"><line x1="0" y1="5" x2="28" y2="5" stroke="#b8860b" stroke-width="0.8"/><circle cx="40" cy="5" r="3" fill="none" stroke="#b8860b" stroke-width="0.8"/><line x1="52" y1="5" x2="80" y2="5" stroke="#b8860b" stroke-width="0.8"/></svg>
  </div>
</div>
```

- [ ] **Schritt 5.3: Im Browser prüfen**

Erwartetes Ergebnis: Seiten 5–6. Kap. III zeigt goldene Tabelle mit 4 Zeilen, Kap. IV zeigt nummerierte Schritte 1–7 und Info-Box.

- [ ] **Schritt 5.4: Commit**

```bash
git add docs/manual/nixie-clock-ultra-bedienungsanleitung.html
git commit -m "feat: Kapitel III Tasten & Bedienung, Kapitel IV Zeit einstellen"
```

---

## Task 6: Kapitel V – Beleuchtung & Animationen

**Files:**
- Modify: `docs/manual/nixie-clock-ultra-bedienungsanleitung.html` — `<!-- Task 6 -->` ersetzen

- [ ] **Schritt 6.1: Kapitel V einfügen**

Ersetze `<!-- Task 6 -->` durch:

```html
<div class="page">
  <div class="running-header">
    <span>Nixie Clock Ultra &middot; Bedienungsanleitung</span>
    <span>⚙ 7</span>
  </div>
  <div class="chapter-title">
    <svg width="30" height="30" viewBox="0 0 30 30" style="flex-shrink:0"><g transform="translate(15,15)"><circle r="5.5" fill="var(--pg-light)" stroke="var(--gold)" stroke-width="1.1"/><circle r="2.2" fill="var(--gold)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(0)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(45)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(90)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(135)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(180)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(225)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(270)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(315)"/></g></svg>
    <div>
      <div class="chapter-label">Kapitel V</div>
      <h1>Beleuchtung &amp; Animationen</h1>
    </div>
  </div>
  <div class="chapter-rule"></div>

  <h2>NeoPixel-Helligkeit</h2>
  <p>Ein kurzer Druck auf <strong>LIGHT</strong> schaltet die Hintergrundbeleuchtung durch vier Helligkeitsstufen. Die Einstellung wird dauerhaft gespeichert.</p>

  <table class="data-table no-break">
    <thead><tr><th>Stufe</th><th>Beschreibung</th></tr></thead>
    <tbody>
      <tr><td><strong>1</strong></td><td>Sehr dim – minimale Hintergrundbeleuchtung</td></tr>
      <tr><td><strong>2</strong></td><td>Dim – dezente Beleuchtung</td></tr>
      <tr><td><strong>3</strong></td><td>Hell – angenehme Raumbeleuchtung</td></tr>
      <tr><td><strong>4</strong></td><td>Voll (Standard) – maximale Helligkeit</td></tr>
    </tbody>
  </table>

  <p>Über das Web-Interface ist zusätzlich ein Feinstufenregler (Wert 10–255) verfügbar.</p>

  <h2>Animationsmodi</h2>
  <table class="data-table no-break">
    <thead><tr><th>Modus</th><th>Beschreibung</th></tr></thead>
    <tbody>
      <tr><td><strong>Rainbow</strong></td><td>Farbverlauf wandert kontinuierlich über die 6 Hintergrund-LEDs</td></tr>
      <tr><td><strong>Statisch Warmweiß</strong></td><td>Alle Hintergrund-LEDs leuchten in warmweißem Orange</td></tr>
      <tr><td><strong>Puls</strong></td><td>Sinusförmiges Auf- und Abdimmen der gesamten Beleuchtung</td></tr>
      <tr><td><strong>Slot-Machine</strong></td><td>Schneller Farbwechsel während der Slot-Animation; sonst Rainbow. Alle 10 Sekunden automatische Slot-Animation.</td></tr>
    </tbody>
  </table>

  <h2>Trennpunkt-LEDs</h2>
  <p>Die vier LEDs zwischen den Zifferngruppen blinken standardmäßig sekundengenau. Ein <strong>langer Druck auf LIGHT</strong> schaltet sie in den Dauerleucht-Modus:</p>

  <table class="data-table no-break">
    <thead><tr><th>Modus</th><th>Beschreibung</th></tr></thead>
    <tbody>
      <tr><td><strong>Blinken</strong> (Standard)</td><td>LEDs blinken sekundengenau im Takt der Uhr</td></tr>
      <tr><td><strong>Dauerhaft an</strong></td><td>LEDs leuchten durchgehend ohne zu blinken</td></tr>
    </tbody>
  </table>

  <div style="margin-top:auto;padding-top:6mm;text-align:center">
    <svg class="ornament-small" width="80" height="10" viewBox="0 0 80 10"><line x1="0" y1="5" x2="28" y2="5" stroke="#b8860b" stroke-width="0.8"/><circle cx="40" cy="5" r="3" fill="none" stroke="#b8860b" stroke-width="0.8"/><line x1="52" y1="5" x2="80" y2="5" stroke="#b8860b" stroke-width="0.8"/></svg>
  </div>
</div>
```

- [ ] **Schritt 6.2: Im Browser prüfen**

Erwartetes Ergebnis: Seite 7 mit drei Tabellen (Helligkeit, Animationen, Trennpunkte).

- [ ] **Schritt 6.3: Commit**

```bash
git add docs/manual/nixie-clock-ultra-bedienungsanleitung.html
git commit -m "feat: Kapitel V Beleuchtung und Animationen"
```

---

## Task 7: Kapitel VI – IR-Fernbedienung

**Files:**
- Modify: `docs/manual/nixie-clock-ultra-bedienungsanleitung.html` — `<!-- Task 7 -->` ersetzen

- [ ] **Schritt 7.1: Kapitel VI einfügen**

Ersetze `<!-- Task 7 -->` durch:

```html
<div class="page">
  <div class="running-header">
    <span>Nixie Clock Ultra &middot; Bedienungsanleitung</span>
    <span>⚙ 9</span>
  </div>
  <div class="chapter-title">
    <svg width="30" height="30" viewBox="0 0 30 30" style="flex-shrink:0"><g transform="translate(15,15)"><circle r="5.5" fill="var(--pg-light)" stroke="var(--gold)" stroke-width="1.1"/><circle r="2.2" fill="var(--gold)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(0)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(45)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(90)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(135)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(180)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(225)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(270)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(315)"/></g></svg>
    <div>
      <div class="chapter-label">Kapitel VI</div>
      <h1>IR-Fernbedienung</h1>
    </div>
  </div>
  <div class="chapter-rule"></div>

  <p>Die Uhr lässt sich mit jeder handelsüblichen Infrarot-Fernbedienung steuern (NEC, Samsung, Sony, RC5 u.v.m.). Jede Taste kann frei einer der sieben Funktionen zugewiesen werden.</p>

  <h2>Verfügbare Funktionen</h2>
  <table class="data-table no-break">
    <thead><tr><th>Funktion</th><th>Beschreibung</th></tr></thead>
    <tbody>
      <tr><td><strong>SET</strong></td><td>Einstellmodus starten / nächste Stelle / Zeit speichern</td></tr>
      <tr><td><strong>UP</strong></td><td>Wert erhöhen im Einstellmodus</td></tr>
      <tr><td><strong>DOWN</strong></td><td>Wert verringern im Einstellmodus</td></tr>
      <tr><td><strong>BRIGHTNESS</strong></td><td>Helligkeit weiterschalten (4 Stufen)</td></tr>
      <tr><td><strong>ANIM_NEXT</strong></td><td>Nächsten Animationsmodus aktivieren</td></tr>
      <tr><td><strong>SLOT</strong></td><td>Slot-Machine-Animation auslösen</td></tr>
      <tr><td><strong>COLON_TOGGLE</strong></td><td>Trennpunkt-LEDs dauerhaft an/aus</td></tr>
    </tbody>
  </table>

  <h2>Fernbedienung anlernen</h2>
  <p>Das Anlernen erfolgt über das Web-Interface unter <strong>IR-Fernbedienung</strong>:</p>

  <div class="steps">
    <div class="step"><div class="step-num">1</div><p>Auf <strong>Anlernen</strong> neben der gewünschten Funktion klicken.</p></div>
    <div class="step"><div class="step-num">2</div><p>Die Zeile blinkt orange und zeigt <em>»Taste auf Fernbedienung drücken…«</em></p></div>
    <div class="step"><div class="step-num">3</div><p>Beliebige Taste auf der Fernbedienung drücken → Code wird sofort gespeichert.</p></div>
  </div>

  <div class="info-box no-break">
    Nach <strong>10 Sekunden</strong> ohne Eingabe bricht der Anlernmodus automatisch ab. Ein bestehender Code kann durch Klick auf <strong>✕</strong> gelöscht werden. Repeat-Codes (automatische Wiederholung der Fernbedienung) werden herausgefiltert.
  </div>

  <div style="margin-top:auto;padding-top:6mm;text-align:center">
    <svg class="ornament-small" width="80" height="10" viewBox="0 0 80 10"><line x1="0" y1="5" x2="28" y2="5" stroke="#b8860b" stroke-width="0.8"/><circle cx="40" cy="5" r="3" fill="none" stroke="#b8860b" stroke-width="0.8"/><line x1="52" y1="5" x2="80" y2="5" stroke="#b8860b" stroke-width="0.8"/></svg>
  </div>
</div>
```

- [ ] **Schritt 7.2: Im Browser prüfen**

Erwartetes Ergebnis: Seite 9 mit Funktionstabelle (7 Zeilen), Anlernschritte und Info-Box.

- [ ] **Schritt 7.3: Commit**

```bash
git add docs/manual/nixie-clock-ultra-bedienungsanleitung.html
git commit -m "feat: Kapitel VI IR-Fernbedienung"
```

---

## Task 8: Kapitel VII – WLAN & Web-Interface

**Files:**
- Modify: `docs/manual/nixie-clock-ultra-bedienungsanleitung.html` — `<!-- Task 8 -->` ersetzen

- [ ] **Schritt 8.1: Kapitel VII einfügen**

Ersetze `<!-- Task 8 -->` durch:

```html
<div class="page">
  <div class="running-header">
    <span>Nixie Clock Ultra &middot; Bedienungsanleitung</span>
    <span>⚙ 11</span>
  </div>
  <div class="chapter-title">
    <svg width="30" height="30" viewBox="0 0 30 30" style="flex-shrink:0"><g transform="translate(15,15)"><circle r="5.5" fill="var(--pg-light)" stroke="var(--gold)" stroke-width="1.1"/><circle r="2.2" fill="var(--gold)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(0)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(45)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(90)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(135)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(180)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(225)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(270)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(315)"/></g></svg>
    <div>
      <div class="chapter-label">Kapitel VII</div>
      <h1>WLAN &amp; Web-Interface</h1>
    </div>
  </div>
  <div class="chapter-rule"></div>

  <h2>Verbindung herstellen</h2>
  <p>Die Uhr startet immer als WLAN-Access-Point. Das Web-Interface ist ohne weitere Konfiguration sofort erreichbar:</p>

  <table class="conn-table no-break">
    <tbody>
      <tr><td>SSID</td><td>NixieClock</td></tr>
      <tr><td>Passwort</td><td>nixie1234</td></tr>
      <tr><td>IP-Adresse</td><td>192.168.4.1</td></tr>
    </tbody>
  </table>

  <div class="steps">
    <div class="step"><div class="step-num">1</div><p>Mit dem WLAN <strong>NixieClock</strong> verbinden.</p></div>
    <div class="step"><div class="step-num">2</div><p>Browser öffnen und <strong>http://192.168.4.1</strong> aufrufen.</p></div>
  </div>

  <h2>Heimnetz einrichten</h2>
  <p>Im Web-Interface unter <strong>WLAN-Einrichtung</strong>:</p>

  <div class="steps">
    <div class="step"><div class="step-num">1</div><p>Auf <strong>Scannen</strong> klicken → verfügbare Netzwerke erscheinen.</p></div>
    <div class="step"><div class="step-num">2</div><p>Gewünschtes Netzwerk auswählen und Passwort eingeben.</p></div>
    <div class="step"><div class="step-num">3</div><p><strong>Verbinden</strong> klicken → Uhr startet automatisch neu.</p></div>
  </div>

  <p>Nach dem Neustart ist die Uhr über den Hotspot (<code>192.168.4.1</code>) <em>und</em> über das Heimnetz erreichbar. Die Heimnetz-IP wird im Web-Interface angezeigt. Heimnetz entfernen: SSID-Feld leer lassen und <strong>Verbinden</strong> klicken.</p>

  <h2>NTP-Zeitsynchronisierung</h2>
  <p>Sobald die Uhr mit einem Heimnetz verbunden ist, synchronisiert sie die Uhrzeit <strong>einmalig</strong> automatisch über NTP (<code>pool.ntp.org</code>). Die Zeitzone ist auf <strong>Mitteleuropa (CET/CEST)</strong> voreingestellt. Der NTP-Status wird im Web-Interface angezeigt.</p>

  <h2>Web-Interface Funktionen</h2>
  <table class="data-table no-break">
    <thead><tr><th>Funktion</th><th>Beschreibung</th></tr></thead>
    <tbody>
      <tr><td><strong>Zeit stellen</strong></td><td>Stunden, Minuten, Sekunden manuell eingeben oder Browser-Zeit übernehmen</td></tr>
      <tr><td><strong>Helligkeit (Stufen)</strong></td><td>4 Stufen — entspricht kurzem Druck auf LIGHT</td></tr>
      <tr><td><strong>Helligkeit (Fein)</strong></td><td>Schieberegler 10–255 für stufenlose Einstellung</td></tr>
      <tr><td><strong>Animation</strong></td><td>Animationsmodus aus Dropdown wählen</td></tr>
      <tr><td><strong>Farbe (Hue)</strong></td><td>Grundfarbe für statische Modi und Puls-Animation</td></tr>
      <tr><td><strong>Trennpunkte</strong></td><td>Blinken oder dauerhaft an</td></tr>
      <tr><td><strong>Slot-Machine</strong></td><td>Animation manuell auslösen</td></tr>
      <tr><td><strong>IR-Fernbedienung</strong></td><td>Codes anlernen und löschen (siehe Kapitel VI)</td></tr>
      <tr><td><strong>WLAN-Einrichtung</strong></td><td>Heimnetz verbinden oder trennen</td></tr>
    </tbody>
  </table>

  <div style="margin-top:auto;padding-top:6mm;text-align:center">
    <svg class="ornament-small" width="80" height="10" viewBox="0 0 80 10"><line x1="0" y1="5" x2="28" y2="5" stroke="#b8860b" stroke-width="0.8"/><circle cx="40" cy="5" r="3" fill="none" stroke="#b8860b" stroke-width="0.8"/><line x1="52" y1="5" x2="80" y2="5" stroke="#b8860b" stroke-width="0.8"/></svg>
  </div>
</div>
```

- [ ] **Schritt 8.2: Im Browser prüfen**

Erwartetes Ergebnis: Seite 11 mit Verbindungstabelle, Heimnetz-Schritte, NTP-Info und Web-UI-Funktionstabelle (9 Zeilen).

- [ ] **Schritt 8.3: Commit**

```bash
git add docs/manual/nixie-clock-ultra-bedienungsanleitung.html
git commit -m "feat: Kapitel VII WLAN und Web-Interface"
```

---

## Task 9: Kapitel VIII & IX

**Files:**
- Modify: `docs/manual/nixie-clock-ultra-bedienungsanleitung.html` — beide `<!-- Task 9 -->` ersetzen

- [ ] **Schritt 9.1: Kapitel VIII einfügen**

Ersetze erstes `<!-- Task 9 -->` durch:

```html
<div class="page">
  <div class="running-header">
    <span>Nixie Clock Ultra &middot; Bedienungsanleitung</span>
    <span>⚙ 13</span>
  </div>
  <div class="chapter-title">
    <svg width="30" height="30" viewBox="0 0 30 30" style="flex-shrink:0"><g transform="translate(15,15)"><circle r="5.5" fill="var(--pg-light)" stroke="var(--gold)" stroke-width="1.1"/><circle r="2.2" fill="var(--gold)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(0)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(45)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(90)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(135)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(180)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(225)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(270)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(315)"/></g></svg>
    <div>
      <div class="chapter-label">Kapitel VIII</div>
      <h1>Einstellungen &amp; Persistenz</h1>
    </div>
  </div>
  <div class="chapter-rule"></div>

  <p>Folgende Einstellungen werden dauerhaft im Flash-Speicher des ESP32-S3 gespeichert und nach jedem Neustart automatisch wiederhergestellt:</p>

  <table class="data-table no-break">
    <thead><tr><th>Einstellung</th><th>Gespeicherter Wert</th></tr></thead>
    <tbody>
      <tr><td><strong>NeoPixel-Helligkeit</strong></td><td>Stufe (1–4) und Feinwert (10–255)</td></tr>
      <tr><td><strong>Animationsmodus</strong></td><td>Rainbow / Statisch / Puls / Slot-Machine</td></tr>
      <tr><td><strong>Trennpunkt-LEDs</strong></td><td>Blinken oder dauerhaft an</td></tr>
      <tr><td><strong>WLAN-Zugangsdaten</strong></td><td>SSID und Passwort des Heimnetzes</td></tr>
      <tr><td><strong>IR-Codes</strong></td><td>Alle 7 angelernten Funktionen</td></tr>
    </tbody>
  </table>

  <div class="info-box no-break">
    Die Uhrzeit wird in der batteriegepufferten RTC (DS1302) gespeichert und bleibt auch bei Stromausfall erhalten — vorausgesetzt, die RTC-Batterie ist geladen.
  </div>

  <div style="margin-top:auto;padding-top:6mm;text-align:center">
    <svg class="ornament-small" width="80" height="10" viewBox="0 0 80 10"><line x1="0" y1="5" x2="28" y2="5" stroke="#b8860b" stroke-width="0.8"/><circle cx="40" cy="5" r="3" fill="none" stroke="#b8860b" stroke-width="0.8"/><line x1="52" y1="5" x2="80" y2="5" stroke="#b8860b" stroke-width="0.8"/></svg>
  </div>
</div>
```

- [ ] **Schritt 9.2: Kapitel IX einfügen**

Ersetze zweites `<!-- Task 9 -->` durch:

```html
<div class="page">
  <div class="running-header">
    <span>Nixie Clock Ultra &middot; Bedienungsanleitung</span>
    <span>⚙ 14</span>
  </div>
  <div class="chapter-title">
    <svg width="30" height="30" viewBox="0 0 30 30" style="flex-shrink:0"><g transform="translate(15,15)"><circle r="5.5" fill="var(--pg-light)" stroke="var(--gold)" stroke-width="1.1"/><circle r="2.2" fill="var(--gold)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(0)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(45)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(90)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(135)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(180)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(225)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(270)"/><rect x="-1.8" y="-10" width="3.6" height="4.5" rx="0.6" fill="var(--gold)" transform="rotate(315)"/></g></svg>
    <div>
      <div class="chapter-label">Kapitel IX</div>
      <h1>Technische Daten</h1>
    </div>
  </div>
  <div class="chapter-rule"></div>

  <table class="data-table no-break">
    <thead><tr><th>Eigenschaft</th><th>Wert</th></tr></thead>
    <tbody>
      <tr><td><strong>Mikrocontroller</strong></td><td>ESP32-S3</td></tr>
      <tr><td><strong>Nixie-Ansteuerung</strong></td><td>4× MCP23017 I²C Port-Expander (0x20–0x23), direkte Ansteuerung ohne Multiplexing</td></tr>
      <tr><td><strong>Nixie-Röhren</strong></td><td>6 Stück</td></tr>
      <tr><td><strong>Nixie-Hochspannung</strong></td><td>170–180 V DC</td></tr>
      <tr><td><strong>Echtzeituhr</strong></td><td>DS1302 (ThreeWire-Interface, batteriegepuffert)</td></tr>
      <tr><td><strong>NeoPixel</strong></td><td>10× WS2812B (6 Hintergrund + 4 Trennpunkte), Pin GPIO21</td></tr>
      <tr><td><strong>IR-Empfänger</strong></td><td>VS1838 (38 kHz), Pin GPIO48</td></tr>
      <tr><td><strong>I²C-Bus</strong></td><td>SDA = GPIO8, SCL = GPIO9</td></tr>
      <tr><td><strong>Taster</strong></td><td>SET = GPIO13, UP = GPIO14, DOWN = GPIO15, LIGHT = GPIO16</td></tr>
      <tr><td><strong>Versorgungsspannung</strong></td><td>5 V (Logik) + HV-Netzteil für Nixies</td></tr>
      <tr><td><strong>Web-Interface</strong></td><td>HTTP, Port 80, AP + STA Dual-Mode</td></tr>
      <tr><td><strong>NTP-Zeitzone</strong></td><td>CET-1CEST (Mitteleuropa, automatische Sommerzeit)</td></tr>
    </tbody>
  </table>

  <!-- Abschlussornament -->
  <div style="text-align:center;margin-top:10mm">
    <svg width="220" height="14" viewBox="0 0 220 14">
      <line x1="0" y1="7" x2="88" y2="7" stroke="#b8860b" stroke-width="1"/>
      <polygon points="100,7 106,3 112,7 106,11" fill="none" stroke="#b8860b" stroke-width="1"/>
      <line x1="122" y1="7" x2="220" y2="7" stroke="#b8860b" stroke-width="1"/>
    </svg>
    <div style="font-size:9pt;color:var(--hdr-txt);margin-top:4mm;letter-spacing:2px;font-style:italic">
      Nixie Clock Ultra &middot; Version 2.0 &middot; Juni 2026
    </div>
  </div>
</div>
```

- [ ] **Schritt 9.3: Im Browser prüfen**

Erwartetes Ergebnis: Seite 13 (Persistenz-Tabelle + Info-Box), Seite 14 (Technische Daten, 12 Zeilen, großes Abschluss-Ornament).

- [ ] **Schritt 9.4: Commit**

```bash
git add docs/manual/nixie-clock-ultra-bedienungsanleitung.html
git commit -m "feat: Kapitel VIII Persistenz, Kapitel IX Technische Daten"
```

---

## Task 10: Druckoptimierung & Abnahme

**Files:**
- Modify: `docs/manual/nixie-clock-ultra-bedienungsanleitung.html` — CSS `@media print` ergänzen

- [ ] **Schritt 10.1: Druckvorschau testen**

Datei in Chrome öffnen → `Strg+P`:
- Ziel: **Als PDF speichern**
- Papierformat: A4
- Ränder: **Keine** (Ränder kommen aus CSS)
- Hintergrundgrafiken: **aktivieren** ← wichtig, sonst fehlen Pergament-Farben und SVG-Dekos

Erwartetes Ergebnis: 14 Seiten, Titelseite ohne laufenden Kopf, alle Kapitel starten auf neuer Seite, keine Tabellen oder Warnboxen werden mittendurch getrennt.

- [ ] **Schritt 10.2: Falls Seitenumbrüche schlecht fallen — CSS anpassen**

Füge im `@media print`-Block gezielte Ergänzungen hinzu wenn nötig. Beispiel: Kapitel V fällt auf zwei Seiten → füge `page-break-before: always` auf dem jeweiligen `h2` ein:

```css
@media print {
  /* Kapitel-spezifische Umbrüche bei Bedarf: */
  .page h2.new-page { page-break-before: always; }
}
```

Und im HTML z.B.:
```html
<h2 class="new-page">Animationsmodi</h2>
```

- [ ] **Schritt 10.3: PDF generieren und prüfen**

`Strg+P` → Als PDF speichern → `nixie-clock-ultra-bedienungsanleitung.pdf`  
Öffne das PDF und prüfe:
- Titelseite korrekt zentriert
- Alle Ornament-SVGs sichtbar
- Nixie-Röhren mit orange Schimmer
- Alle Tabellen vollständig (kein Abschnitt am Seitenende)
- Seitenzahlen in Kopfzeilen korrekt

- [ ] **Schritt 10.4: Abschluss-Commit**

```bash
git add docs/manual/nixie-clock-ultra-bedienungsanleitung.html
git commit -m "feat: Steampunk Bedienungsanleitung vollständig – druckfertig als PDF"
```
