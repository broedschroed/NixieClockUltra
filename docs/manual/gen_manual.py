#!/usr/bin/env python3
"""Erzeugt die Bedienungsanleitung als A5-Heft (12 Seiten) im Steampunk-Stil.

Ausgabe (neben diesem Skript):
  NixieClockUltra-Bedienungsanleitung.odt   – bearbeitbare Vorlage (A5)
  NixieClockUltra-Bedienungsanleitung.pdf   – Heftseiten 1–12 in Lesereihenfolge (A5)
  NixieClockUltra-Bedienungsanleitung-Heftdruck-A4.pdf
      – ausgeschossen für den Heftdruck: A4 quer, beidseitig drucken
        (an der kurzen Kante wenden), falten, in der Mitte heften.
        Nur wenn das Python-Paket pypdf verfügbar ist.

Aufruf:  python3 gen_manual.py
Benötigt: LibreOffice (soffice/libreoffice) und pdftotext (poppler-utils).

Seitenzahlen im Inhaltsverzeichnis: Das Skript rendert das Heft einmal, liest
aus dem PDF ab, auf welcher Seite jedes Kapitel beginnt, und baut es dann mit
diesen Zahlen neu.
"""

import re, shutil, subprocess, sys, tempfile, zipfile
from pathlib import Path

HERE   = Path(__file__).resolve().parent
NAME   = "NixieClockUltra-Bedienungsanleitung"
ODT    = HERE / f"{NAME}.odt"
PDF    = HERE / f"{NAME}.pdf"
BOOKLET = HERE / "NixieClockUltra-Bedienungsanleitung-Heftdruck-A4.pdf"
PAGES  = 12                      # Heftumfang (durch 4 teilbar)
VERSION = "Version 2.1 · November 2026"

# Farben wie in der HTML-Anleitung
C_HEAD  = "#2c1810"   # Überschriften
C_TEXT  = "#3d2b00"   # Fließtext
C_GOLD  = "#b8860b"
C_GOLDDK = "#8b6914"
C_LINE  = "#d3c9a8"
C_HDRBG = "#efe6cf"   # Tabellenkopf
C_INFO  = "#faf7ef"
C_WARNBG = "#fff8f0"
C_WARN  = "#8b0000"
C_GLOW  = "#e8740c"   # Röhrenziffern auf dem Titel

F_BODY = "P052"          # Palatino-Schnitt
F_HEAD = "URW Bookman"
F_MONO = "Nimbus Mono PS"
F_SYM  = "DejaVu Sans"   # Zahnrad, Ziffernkreise

# ─── Inhalt ─────────────────────────────────────────────────────────────────
# Auszeichnung im Text: **fett**, `Schreibmaschine`

CHAPTERS = []   # (Nummer, Titel) für das Inhaltsverzeichnis


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def inline(s):
    """**fett** und `mono` in ODF-Spans umsetzen."""
    out = []
    for part in re.split(r"(\*\*.+?\*\*|`.+?`)", s):
        if part.startswith("**") and part.endswith("**"):
            out.append(f'<text:span text:style-name="B">{esc(part[2:-2])}</text:span>')
        elif part.startswith("`") and part.endswith("`"):
            out.append(f'<text:span text:style-name="M">{esc(part[1:-1])}</text:span>')
        else:
            out.append(esc(part))
    return "".join(out)


def chapter(num, title):
    """Kapitelkopf; jedes Kapitel beginnt auf einer neuen Seite."""
    CHAPTERS.append((num, title))
    return (f'<text:p text:style-name="Kapitel">Kapitel {num}</text:p>'
            f'<text:h text:style-name="H1" text:outline-level="1">{esc(title)}</text:h>')


def h2(title):
    return f'<text:h text:style-name="H2" text:outline-level="2">{esc(title)}</text:h>'


def h2_newpage(title):
    return f'<text:h text:style-name="H2Page" text:outline-level="2">{esc(title)}</text:h>'


def p(text, style="Body"):
    return f'<text:p text:style-name="{style}">{inline(text)}</text:p>'


def steps(*items):
    nums = "➊➋➌➍➎➏➐➑➒➓"
    return "".join(
        f'<text:p text:style-name="Step"><text:span text:style-name="Num">{nums[i]}</text:span>'
        f'<text:tab/>{inline(t)}</text:p>' for i, t in enumerate(items))


def info(text):
    return f'<text:p text:style-name="Info">{inline(text)}</text:p>'


def warn(title, text):
    return (f'<text:p text:style-name="WarnTitle"><text:span text:style-name="Sym">⚠</text:span> {esc(title)}</text:p>'
            f'<text:p text:style-name="Warn">{inline(text)}</text:p>')


_tables = []


def table(widths_mm, header, rows):
    """Tabelle; widths_mm = Spaltenbreiten in mm (Summe = Satzspiegel)."""
    n = len(_tables) + 1
    _tables.append(widths_mm)
    cols = "".join(f'<table:table-column table:style-name="T{n}.C{i}"/>' for i in range(len(widths_mm)))

    def row(cells, head=False):
        cs, ps = ("TH", "Th") if head else ("TD", "Td")
        return "<table:table-row>" + "".join(
            f'<table:table-cell table:style-name="{cs}" office:value-type="string">'
            f'<text:p text:style-name="{ps}">{inline(c)}</text:p></table:table-cell>' for c in cells
        ) + "</table:table-row>"

    hdr = f"<table:table-header-rows>{row(header, True)}</table:table-header-rows>" if header else ""
    body = "".join(row(r) for r in rows)
    return f'<table:table table:name="Tabelle{n}" table:style-name="T{n}">{cols}{hdr}{body}</table:table>'


def build_body(toc_pages):
    global CHAPTERS, _tables
    CHAPTERS, _tables = [], []
    W = 120  # Satzspiegelbreite in mm (148 − 2 × 14)
    b = []

    # ── Titelseite ──
    b.append('<text:p text:style-name="TitleTop"/>')
    b.append('<text:p text:style-name="Title">NIXIE CLOCK ULTRA</text:p>')
    b.append('<text:p text:style-name="Ornament">✦ ─────── ⚙ ─────── ✦</text:p>')
    b.append('<text:p text:style-name="Subtitle">Bedienungsanleitung</text:p>')
    b.append('<text:p text:style-name="Tubes">8 8 : 8 8 : 8 8</text:p>')
    b.append('<text:p text:style-name="Model">Röhrenuhr mit 6 Nixie-Röhren IN-12</text:p>')
    b.append(f'<text:p text:style-name="Version">{esc(VERSION)}</text:p>')

    # ── Seite 2: Inhalt ──
    b.append('<text:p text:style-name="TocTitle">Inhalt</text:p>')
    toc = [("I", "Übersicht & Sicherheit"), ("II", "Inbetriebnahme"), ("III", "Tasten & Bedienung"),
           ("IV", "Zeit & Datum einstellen"), ("V", "Beleuchtung & Animationen"),
           ("VI", "IR-Fernbedienung"), ("VII", "WLAN & Web-Interface"), ("VIII", "Nacht-Modus"),
           ("IX", "Einstellungen & Technische Daten")]
    for num, title in toc:
        pg = toc_pages.get(num, "–")
        b.append(f'<text:p text:style-name="Toc"><text:span text:style-name="TocNum">{num}.</text:span>'
                 f'<text:tab/>{esc(title)}<text:tab/>{pg}</text:p>')

    # ── I ──
    b.append(chapter("I", "Übersicht & Sicherheit"))
    b.append(p("Die Nixie Clock Ultra ist eine Röhrenuhr mit 6 Nixie-Röhren IN-12, farbiger "
               "Hintergrundbeleuchtung, einem Web-Interface und einem Infrarot-Empfänger. Jede "
               "Röhre wird direkt angesteuert – für maximale Helligkeit und eine saubere "
               "Darstellung ohne Geisterziffern."))
    b.append(warn("Sicherheitshinweis – Hochspannung",
                  "Die Nixie-Röhren werden mit 170–180 V Gleichspannung betrieben. Eine Berührung "
                  "ist wegen der geringen Leistung nicht lebensgefährlich, aber deutlich unangenehm. "
                  "Vor jeglichen Arbeiten an der Schaltung die Versorgung trennen und eine Minute "
                  "warten. Nie unter Spannung an der Schaltung arbeiten."))
    b.append(h2("Lieferumfang"))
    b.append(p("Die Uhr wird betriebsbereit geliefert. Eine Infrarot-Fernbedienung liegt nicht bei – "
               "nahezu jede handelsübliche Fernbedienung kann angelernt werden."))

    # ── II ──
    b.append(chapter("II", "Inbetriebnahme"))
    b.append(p("Nach dem Anschließen an die Spannungsversorgung (USB, 5 Volt) ist die Uhr sofort "
               "betriebsbereit."))
    b.append(steps(
        "Spannungsversorgung anschließen. Die Röhren erscheinen, die Beleuchtung blendet sanft auf.",
        "Die Uhr zeigt sofort die im internen Uhr-Chip gespeicherte Uhrzeit – auch ohne WLAN.",
        "Alle gespeicherten Einstellungen (Helligkeit, Animation, Fernbedienung, WLAN, "
        "Nacht-Modus) werden automatisch geladen.",
        "Der WLAN-Hotspot **NixieClock** (Passwort `nixie1234`) ist sofort aktiv und unter "
        "`192.168.4.1` erreichbar, im Heimnetz zusätzlich unter `nixieclock.local`."))
    b.append(info("Ist die Uhrzeit nach dem ersten Start falsch, wurde der Uhr-Chip noch nicht "
                  "gestellt. Zeit und Datum lassen sich per Taste (Kapitel IV) oder bequem im "
                  "Web-Interface korrigieren. Ist die Uhr mit dem Heimnetz verbunden, stellt sie "
                  "sich automatisch nach einem Zeitserver."))

    # ── III ──
    b.append(chapter("III", "Tasten & Bedienung"))
    b.append(p("Vier Taster erlauben alle wesentlichen Einstellungen direkt am Gerät. Jede Taste "
               "reagiert auf kurzen und – wo angegeben – auf langen Druck (mindestens 0,6 s)."))
    b.append(table([20, 55, 45], ["Taste", "Kurzer Druck", "Langer Druck"], [
        ["**SET**", "Einstellmodus starten · nächste Stelle · Speichern", "–"],
        ["**UP**", "Wert erhöhen", "Wert erhöhen (Dauerlauf)"],
        ["**DOWN**", "Wert verringern", "Wert verringern (Dauerlauf)"],
        ["**LIGHT**", "Helligkeit weiterschalten (4 Stufen)", "Trennpunkte dauerhaft an/aus"],
    ]))
    b.append(p("Alle Einstellungen wirken sofort und bleiben gespeichert. Dieselben Funktionen "
               "gibt es auch über Fernbedienung und Web-Interface."))

    # ── IV ──
    b.append(chapter("IV", "Zeit & Datum einstellen"))
    b.append(p("Die blinkende Stelle zeigt an, welcher Wert gerade bearbeitet wird. Mit **UP** "
               "und **DOWN** wird er eingestellt, langer Druck lässt ihn durchlaufen."))
    b.append(steps(
        "**SET** → Stunden blinken (0–23).",
        "**SET** → Minuten blinken (0–59).",
        "**SET** → Sekunden blinken (0–59).",
        "**SET** → Tag blinkt (1–31); die Anzeige wechselt auf das Datum TT MM JJ.",
        "**SET** → Monat blinkt (1–12).",
        "**SET** → Jahr blinkt (00–99).",
        "**SET** → alle Werte werden gespeichert, die Uhrzeit erscheint wieder."))
    b.append(info("**Abkürzung:** Wer nur die Uhrzeit korrigiert, wartet nach den Sekunden einfach "
                  "15 Sekunden – der Einstellmodus endet von selbst und speichert die Werte. Mit "
                  "Heimnetz stellt sich die Uhr ohnehin automatisch."))

    # ── V ──
    b.append(chapter("V", "Beleuchtung & Animationen"))
    b.append(h2("Helligkeit der Beleuchtung"))
    b.append(p("Ein kurzer Druck auf **LIGHT** schaltet die Hintergrundbeleuchtung durch vier "
               "Stufen. Die Einstellung bleibt gespeichert; feiner geht es im Web-Interface."))
    b.append(table([18, 102], ["Stufe", "Wirkung"], [
        ["1", "sehr dezent – minimale Hintergrundbeleuchtung"],
        ["2", "gedämpft – zurückhaltende Beleuchtung"],
        ["3", "hell – angenehme Raumbeleuchtung"],
        ["4", "voll – maximale Helligkeit"],
    ]))
    b.append(h2("Animationsmodi"))
    b.append(table([32, 88], ["Modus", "Wirkung"], [
        ["Rainbow", "Farbverlauf wandert über die Hintergrund-LEDs"],
        ["Warmweiß", "Alle Hintergrund-LEDs leuchten warmweiß"],
        ["Puls", "Die Beleuchtung dimmt sanft auf und ab"],
    ]))
    b.append(h2_newpage("Slot-Animation & Datumsanzeige"))
    b.append(p("Wie bei einem Spielautomaten rollen alle Ziffern durch und halten nacheinander an. "
               "Die Animation läuft auf Wunsch automatisch (aus, alle 10 Sekunden, jede Minute, "
               "Viertelstunde oder Stunde); ihre Geschwindigkeit ist im Web-Interface einstellbar. "
               "Danach zeigt die Uhr 5 Sekunden lang das Datum (TT MM JJ). Dabei leuchten nur die "
               "unteren Trennpunkte in gedämpftem Warmweiß."))
    b.append(h2("Weicher Ziffernwechsel"))
    b.append(p("Statt hart umzuschalten, dunkeln die Röhren beim Ziffernwechsel kurz ab und hellen "
               "mit der neuen Ziffer wieder auf – getrennt einstellbar für den Sekundentakt und den "
               "Wechsel zwischen Uhrzeit und Datum. Bei Uhren mit der Pro-Röhre-Dimmoption blendet "
               "nur die Röhre, deren Ziffer sich ändert."))
    b.append(h2("Trennpunkte"))
    b.append(p("Die vier Trennpunkte blinken im Sekundentakt. Ein langer Druck auf **LIGHT** "
               "lässt sie dauerhaft leuchten; im Web-Interface gibt es zusätzlich »statisch "
               "warmweiß«."))

    # ── VI ──
    b.append(chapter("VI", "IR-Fernbedienung"))
    b.append(p("Die Uhr lässt sich mit nahezu jeder Infrarot-Fernbedienung steuern. Jede Taste "
               "kann frei einer der acht Funktionen zugewiesen werden:"))
    b.append(table([34, 86], ["Funktion", "Wirkung"], [
        ["SET · UP · DOWN", "wie die gleichnamigen Tasten am Gerät"],
        ["BRIGHTNESS", "Helligkeit weiterschalten"],
        ["ANIM_NEXT", "nächster Animationsmodus"],
        ["SLOT", "Slot-Animation auslösen"],
        ["COLON_TOGGLE", "Trennpunkte dauerhaft an/aus"],
        ["DATUM", "Datum 5 Sekunden lang zeigen"],
    ]))
    b.append(h2("Fernbedienung anlernen"))
    b.append(p("Im Web-Interface unter **IR-Fernbedienung**:"))
    b.append(steps(
        "Neben der gewünschten Funktion auf **Anlernen** klicken – die Zeile blinkt orange.",
        "Beliebige Taste der Fernbedienung drücken – der Code wird sofort gespeichert."))
    b.append(info("Nach 10 Sekunden ohne Tastendruck bricht das Anlernen ab. Mit ✕ wird ein Code "
                  "gelöscht. Das automatische Wiederholsignal gedrückt gehaltener Tasten wird "
                  "herausgefiltert."))

    # ── VII ──
    b.append(chapter("VII", "WLAN & Web-Interface"))
    b.append(h2("Verbindung herstellen"))
    b.append(table([42, 78], None, [
        ["**WLAN (Hotspot)**", "`NixieClock`"],
        ["**Passwort**", "`nixie1234`"],
        ["**Adresse**", "`http://192.168.4.1`"],
        ["**Im Heimnetz**", "`http://nixieclock.local`"],
    ]))
    b.append(steps("Mit dem WLAN **NixieClock** verbinden.",
                   "Im Browser `http://192.168.4.1` aufrufen."))
    b.append(h2("Heimnetz einrichten"))
    b.append(p("Name und Passwort des Heimnetzes eingeben und **Verbinden** klicken – die Uhr "
               "startet neu und ist danach auch im Heimnetz erreichbar. Zum Trennen das Namensfeld "
               "leer lassen und **Verbinden** klicken. Im Heimnetz stellt sich die Uhr automatisch "
               "nach einem Zeitserver, die Sommerzeit wird berücksichtigt."))
    b.append(h2("Funktionen im Web-Interface"))
    b.append(table([36, 84], ["Bereich", "Möglichkeiten"], [
        ["Zeit & Datum", "von Hand eingeben oder die Zeit des Browsers übernehmen"],
        ["Beleuchtung", "Helligkeit fein einstellen, Animation, Slot-Intervall"],
        ["Trennpunkte", "blinken, dauerhaft an oder statisch warmweiß"],
        ["Ziffernwechsel", "weich oder hart, für Sekundentakt und Datum"],
        ["Nacht-Modus", "Zeitbereich und Lichtsensor"],
        ["Fernbedienung", "Funktionen anlernen und löschen"],
        ["WLAN", "Heimnetz verbinden oder trennen"],
        ["Röhrentest", "alle Röhren zeigen gemeinsam jede Ziffer von 0 bis 9 – zum Prüfen jeder "
                       "Ziffer; rund 20 Sekunden, Abbruch über dieselbe Schaltfläche"],
    ]))

    # ── VIII ──
    b.append(chapter("VIII", "Nacht-Modus"))
    b.append(p("Der Nacht-Modus dimmt Röhren und Beleuchtung oder schaltet sie ab – zu festen "
               "Zeiten oder abhängig vom Umgebungslicht. Eingestellt wird er im Web-Interface."))
    b.append(table([22, 52, 46], ["Modus", "Röhren", "Beleuchtung"], [
        ["Normal", "volle Helligkeit", "volle Helligkeit"],
        ["Gedimmt", "2–60 % (Standard 25 %)", "15 % der Einstellung"],
        ["Dunkel", "aus", "aus"],
    ]))
    b.append(h2("Zeitbereich"))
    b.append(p("Zwischen **Von** und **Bis** (volle Stunden, auch über Mitternacht, z. B. 23 bis 7) "
               "wechselt die Uhr in den gewählten Modus – Gedimmt oder Dunkel."))
    b.append(h2("Lichtsensor"))
    b.append(p("Fällt die gemessene Helligkeit unter den **Schwellwert** (0–4095, höher = dimmt "
               "früher), wechselt die Uhr auf Gedimmt. Der aktuelle Messwert wird im "
               "Web-Interface angezeigt."))
    b.append(info("**Vorrang:** Ein aktives Zeitfenster geht dem Lichtsensor vor. Der Lichtsensor "
                  "kann nur dimmen, nicht ganz abschalten."))

    # ── IX ──
    b.append(chapter("IX", "Einstellungen & Technische Daten"))
    b.append(p("Alle Einstellungen bleiben dauerhaft gespeichert und sind nach jedem Neustart "
               "wieder da: Helligkeit, Animation und Slot-Intervall, Ziffernwechsel, Trennpunkte, "
               "Nacht-Modus, WLAN-Zugang und die angelernten Fernbedienungs-Tasten. Uhrzeit und "
               "Datum hält der batteriegepufferte Uhr-Chip auch bei Stromausfall."))
    b.append(table([42, 78], None, [
        ["**Röhren**", "6 × Nixie IN-12"],
        ["**Hochspannung**", "170–180 V DC"],
        ["**Versorgung**", "5 V über USB"],
        ["**Mikrocontroller**", "ESP32-S3 mit WLAN"],
        ["**Uhr-Chip**", "DS1302, Pufferbatterie CR2032"],
        ["**Beleuchtung**", "10 RGB-LEDs (6 Hintergrund, 4 Trennpunkte)"],
        ["**Zeitzone**", "Mitteleuropa, automatische Sommerzeit"],
    ]))
    b.append('<text:p text:style-name="End">✦ ⚙ ✦</text:p>')
    return "".join(b)


# ─── Stile ──────────────────────────────────────────────────────────────────

def styles_xml():
    return f'''<?xml version="1.0" encoding="UTF-8"?>
<office:document-styles {NS} office:version="1.3">
<office:font-face-decls>
 <style:font-face style:name="{F_BODY}" svg:font-family="'{F_BODY}'" style:font-family-generic="roman"/>
 <style:font-face style:name="{F_HEAD}" svg:font-family="'{F_HEAD}'" style:font-family-generic="roman"/>
 <style:font-face style:name="{F_MONO}" svg:font-family="'{F_MONO}'" style:font-family-generic="modern" style:font-pitch="fixed"/>
 <style:font-face style:name="{F_SYM}" svg:font-family="'{F_SYM}'" style:font-family-generic="swiss"/>
</office:font-face-decls>
<office:styles>
 <style:default-style style:family="paragraph">
  <style:paragraph-properties fo:hyphenation-ladder-count="2" style:writing-mode="lr-tb"/>
  <style:text-properties style:font-name="{F_BODY}" fo:font-size="9.5pt" fo:language="de" fo:country="DE"
     fo:color="{C_TEXT}" fo:hyphenate="true" fo:hyphenation-remain-char-count="3" fo:hyphenation-push-char-count="3"/>
 </style:default-style>
 <style:default-style style:family="table-cell">
  <style:table-cell-properties fo:padding="0.8mm"/>
 </style:default-style>

 <style:style style:name="Body" style:family="paragraph">
  <style:paragraph-properties fo:text-align="justify" fo:margin-top="0mm" fo:margin-bottom="2mm" fo:line-height="120%"
     fo:orphans="2" fo:widows="2"/>
 </style:style>
 <style:style style:name="Kapitel" style:family="paragraph">
  <style:paragraph-properties fo:margin-top="2mm" fo:margin-bottom="0mm" fo:keep-with-next="always" fo:break-before="page"/>
  <style:text-properties style:font-name="{F_HEAD}" fo:font-size="7.5pt" fo:color="{C_GOLD}" fo:letter-spacing="0.6mm"
     fo:text-transform="uppercase"/>
 </style:style>
 <style:style style:name="H1" style:family="paragraph" style:default-outline-level="1">
  <style:paragraph-properties fo:margin-top="0.6mm" fo:margin-bottom="3.5mm" fo:keep-with-next="always"
     fo:border-bottom="1.1pt double {C_GOLD}" fo:padding-bottom="0.8mm"/>
  <style:text-properties style:font-name="{F_HEAD}" fo:font-size="15pt" fo:color="{C_HEAD}" fo:hyphenate="false"/>
 </style:style>
 <style:style style:name="H2" style:family="paragraph" style:default-outline-level="2">
  <style:paragraph-properties fo:margin-top="3.5mm" fo:margin-bottom="1.2mm" fo:keep-with-next="always"/>
  <style:text-properties style:font-name="{F_HEAD}" fo:font-size="10pt" fo:font-weight="bold" fo:color="{C_GOLDDK}"/>
 </style:style>
 <style:style style:name="H2Page" style:family="paragraph" style:parent-style-name="H2">
  <style:paragraph-properties fo:break-before="page" fo:margin-top="2mm"/>
 </style:style>
 <style:style style:name="Step" style:family="paragraph" style:parent-style-name="Body">
  <style:paragraph-properties fo:text-align="start" fo:margin-left="5.5mm" fo:text-indent="-5.5mm" fo:margin-bottom="1mm">
   <style:tab-stops><style:tab-stop style:position="0mm"/></style:tab-stops>
  </style:paragraph-properties>
 </style:style>
 <style:style style:name="Info" style:family="paragraph" style:parent-style-name="Body">
  <style:paragraph-properties fo:text-align="start" fo:background-color="{C_INFO}" fo:border-left="1.6pt solid {C_GOLD}"
     fo:border-right="none" fo:border-top="none" fo:border-bottom="none" fo:padding="1.6mm" fo:padding-left="2.4mm"
     fo:margin-top="1.2mm" fo:margin-bottom="2mm" fo:keep-together="always"/>
  <style:text-properties fo:font-size="8.5pt"/>
 </style:style>
 <style:style style:name="WarnTitle" style:family="paragraph">
  <style:paragraph-properties fo:background-color="{C_WARNBG}" fo:border-top="0.8pt solid {C_WARN}" fo:border-left="0.8pt solid {C_WARN}"
     fo:border-right="0.8pt solid {C_WARN}" fo:border-bottom="none" fo:padding="1.6mm" fo:padding-bottom="0.4mm"
     fo:margin-top="1mm" fo:keep-with-next="always"/>
  <style:text-properties style:font-name="{F_HEAD}" fo:font-size="8.5pt" fo:font-weight="bold" fo:color="{C_WARN}"/>
 </style:style>
 <style:style style:name="Warn" style:family="paragraph" style:parent-style-name="Body">
  <style:paragraph-properties fo:text-align="start" fo:background-color="{C_WARNBG}" fo:border-bottom="0.8pt solid {C_WARN}"
     fo:border-left="0.8pt solid {C_WARN}" fo:border-right="0.8pt solid {C_WARN}" fo:border-top="none"
     fo:padding="1.6mm" fo:padding-top="0.4mm" fo:margin-bottom="2mm" fo:keep-together="always"/>
  <style:text-properties fo:font-size="8.5pt" fo:color="#5a2000"/>
 </style:style>
 <style:style style:name="Th" style:family="paragraph">
  <style:text-properties style:font-name="{F_HEAD}" fo:font-size="8pt" fo:font-weight="bold" fo:color="{C_HEAD}"/>
 </style:style>
 <style:style style:name="Td" style:family="paragraph">
  <style:paragraph-properties fo:line-height="112%"/>
  <style:text-properties fo:font-size="9pt" fo:hyphenate="false"/>
 </style:style>
 <style:style style:name="TocTitle" style:family="paragraph">
  <style:paragraph-properties fo:text-align="center" fo:margin-top="14mm" fo:margin-bottom="8mm"
     fo:border-bottom="1.1pt double {C_GOLD}" fo:padding-bottom="1.5mm"/>
  <style:text-properties style:font-name="{F_HEAD}" fo:font-size="15pt" fo:color="{C_HEAD}" fo:letter-spacing="0.8mm"/>
 </style:style>
 <style:style style:name="Toc" style:family="paragraph">
  <style:paragraph-properties fo:margin-left="4mm" fo:margin-right="4mm" fo:margin-bottom="4.5mm">
   <style:tab-stops>
    <style:tab-stop style:position="11mm"/>
    <style:tab-stop style:position="112mm" style:type="right" style:leader-style="dotted" style:leader-text="."/>
   </style:tab-stops>
  </style:paragraph-properties>
  <style:text-properties style:font-name="{F_BODY}" fo:font-size="10pt" fo:color="{C_HEAD}"/>
 </style:style>
 <style:style style:name="Title" style:family="paragraph">
  <style:paragraph-properties fo:text-align="center" fo:margin-bottom="3mm"/>
  <style:text-properties style:font-name="{F_HEAD}" fo:font-size="21pt" fo:color="{C_HEAD}" fo:letter-spacing="1.2mm"/>
 </style:style>
 <style:style style:name="TitleTop" style:family="paragraph">
  <style:paragraph-properties fo:margin-top="30mm"/>
 </style:style>
 <style:style style:name="Ornament" style:family="paragraph">
  <style:paragraph-properties fo:text-align="center" fo:margin-bottom="3mm"/>
  <style:text-properties style:font-name="{F_SYM}" fo:font-size="10pt" fo:color="{C_GOLD}"/>
 </style:style>
 <style:style style:name="Subtitle" style:family="paragraph">
  <style:paragraph-properties fo:text-align="center" fo:margin-bottom="22mm"/>
  <style:text-properties style:font-name="{F_HEAD}" fo:font-size="13pt" fo:color="{C_GOLDDK}" fo:letter-spacing="0.9mm"
     fo:font-style="italic"/>
 </style:style>
 <style:style style:name="Tubes" style:family="paragraph">
  <style:paragraph-properties fo:text-align="center" fo:margin-bottom="22mm"/>
  <style:text-properties style:font-name="{F_BODY}" fo:font-size="34pt" fo:color="{C_GLOW}" fo:letter-spacing="0.6mm"/>
 </style:style>
 <style:style style:name="Model" style:family="paragraph">
  <style:paragraph-properties fo:text-align="center" fo:margin-bottom="2mm"/>
  <style:text-properties fo:font-size="9.5pt" fo:color="{C_HEAD}"/>
 </style:style>
 <style:style style:name="Version" style:family="paragraph">
  <style:paragraph-properties fo:text-align="center" fo:break-after="page"/>
  <style:text-properties fo:font-size="8.5pt" fo:color="{C_GOLDDK}"/>
 </style:style>
 <style:style style:name="End" style:family="paragraph">
  <style:paragraph-properties fo:text-align="center" fo:margin-top="4mm"/>
  <style:text-properties style:font-name="{F_SYM}" fo:font-size="9pt" fo:color="{C_GOLD}"/>
 </style:style>
 <style:style style:name="Footer" style:family="paragraph">
  <style:paragraph-properties fo:text-align="center" fo:border-top="0.5pt solid {C_LINE}" fo:padding-top="1mm"/>
  <style:text-properties style:font-name="{F_HEAD}" fo:font-size="7pt" fo:color="{C_GOLDDK}" fo:letter-spacing="0.3mm"/>
 </style:style>

 <style:style style:name="B" style:family="text"><style:text-properties fo:font-weight="bold" fo:color="{C_HEAD}"/></style:style>
 <style:style style:name="M" style:family="text"><style:text-properties style:font-name="{F_MONO}" fo:font-size="8.5pt" fo:color="{C_HEAD}" fo:hyphenate="false"/></style:style>
 <style:style style:name="Num" style:family="text"><style:text-properties style:font-name="{F_SYM}" fo:color="{C_GOLD}"/></style:style>
 <style:style style:name="TocNum" style:family="text"><style:text-properties style:font-name="{F_HEAD}" fo:color="{C_GOLD}"/></style:style>
 <style:style style:name="Sym" style:family="text"><style:text-properties style:font-name="{F_SYM}"/></style:style>
 <style:style style:name="Gear" style:family="text"><style:text-properties style:font-name="{F_SYM}" fo:color="{C_GOLD}"/></style:style>
</office:styles>
<office:automatic-styles>
 <style:page-layout style:name="A5">
  <style:page-layout-properties fo:page-width="148mm" fo:page-height="210mm" style:print-orientation="portrait"
     fo:margin-top="13mm" fo:margin-bottom="9mm" fo:margin-left="14mm" fo:margin-right="14mm"/>
  <style:footer-style><style:header-footer-properties fo:min-height="0mm" fo:margin-top="3mm"/></style:footer-style>
 </style:page-layout>
</office:automatic-styles>
<office:master-styles>
 <style:master-page style:name="Standard" style:page-layout-name="A5">
  <style:footer>
   <text:p text:style-name="Footer">Nixie Clock Ultra  <text:span text:style-name="Gear">⚙</text:span>  <text:page-number text:select-page="current"/></text:p>
  </style:footer>
 </style:master-page>
 <style:master-page style:name="Cover" style:page-layout-name="A5" style:next-style-name="Standard"/>
</office:master-styles>
</office:document-styles>'''


NS = ('xmlns:office="urn:oasis:names:tc:opendocument:xmlns:office:1.0" '
      'xmlns:style="urn:oasis:names:tc:opendocument:xmlns:style:1.0" '
      'xmlns:text="urn:oasis:names:tc:opendocument:xmlns:text:1.0" '
      'xmlns:table="urn:oasis:names:tc:opendocument:xmlns:table:1.0" '
      'xmlns:fo="urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0" '
      'xmlns:svg="urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0" '
      'xmlns:meta="urn:oasis:names:tc:opendocument:xmlns:meta:1.0" '
      'xmlns:dc="http://purl.org/dc/elements/1.1/"')


def content_xml(toc_pages):
    body = build_body(toc_pages)
    auto = [
        # erste Seite (Titel) ohne Fußzeile
        '<style:style style:name="First" style:family="paragraph" style:parent-style-name="TitleTop" '
        'style:master-page-name="Cover"/>',
        f'<style:style style:name="TH" style:family="table-cell"><style:table-cell-properties '
        f'fo:background-color="{C_HDRBG}" fo:border-top="0.8pt solid {C_GOLD}" fo:border-bottom="0.8pt solid {C_GOLD}" '
        f'fo:border-left="none" fo:border-right="none" fo:padding="1mm"/></style:style>',
        f'<style:style style:name="TD" style:family="table-cell"><style:table-cell-properties '
        f'fo:border-bottom="0.4pt solid {C_LINE}" fo:border-top="none" fo:border-left="none" fo:border-right="none" '
        f'fo:padding="1mm"/></style:style>',
    ]
    for n, widths in enumerate(_tables, 1):
        auto.append(f'<style:style style:name="T{n}" style:family="table"><style:table-properties '
                    f'style:width="{sum(widths)}mm" table:align="left" fo:margin-top="1mm" fo:margin-bottom="2.4mm" '
                    f'style:may-break-between-rows="false" fo:keep-with-next="auto"/></style:style>')
        for i, w in enumerate(widths):
            auto.append(f'<style:style style:name="T{n}.C{i}" style:family="table-column">'
                        f'<style:table-column-properties style:column-width="{w}mm"/></style:style>')
    body = body.replace('<text:p text:style-name="TitleTop"/>', '<text:p text:style-name="First"/>', 1)
    return (f'<?xml version="1.0" encoding="UTF-8"?>\n<office:document-content {NS} office:version="1.3">'
            f'<office:automatic-styles>{"".join(auto)}</office:automatic-styles>'
            f'<office:body><office:text>{body}</office:text></office:body></office:document-content>')


MANIFEST = '''<?xml version="1.0" encoding="UTF-8"?>
<manifest:manifest xmlns:manifest="urn:oasis:names:tc:opendocument:xmlns:manifest:1.0" manifest:version="1.3">
 <manifest:file-entry manifest:full-path="/" manifest:media-type="application/vnd.oasis.opendocument.text"/>
 <manifest:file-entry manifest:full-path="content.xml" manifest:media-type="text/xml"/>
 <manifest:file-entry manifest:full-path="styles.xml" manifest:media-type="text/xml"/>
 <manifest:file-entry manifest:full-path="meta.xml" manifest:media-type="text/xml"/>
</manifest:manifest>'''

META = f'''<?xml version="1.0" encoding="UTF-8"?>
<office:document-meta {NS} office:version="1.3"><office:meta>
 <dc:title>Nixie Clock Ultra – Bedienungsanleitung</dc:title><dc:language>de-DE</dc:language>
</office:meta></office:document-meta>'''


def write_odt(path, toc_pages):
    content = content_xml(toc_pages)   # füllt _tables
    with zipfile.ZipFile(path, "w") as z:
        z.writestr(zipfile.ZipInfo("mimetype"), "application/vnd.oasis.opendocument.text")
        z.writestr("META-INF/manifest.xml", MANIFEST, zipfile.ZIP_DEFLATED)
        z.writestr("styles.xml", styles_xml(), zipfile.ZIP_DEFLATED)
        z.writestr("content.xml", content, zipfile.ZIP_DEFLATED)
        z.writestr("meta.xml", META, zipfile.ZIP_DEFLATED)


def to_pdf(odt, outdir):
    office = shutil.which("libreoffice") or shutil.which("soffice")
    if not office:
        sys.exit("LibreOffice nicht gefunden")
    subprocess.run([office, "--headless", "--convert-to", "pdf", "--outdir", str(outdir), str(odt)],
                   check=True, capture_output=True)
    return Path(outdir) / (odt.stem + ".pdf")


def page_texts(pdf):
    out = subprocess.run(["pdftotext", "-layout", str(pdf), "-"], check=True, capture_output=True, text=True).stdout
    return out.split("\f")[:-1] if out.endswith("\f") else out.split("\f")


def chapter_pages(pdf):
    pages = page_texts(pdf)
    found = {}
    for num, _ in CHAPTERS:
        for i, t in enumerate(pages):
            if i >= 2 and re.search(rf"KAPITEL {num}\b", t):   # Seite 1/2 = Titel/Inhalt
                found[num] = i + 1
                break
    return found, len(pages)


def make_booklet(src, dst, n):
    """A5-Seiten auf A4 quer ausschießen (Rückstichheftung)."""
    try:
        from pypdf import PdfReader, PdfWriter, Transformation, PageObject
    except ImportError:
        print("Hinweis: pypdf nicht installiert – Heftdruck-PDF übersprungen "
              "(LibreOffice: Drucken → Seitenlayout → Broschüre geht auch).")
        return False
    r = PdfReader(str(src))
    w = PdfWriter()
    a5w, a5h = float(r.pages[0].mediabox.width), float(r.pages[0].mediabox.height)
    order = []
    for s in range(n // 4):                     # je Bogen: Vorderseite, Rückseite
        order.append((n - 2 * s, 1 + 2 * s))
        order.append((2 + 2 * s, n - 1 - 2 * s))
    for left, right in order:
        sheet = PageObject.create_blank_page(width=2 * a5w, height=a5h)
        sheet.merge_transformed_page(r.pages[left - 1], Transformation())
        sheet.merge_transformed_page(r.pages[right - 1], Transformation().translate(a5w, 0))
        w.add_page(sheet)
    with open(dst, "wb") as f:
        w.write(f)
    return True


def main():
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        probe = tmp / f"{NAME}.odt"
        write_odt(probe, {})
        pages, count = chapter_pages(to_pdf(probe, tmp))
        write_odt(ODT, pages)
        pdf = to_pdf(ODT, tmp)
        pages2, count2 = chapter_pages(pdf)
        shutil.copy(pdf, PDF)
    print(f"Seiten: {count2} (Ziel {PAGES}) · Kapitelanfänge: {pages2}")
    if pages2 != pages:
        print("WARNUNG: Seitenzahlen haben sich im zweiten Durchlauf verschoben")
    if count2 != PAGES:
        print(f"WARNUNG: Heft hat {count2} statt {PAGES} Seiten")
    print(f"→ {ODT.name}\n→ {PDF.name}")
    if count2 % 4 == 0 and make_booklet(PDF, BOOKLET, count2):
        print(f"→ {BOOKLET.name}")


if __name__ == "__main__":
    main()
