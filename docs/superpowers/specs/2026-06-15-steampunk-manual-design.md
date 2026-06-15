# Design-Spec: Steampunk Bedienungsanleitung – Nixie Clock Ultra

**Datum:** 2026-06-15  
**Status:** Freigegeben  

---

## Ziel

Erstellung einer hochwertigen Bedienungsanleitung für die Nixie Clock Ultra im Steampunk-Stil, als druckfertiges PDF.

---

## Format & Erzeugung

- **Ausgabeformat:** PDF, A4 Hochformat
- **Erzeugung:** Einzelne selbst-enthaltene HTML-Datei mit `@media print` CSS → User öffnet in Chrome/Firefox → `Strg+P` → „Als PDF speichern"
- **Kein externes Tool** (kein Pandoc, kein LaTeX, kein WeasyPrint) erforderlich
- Die HTML-Datei verbleibt im Repo und dient gleichzeitig als interaktive Vorschau

---

## Visueller Stil: Elegant & Zeitlos

### Farbpalette

| Element              | Farbe        | Hex       |
|----------------------|--------------|-----------|
| Seitenhintergrund    | Pergament    | `#f7f2e8` / `#f0ebe0` |
| Überschriften        | Dunkelbraun  | `#2c1810` |
| Fließtext            | Dunkelbraun  | `#3d2b00` |
| Gold-Akzente         | Messing-Gold | `#b8860b` |
| Gold-Ornamente       | Dunkelgold   | `#8b6914` |
| Tabellenkopf         | Dunkelgold   | `#8b6914` (Hintergrund), `#f7f2e8` (Text) |
| Warnbox-Rand         | Dunkelrot    | `#8b0000` |
| Laufkopf / Fußnoten  | Goldbraun    | `#a08040` |

### Schriften

- **Überschriften & Titel:** Georgia, serif
- **Fließtext:** Georgia, serif (11 pt im Druck)
- Keine externen Web-Fonts (selbst-enthalten)

### Steampunk-Dekorationselemente (alle als inline SVG)

1. **Eckverzierungen** auf Titel- und Kapitelseiten: L-förmige Goldlinien mit Punkt
2. **Ornamentlinie** als Trenner: Horizontale Linie mit Raute/Pfeil-Mittelpunkt
3. **Zahnrad-Icon** am Kapitelheader (8-zähniges SVG-Zahnrad, ~28×28px)
4. **Nixie-Röhren-Illustration** auf der Titelseite: 6 stilisierte Röhren mit orange leuchtendem Schimmer (SVG, Radial-Gradient)
5. **Laufende Kopfzeile** auf jeder Innenseite: Dokumenttitel links, ⚙ + Seitenzahl rechts, getrennt durch dünne Goldlinie

---

## Seitenaufbau

### Titelseite (Seite 1)

- Pergament-Hintergrund mit leichtem Gradient
- 4 Eckverzierungen (SVG)
- Ornamentlinie oben
- Zentrierter Titelblock:
  - SVG-Zahnrad-Ornament
  - Haupttitel: „Nixie Clock Ultra" (22 pt, bold, letter-spacing)
  - Doppelte Goldlinie mit eingebettetem Text: „Bedienungsanleitung"
  - Untertitel: „Modell ESP32-S3 · 6 Nixie-Röhren"
- Nixie-Röhren-Illustration (6 Röhren mit Leuchtschimmer, Doppelpunkt-LEDs)
- Ornamentlinie unten
- Versionszeile: „Version 2.0 · Juni 2026"

### Inhaltsverzeichnis (Seite 2)

- Gleicher Pergament-Hintergrund, Eckverzierungen
- Zentrierte Überschrift „Inhalt" mit Ornamentlinie darunter
- 9 Einträge mit gepunkteten Leitlinien und rechtsbündigen Seitenzahlen
- Abschluss-Ornament

### Kapitelseiten (Seiten 3–14)

- **Laufende Kopfzeile:** „Nixie Clock Ultra · Bedienungsanleitung" | ⚙ [Seitenzahl]
- **Kapitelheader:** Kleines SVG-Zahnrad + „Kapitel [Römisch]" (klein, gold) + Kapiteltitel (groß)
- **Goldene Trennlinie** unter dem Header (Gradient: gold → transparent)
- **Fließtext:** Georgia 10.5 pt, Zeilenabstand 1.7, Dunkelbraun
- **Fußlinie:** Ornament-Linie (Linie–Kreis–Linie)

---

## Kapitelstruktur

| Nr.  | Titel                        | Inhalt                                                              |
|------|------------------------------|---------------------------------------------------------------------|
| I    | Übersicht & Sicherheit       | Kurzbeschreibung, Hochspannungswarnung (Warnbox), Lieferumfang      |
| II   | Inbetriebnahme               | Erststart, Fade-In, RTC-Anzeige, Grundzustand                       |
| III  | Tasten & Bedienung           | SET/UP/DOWN/LIGHT, Kurz- und Langdruck, Tabelle                     |
| IV   | Zeit einstellen              | Schritt-für-Schritt per Taste, 15-s-Timeout, nummerierte Schritte  |
| V    | Beleuchtung & Animationen    | 4 Helligkeitsstufen, Animationsmodi, Trennpunkt-LEDs, Farbwahl     |
| VI   | IR-Fernbedienung             | Funktionsübersicht (Tabelle), Anlernen, Löschen                     |
| VII  | WLAN & Web-Interface         | AP-Modus, Heimnetz, NTP-Sync, alle Web-UI-Funktionen               |
| VIII | Einstellungen & Persistenz   | Was im Flash gespeichert wird                                       |
| IX   | Technische Daten             | ESP32-S3, MCP23017, DS1302, WS2812B, Spannungen, Pins              |

---

## Wiederkehrende Inhaltselemente

### Tabellen
- Kopfzeile: Hintergrund `#8b6914`, Text `#f7f2e8`, kein Fettdruck
- Zeilen alternierend: `#f7f2e8` / `#f0ebe0`
- Trennlinien: `#d3c9a8`

### Warnbox (Hochspannungshinweis)
- Linker Rand: 4px solid `#8b0000`
- Umrandung: 1px solid `#b8860b`
- Hintergrund: `#fff8f0`
- SVG-Warndreieck, Titel in Kapitälchen, roter Text

### Nummerierte Schritte
- Goldene Kreise (∅ 16px, Hintergrund `#8b6914`, Text `#f7f2e8`) als Schritt-Nummer
- Fließtext rechts daneben

### Ornamentlinie (Trenner)
```
────────── ◇ ──────────
```
Als SVG: Linie links, Raute/Pfeil mittig, Linie rechts

---

## Ausgabedatei

`docs/manual/nixie-clock-ultra-bedienungsanleitung.html`

Zum Druck: Chrome → `Strg+P` → Ziel: „Als PDF speichern" → A4, Ränder: Keine (Ränder im CSS definiert), Hintergrundgrafiken: **aktivieren**.
