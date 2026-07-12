#!/usr/bin/env python3
"""Generiert NixieClockUltra-Werbetext.odt"""

import zipfile, struct
from pathlib import Path

OUT = Path("/home/andreas/Dokumente/_Develop/Arduino/NixieClockUltra/docs/marketing/NixieClockUltra-Werbetext.odt")

# ─── XML-Hilfsfunktionen ────────────────────────────────────────────────────

def esc(s):
    return str(s).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")

def h1(text):
    return f'<text:h text:style-name="Heading_1" text:outline-level="1">{esc(text)}</text:h>\n'

def h2(text):
    return f'<text:h text:style-name="Heading_2" text:outline-level="2">{esc(text)}</text:h>\n'

def p(text="", style="Text_Body"):
    return f'<text:p text:style-name="{style}">{esc(text)}</text:p>\n'

def br():
    return '<text:p text:style-name="Text_Body"/>\n'

def tagline(text):
    return f'<text:p text:style-name="Tagline">{esc(text)}</text:p>\n'

def bullet(text):
    return (
        '<text:list text:style-name="BulletList">'
        '<text:list-item>'
        f'<text:p text:style-name="ListItem">{esc(text)}</text:p>'
        '</text:list-item>'
        '</text:list>\n'
    )

def section_label(text):
    return f'<text:p text:style-name="SectionLabel">{esc(text)}</text:p>\n'

# ─── Styles ─────────────────────────────────────────────────────────────────

STYLES_XML = """\
<?xml version="1.0" encoding="UTF-8"?>
<office:document-styles
  xmlns:office="urn:oasis:names:tc:opendocument:xmlns:office:1.0"
  xmlns:style="urn:oasis:names:tc:opendocument:xmlns:style:1.0"
  xmlns:text="urn:oasis:names:tc:opendocument:xmlns:text:1.0"
  xmlns:fo="urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0"
  xmlns:svg="urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0"
  office:version="1.3">

<office:styles>
  <style:style style:name="PageLayout" style:family="page-layout">
    <style:page-layout-properties fo:page-width="21cm" fo:page-height="29.7cm"
      fo:margin-top="3cm" fo:margin-bottom="3cm"
      fo:margin-left="3cm" fo:margin-right="3cm"/>
  </style:style>

  <style:style style:name="Title" style:family="paragraph">
    <style:paragraph-properties fo:text-align="center" fo:margin-top="4cm" fo:margin-bottom="0.3cm"/>
    <style:text-properties fo:font-size="32pt" fo:font-weight="bold" fo:color="#1a1a2e"
      fo:letter-spacing="0.1cm"/>
  </style:style>

  <style:style style:name="Tagline" style:family="paragraph">
    <style:paragraph-properties fo:text-align="center" fo:margin-bottom="0.6cm"/>
    <style:text-properties fo:font-size="13pt" fo:color="#8b6914" fo:font-style="italic"/>
  </style:style>

  <style:style style:name="Subtitle" style:family="paragraph">
    <style:paragraph-properties fo:text-align="center" fo:margin-bottom="2cm"/>
    <style:text-properties fo:font-size="11pt" fo:color="#555555"/>
  </style:style>

  <style:style style:name="Heading_1" style:family="paragraph">
    <style:paragraph-properties fo:margin-top="1.2cm" fo:margin-bottom="0.4cm"
      fo:keep-with-next="always" fo:border-bottom="1.5pt solid #c8a000"
      fo:padding-bottom="0.15cm"/>
    <style:text-properties fo:font-size="16pt" fo:font-weight="bold" fo:color="#1a1a2e"/>
  </style:style>

  <style:style style:name="Heading_2" style:family="paragraph">
    <style:paragraph-properties fo:margin-top="0.7cm" fo:margin-bottom="0.25cm"
      fo:keep-with-next="always"/>
    <style:text-properties fo:font-size="11pt" fo:font-weight="bold" fo:color="#8b6914"/>
  </style:style>

  <style:style style:name="Text_Body" style:family="paragraph">
    <style:paragraph-properties fo:margin-top="0.1cm" fo:margin-bottom="0.3cm"
      fo:text-align="justify" fo:line-height="150%"/>
    <style:text-properties fo:font-size="11pt" fo:color="#222222"/>
  </style:style>

  <style:style style:name="SectionLabel" style:family="paragraph">
    <style:paragraph-properties fo:margin-top="0.5cm" fo:margin-bottom="0.15cm"
      fo:keep-with-next="always"/>
    <style:text-properties fo:font-size="10pt" fo:font-weight="bold" fo:color="#1a1a2e"
      fo:text-transform="uppercase" fo:letter-spacing="0.05cm"/>
  </style:style>

  <style:style style:name="ListItem" style:family="paragraph">
    <style:paragraph-properties fo:margin-left="0.6cm" fo:margin-bottom="0.15cm"
      fo:line-height="140%"/>
    <style:text-properties fo:font-size="10.5pt" fo:color="#222222"/>
  </style:style>

  <style:style style:name="Footer" style:family="paragraph">
    <style:paragraph-properties fo:text-align="center"/>
    <style:text-properties fo:font-size="9pt" fo:color="#888888" fo:font-style="italic"/>
  </style:style>

  <text:list-style style:name="BulletList">
    <text:list-level-style-bullet text:level="1" text:bullet-char="›">
      <style:list-level-properties text:space-before="0.4cm" text:min-label-width="0.4cm"/>
      <style:text-properties fo:color="#c8a000" fo:font-size="13pt"/>
    </text:list-level-style-bullet>
  </text:list-style>
</office:styles>

<office:master-styles>
  <style:master-page style:name="Standard" style:page-layout-name="PageLayout">
    <style:footer>
      <text:p text:style-name="Footer">Nixie Clock Ultra · broed digital media · 2026</text:p>
    </style:footer>
  </style:master-page>
</office:master-styles>
</office:document-styles>
"""

# ─── Dokumentinhalt ──────────────────────────────────────────────────────────

def build_content():
    c = []

    # Titelseite
    c.append(p("Nixie Clock Ultra", "Title"))
    c.append(tagline("wenn Zeit leuchtet"))
    c.append(p("Handgefertigte Röhrenuhr · ESP32-S3 · 6 × IN-12A · WiFi & Web-Interface", "Subtitle"))

    # Fließtext
    c.append(h1("Eine Uhr, die man nicht kauft, weil man die Zeit nicht kennt"))
    c.append(p("Nixie-Röhren gehören zu den faszinierendsten Anzeigetechnologien, die "
               "je entwickelt wurden: warmes, leicht diffuses Licht, das Ziffern "
               "plastisch aus dem Glas herausleuchten lässt — unverwechselbar, zeitlos "
               "und heute begehrter denn je. Die Nixie Clock Ultra bringt dieses "
               "Feeling ins 21. Jahrhundert, ohne seinen Charakter zu verlieren."))
    c.append(p("Sechs IN-12A-Nixieröhren zeigen Stunden, Minuten und Sekunden — und "
               "auf Wunsch auch das Datum. Hinter jeder Röhre sitzt eine individuell "
               "ansteuerbare RGB-LED, die das Glas von innen in warmem Licht "
               "erstrahlen lässt: statisch, im sanften Atemrhythmus oder als "
               "fließendes Farbspektrum. Vier weitere LEDs beleuchten die "
               "Trennpunkte zwischen den Zifferngruppen."))
    c.append(p("Was diese Uhr von den meisten anderen Nixie-Projekten unterscheidet, "
               "ist die Art der Röhrenansteuerung: Kein Multiplexing, keine Kompromisse. "
               "Jede der 60 Kathoden besitzt ihren eigenen Hochvolt-Transistor — "
               "die Ziffern leuchten dauerhaft, ohne jegliches Ghosting oder Flimmern. "
               "Das Ergebnis ist ein sauberes, stabiles Bild, das man sofort sieht "
               "und lange nicht vergisst."))
    c.append(p("Die Uhr denkt mit: Ein ESP32-S3 verbindet sich mit dem Heimnetz, "
               "synchronisiert die Uhrzeit vollautomatisch per NTP aus dem Internet "
               "und ist über jeden Browser erreichbar — kein App-Download, kein Pairing. "
               "Über das Web-Interface lassen sich Helligkeit, Farben, Animationen, "
               "Nacht-Modus und vieles mehr bequem einstellen. Wer lieber zur "
               "Fernbedienung greift: fast jede handelsübliche IR-Fernbedienung "
               "lässt sich anlernen."))
    c.append(p("Für ruhige Nächte sorgt der Nacht-Modus: Die Uhr dimmt sich auf "
               "Wunsch zu einer konfigurierbaren Uhrzeit automatisch herunter — "
               "oder reagiert direkt auf die Umgebungshelligkeit über einen "
               "eingebauten Lichtsensor. Im Dunkelmodus erlöschen die Röhren "
               "vollständig."))
    c.append(p("Und weil eine Uhr auch eine Uhr sein soll: Die Echtzeituhr mit "
               "CR2032-Backup-Batterie hält die Zeit auch bei Stromausfall zuverlässig. "
               "Beim nächsten Einschalten zeigt die Nixie Clock Ultra sofort die "
               "richtige Zeit — und blendet sie mit einem sanften Fade-In ein."))
    c.append(p("Die Nixie Clock Ultra ist ein Unikat — handgefertigte Einzelplatinen, "
               "sorgfältig bestückt und getestet. Kein Bausatz, keine Kompromisse. "
               "Eine Uhr, die man nicht kauft, weil man die Uhrzeit nicht kennt — "
               "sondern weil man sie kennen will."))

    # Feature-Liste
    c.append(h1("Features auf einen Blick"))

    c.append(section_label("Anzeige"))
    c.append(bullet("6 × IN-12A Nixie-Röhren — direkte Kathodensteuerung ohne Multiplexing, kein Ghosting, kein Flimmern"))
    c.append(bullet("Stunden · Minuten · Sekunden sowie Datumsanzeige (TT MM JJ)"))
    c.append(bullet("Sanfter Fade-In beim Einschalten"))
    c.append(bullet("Optional weicher Ziffernwechsel (Crossfade) — einzeln zuschaltbar für Sekundentakt und für den Wechsel zwischen Uhrzeit und Datum"))

    c.append(section_label("Beleuchtung"))
    c.append(bullet("6 × WS2812B RGB-LEDs als Röhrenhintergrundbeleuchtung (individuell pro Röhre)"))
    c.append(bullet("4 × WS2812B RGB-LEDs für die Trennpunkt-Beleuchtung"))
    c.append(bullet("Animationsmodi: Rainbow, Statisch (warmweiß), Puls"))
    c.append(bullet("Slot-Machine-Effekt: konfigurierbar (aus / alle 10 s / 1 min / 15 min / 1 h), zeigt danach automatisch das Datum"))
    c.append(bullet("Rollgeschwindigkeit der Slot-Machine-Animation stufenlos einstellbar"))

    c.append(section_label("Konnektivität & Bedienung"))
    c.append(bullet("ESP32-S3 mit integriertem WLAN (2,4 GHz)"))
    c.append(bullet("Gleichzeitig eigener WLAN-Accesspoint (kein Heimnetz notwendig) und WLAN-Client"))
    c.append(bullet("Automatische Zeitsynchronisation per NTP"))
    c.append(bullet("Vollständiges Web-Interface im Browser — kein App-Download, funktioniert auf Smartphone, Tablet und PC"))
    c.append(bullet("Erreichbar per IP oder als nixieclockcs.local im Heimnetz (mDNS)"))
    c.append(bullet("IR-Fernbedienung: 8 Funktionen frei auf jede handelsübliche Fernbedienung anlernbar"))
    c.append(bullet("4 Bedientaster direkt an der Uhr"))

    c.append(section_label("Nacht-Modus"))
    c.append(bullet("Zeitgesteuertes Dimmen oder vollständiges Ausschalten der Röhren"))
    c.append(bullet("Automatische Helligkeitsanpassung über eingebauten Lichtsensor (LDR)"))
    c.append(bullet("NeoPixel-Helligkeit wird im Nacht-Modus proportional mitgedimmt"))

    c.append(section_label("Technik"))
    c.append(bullet("Zweiplatinen-Design: Logic Board (ESP32-S3, RTC, Spannungsversorgung) + Nixie Display Board"))
    c.append(bullet("DS1302 Echtzeituhr mit CR2032-Batterie-Backup — hält Zeit auch ohne Strom"))
    c.append(bullet("Hochvolt-Versorgung: 5 V USB → DC-DC-Konverter → 10 V → HV-Modul → ~170 V stabile Anodenspannung"))
    c.append(bullet("60 diskrete SMBTA42-Hochvolt-Transistoren (je eine Kathode, je eine Röhrenziffer)"))
    c.append(bullet("Alle Einstellungen werden im Flash gespeichert und bleiben nach Stromunterbrechung erhalten"))
    c.append(bullet("Stromversorgung: Micro-USB, 5 V"))

    return "".join(c)


# ─── ODT-Aufbau ─────────────────────────────────────────────────────────────

CONTENT_NS = """\
<?xml version="1.0" encoding="UTF-8"?>
<office:document-content
  xmlns:office="urn:oasis:names:tc:opendocument:xmlns:office:1.0"
  xmlns:style="urn:oasis:names:tc:opendocument:xmlns:style:1.0"
  xmlns:text="urn:oasis:names:tc:opendocument:xmlns:text:1.0"
  xmlns:fo="urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0"
  office:version="1.3">
<office:automatic-styles/>
<office:body><office:text>
"""

META_XML = """\
<?xml version="1.0" encoding="UTF-8"?>
<office:document-meta
  xmlns:office="urn:oasis:names:tc:opendocument:xmlns:office:1.0"
  xmlns:dc="http://purl.org/dc/elements/1.1/"
  xmlns:meta="urn:oasis:names:tc:opendocument:xmlns:meta:1.0"
  office:version="1.3">
<office:meta>
  <dc:title>Nixie Clock Ultra – Werbetext</dc:title>
  <dc:creator>broed digital media</dc:creator>
  <dc:date>2026-06-29</dc:date>
  <meta:generator>gen_werbetext.py</meta:generator>
</office:meta>
</office:document-meta>"""

MANIFEST_XML = """\
<?xml version="1.0" encoding="UTF-8"?>
<manifest:manifest xmlns:manifest="urn:oasis:names:tc:opendocument:xmlns:manifest:1.0" manifest:version="1.3">
  <manifest:file-entry manifest:full-path="/" manifest:media-type="application/vnd.oasis.opendocument.text"/>
  <manifest:file-entry manifest:full-path="content.xml" manifest:media-type="text/xml"/>
  <manifest:file-entry manifest:full-path="styles.xml" manifest:media-type="text/xml"/>
  <manifest:file-entry manifest:full-path="meta.xml" manifest:media-type="text/xml"/>
</manifest:manifest>"""


def create_odt():
    body = build_content()
    content_xml = CONTENT_NS + body + "</office:text></office:body></office:document-content>"

    OUT.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(OUT, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr(zipfile.ZipInfo("mimetype"), "application/vnd.oasis.opendocument.text",
                   compress_type=zipfile.ZIP_STORED)
        z.writestr("META-INF/manifest.xml", MANIFEST_XML)
        z.writestr("meta.xml",              META_XML)
        z.writestr("styles.xml",            STYLES_XML)
        z.writestr("content.xml",           content_xml)

    print(f"Erstellt: {OUT}  ({OUT.stat().st_size // 1024} KB)")


if __name__ == "__main__":
    create_odt()
