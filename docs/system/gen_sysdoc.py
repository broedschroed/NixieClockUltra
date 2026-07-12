#!/usr/bin/env python3
"""Generiert NixieClockUltra-Systemdokumentation.odt"""

import zipfile, base64, os, struct, zlib
from pathlib import Path

WD   = Path("/home/andreas/Dokumente/_Develop/Arduino/NixieClockUltra")
IMGS = WD / "docs/system/assets"
OUT  = WD / "docs/system/NixieClockUltra-Systemdokumentation.odt"

# Kollektion aller verwendeten Spaltenbreiten (für Auto-Style-Generierung)
_used_col_widths: set = set()

MAX_IMG_W_CM = 15.5   # maximale Bildbreite auf der Seite

# Mapping logischer Bildname → tatsächlicher Dateipfad; wird in create_odt() gefüllt
_img_paths: dict = {}

def _png_size(path: Path):
    """Liest Breite und Höhe eines PNG aus dem IHDR-Chunk (ohne Pillow)."""
    with open(path, "rb") as f:
        f.read(8)           # PNG-Signatur
        f.read(4)           # IHDR-Länge
        f.read(4)           # "IHDR"
        w = struct.unpack(">I", f.read(4))[0]
        h = struct.unpack(">I", f.read(4))[0]
    return w, h

# ─── XML-Hilfsfunktionen ────────────────────────────────────────────────────

def esc(s):
    return str(s).replace("&","&amp;").replace("<","&lt;").replace(">","&gt;")

def h1(text, page_break=True):
    pb = '<text:p text:style-name="PageBreak"/>' if page_break else ""
    return f'{pb}<text:h text:style-name="Heading_1" text:outline-level="1">{esc(text)}</text:h>\n'

def h2(text):
    return f'<text:h text:style-name="Heading_2" text:outline-level="2">{esc(text)}</text:h>\n'

def h3(text):
    return f'<text:h text:style-name="Heading_3" text:outline-level="3">{esc(text)}</text:h>\n'

def p(text="", style="Text_Body"):
    return f'<text:p text:style-name="{style}">{esc(text)}</text:p>\n'

def br():
    return '<text:p text:style-name="Text_Body"/>\n'

def caption(text):
    return f'<text:p text:style-name="Caption"><text:span text:style-name="Italic">{esc(text)}</text:span></text:p>\n'

def code_block(*lines):
    return "".join(f'<text:p text:style-name="Code">{esc(l)}</text:p>\n' for l in lines)

def note(text):
    return f'<text:p text:style-name="Note">&#x26A0; {esc(text)}</text:p>\n'

def img(name, cap):
    """Bettet ein PNG ein. Breite = MAX_IMG_W_CM, Höhe aus echtem Seitenverhältnis."""
    path = _img_paths.get(f"{name}.png", IMGS / f"{name}.png")
    pw, ph = _png_size(path)
    w_cm = MAX_IMG_W_CM
    h_cm = round(w_cm * ph / pw, 2)
    return (
        f'<text:p text:style-name="Text_Body">'
        f'<draw:frame draw:name="{name}" draw:style-name="Graphics" '
        f'svg:width="{w_cm}cm" svg:height="{h_cm}cm" '
        f'text:anchor-type="paragraph" draw:z-index="0">'
        f'<draw:image xlink:href="Pictures/{name}.png" '
        f'xlink:type="simple" xlink:show="embed" xlink:actuate="onLoad"/>'
        f'</draw:frame></text:p>\n'
        + caption(cap)
        + br()
    )

# ─── Tabellen ────────────────────────────────────────────────────────────────

def table(headers, rows, col_widths=None):
    global _used_col_widths
    n = len(headers)
    if col_widths is None:
        col_widths = [16.0 / n] * n
    # Spaltenbreiten merken für Auto-Style-Generierung
    for w in col_widths:
        _used_col_widths.add(round(w, 2))
    out = '<table:table table:style-name="TableStyle">\n'
    for w in col_widths:
        sn = f"Col{str(round(w,2)).replace('.','p')}"
        out += f'<table:table-column table:style-name="{sn}"/>\n'
    # header row
    out += '<table:table-row>\n'
    for h in headers:
        out += (f'<table:table-cell table:style-name="TH">'
                f'<text:p text:style-name="TableH">{esc(h)}</text:p>'
                f'</table:table-cell>\n')
    out += '</table:table-row>\n'
    # data rows
    for i, row in enumerate(rows):
        style = "TD_Alt" if i % 2 else "TD"
        out += '<table:table-row>\n'
        for cell in row:
            out += (f'<table:table-cell table:style-name="{style}">'
                    f'<text:p text:style-name="TableP">{esc(cell)}</text:p>'
                    f'</table:table-cell>\n')
        out += '</table:table-row>\n'
    out += '</table:table>\n'
    return out + br()

# ─── Styles XML ─────────────────────────────────────────────────────────────

STYLES_XML = """\
<?xml version="1.0" encoding="UTF-8"?>
<office:document-styles
  xmlns:office="urn:oasis:names:tc:opendocument:xmlns:office:1.0"
  xmlns:style="urn:oasis:names:tc:opendocument:xmlns:style:1.0"
  xmlns:text="urn:oasis:names:tc:opendocument:xmlns:text:1.0"
  xmlns:fo="urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0"
  xmlns:svg="urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0"
  xmlns:draw="urn:oasis:names:tc:opendocument:xmlns:drawing:1.0"
  xmlns:table="urn:oasis:names:tc:opendocument:xmlns:table:1.0"
  office:version="1.3">

<office:styles>
  <!-- Seitenlayout A4 -->
  <style:style style:name="PageLayout" style:family="page-layout">
    <style:page-layout-properties fo:page-width="21cm" fo:page-height="29.7cm"
      fo:margin-top="2.5cm" fo:margin-bottom="2.5cm"
      fo:margin-left="2.5cm" fo:margin-right="2.5cm"/>
  </style:style>

  <!-- Absatzstile -->
  <style:style style:name="Title" style:family="paragraph">
    <style:paragraph-properties fo:text-align="center" fo:margin-top="6cm" fo:margin-bottom="0.5cm"/>
    <style:text-properties fo:font-size="28pt" fo:font-weight="bold" fo:color="#1a2440"/>
  </style:style>

  <style:style style:name="Subtitle" style:family="paragraph">
    <style:paragraph-properties fo:text-align="center" fo:margin-bottom="0.4cm"/>
    <style:text-properties fo:font-size="14pt" fo:color="#444444"/>
  </style:style>

  <style:style style:name="TitleMeta" style:family="paragraph">
    <style:paragraph-properties fo:text-align="center" fo:margin-bottom="0.2cm"/>
    <style:text-properties fo:font-size="11pt" fo:color="#666666"/>
  </style:style>

  <style:style style:name="Heading_1" style:family="paragraph">
    <style:paragraph-properties fo:margin-top="0.8cm" fo:margin-bottom="0.4cm"
      fo:keep-with-next="always"/>
    <style:text-properties fo:font-size="18pt" fo:font-weight="bold" fo:color="#1a2440"/>
  </style:style>

  <style:style style:name="Heading_2" style:family="paragraph">
    <style:paragraph-properties fo:margin-top="0.6cm" fo:margin-bottom="0.3cm"
      fo:keep-with-next="always"/>
    <style:text-properties fo:font-size="14pt" fo:font-weight="bold" fo:color="#1a3060"/>
  </style:style>

  <style:style style:name="Heading_3" style:family="paragraph">
    <style:paragraph-properties fo:margin-top="0.4cm" fo:margin-bottom="0.2cm"
      fo:keep-with-next="always"/>
    <style:text-properties fo:font-size="12pt" fo:font-weight="bold" fo:color="#2a4080"/>
  </style:style>

  <style:style style:name="Text_Body" style:family="paragraph">
    <style:paragraph-properties fo:margin-top="0.1cm" fo:margin-bottom="0.2cm"
      fo:text-align="justify" fo:line-height="140%"/>
    <style:text-properties fo:font-size="10.5pt"/>
  </style:style>

  <style:style style:name="Caption" style:family="paragraph">
    <style:paragraph-properties fo:text-align="center" fo:margin-bottom="0.3cm"
      fo:margin-top="0.1cm"/>
    <style:text-properties fo:font-size="9pt" fo:color="#555555"/>
  </style:style>

  <style:style style:name="Code" style:family="paragraph">
    <style:paragraph-properties fo:margin-left="1cm" fo:margin-top="0.05cm"
      fo:margin-bottom="0.05cm" fo:background-color="#f3f3f3"/>
    <style:text-properties fo:font-family="Liberation Mono" fo:font-size="9pt"
      fo:color="#1a1a1a"/>
  </style:style>

  <style:style style:name="Note" style:family="paragraph">
    <style:paragraph-properties fo:margin-top="0.2cm" fo:margin-bottom="0.2cm"
      fo:margin-left="0.5cm" fo:background-color="#fff8e0" fo:border="0.5pt solid #c8a000"
      fo:padding="0.2cm"/>
    <style:text-properties fo:font-size="10pt" fo:color="#5a4000"/>
  </style:style>

  <style:style style:name="PageBreak" style:family="paragraph">
    <style:paragraph-properties fo:break-after="page"/>
    <style:text-properties fo:font-size="1pt"/>
  </style:style>

  <style:style style:name="TableH" style:family="paragraph">
    <style:text-properties fo:font-size="10pt" fo:font-weight="bold" fo:color="#ffffff"/>
  </style:style>

  <style:style style:name="TableP" style:family="paragraph">
    <style:text-properties fo:font-size="9.5pt"/>
  </style:style>

  <style:style style:name="Italic" style:family="text">
    <style:text-properties fo:font-style="italic"/>
  </style:style>

  <style:style style:name="Graphics" style:family="graphic">
    <style:graphic-properties fo:margin-top="0.3cm" fo:margin-bottom="0.1cm"/>
  </style:style>
</office:styles>

<office:master-styles>
  <style:master-page style:name="Standard" style:page-layout-name="PageLayout">
    <style:footer>
      <text:p text:style-name="TitleMeta">
        Nixie Clock Ultra — Systemdokumentation
        <text:tab/><text:tab/><text:tab/>
        Seite <text:page-number/>
      </text:p>
    </style:footer>
  </style:master-page>
</office:master-styles>
</office:document-styles>
"""

# ─── Dokumentinhalt ──────────────────────────────────────────────────────────

def build_content():
    c = []

    # ── Titelseite ─────────────────────────────────────────────────────────
    c.append(p("Nixie Clock Ultra", "Title"))
    c.append(p("Systemdokumentation", "Subtitle"))
    c.append(br()); c.append(br())
    c.append(p("Version 2.1 · ESP32-S3 · 6 × IN-12A Nixie", "TitleMeta"))
    c.append(p("MCP23017 Direct Drive · DS1302 RTC · WS2812B NeoPixel", "TitleMeta"))
    c.append(br())
    c.append(p("Stand: Juni 2026", "TitleMeta"))
    c.append(p("Projekt: broed digital media", "TitleMeta"))
    c.append('<text:p text:style-name="PageBreak"/>\n')

    # ── Kapitel 1: Systemübersicht ─────────────────────────────────────────
    c.append(h1("Systemübersicht", page_break=False))
    c.append(p("Die Nixie Clock Ultra ist eine Röhrenuhr auf Basis des ESP32-S3 "
               "Mikrocontrollers mit 6 IN-12A Nixie-Röhren. Das System besteht aus "
               "zwei Platinen: dem Logic Board, das den Mikrocontroller, die "
               "Echtzeituhr, die Hochspannungserzeugung und alle Schnittstellen "
               "beherbergt, sowie dem Nixie Display Board, das die Anzeigeelektronik "
               "mit vier MCP23017 I²C-Port-Expandern, 60 NPN-Transistoren und den "
               "WS2812B RGB-LEDs enthält."))
    c.append(p("Die Röhren werden ohne Multiplexing direkt angesteuert — jede der "
               "60 Kathoden besitzt einen eigenen SMBTA42-Hochvolt-Transistor. "
               "Ghosting-Effekte sind dadurch vollständig eliminiert."))
    c.append(br())

    c.append(h2("Zweiplatinenaufbau"))
    c.append(table(
        ["Platine", "KiCAD-Projekt", "Rev.", "Hauptfunktion"],
        [
            ["Logic Board", "nixieclocklogic_V2-1", "2.1", "ESP32-S3, DS1302 RTC, DC-DC 5V→10V, HV-MOD, LDR, Taster, IR, USB"],
            ["Nixie Display Board", "nixieclockin12_V2", "2.01", "4× MCP23017, 60× SMBTA42, 6× IN-12A, WS2812B"],
        ],
        [3.2, 4.5, 1.2, 7.1]
    ))

    c.append(h2("Inter-Board-Steckverbinder"))
    c.append(p("Die beiden Platinen sind über zwei Steckverbinder verbunden. "
               "J3 (Logic Board) ↔ J1 (Display Board) führt Logik- und Steuersignale, "
               "J4 (Logic Board) ↔ J2 (Display Board) die Hochspannungsversorgung."))
    c.append(table(
        ["Steckverbinder", "Polig", "Signale"],
        [
            ["J3 ↔ J1  (Logic)", "8", "VCC 3,3V, GND, SDA (GPIO8), SCL (GPIO9), NEO-Daten (GPIO21)"],
            ["J4 ↔ J2  (HV)", "4", "HV ~170V DC (×2), HVgnd (×2)"],
        ],
        [3.0, 1.5, 11.5]
    ))

    # ── Kapitel 2: PCB 1 – Logic Board ─────────────────────────────────────
    c.append(h1("PCB 1 – Logic Board"))
    c.append(p("Das Logic Board (Rev 2.1, KiCAD: nixieclocklogic_V2-1) trägt alle "
               "zentralen Komponenten des Systems: den ESP32-S3-WROOM-1 Mikrocontroller "
               "(U1), die DS1302 Echtzeituhr (U2), den VS1838B IR-Empfänger (U3), "
               "das HV-Boost-Modul (U4, 10 V → ~170 V), den AMS1117-3.3 LDO-Regler "
               "(U5, 5 V → 3,3 V), den DC-DC-Boost-Konverter (U6, 5 V → 10 V), "
               "den USB-ESD-Schutz (U22), einen LDR-Helligkeitssensor (J5), "
               "vier Taster sowie den Micro-USB-Anschluss (J2)."))
    c.append(p("Gegenüber Rev 2.0 wurden zwei Änderungen vorgenommen: Das HV-Modul "
               "benötigt mindestens 10 V Eingangsspannung, um stabile 170 V für die "
               "IN-12A-Röhren zu liefern — direkt aus USB-5 V war dies nicht "
               "erreichbar. Daher wurde ein DC-DC-Konverter (5 V → 10 V) ergänzt. "
               "Außerdem wurde der LDR-Helligkeitssensor (J5) neu bestückt, der in "
               "Rev 2.0 noch nicht vorhanden war."))

    c.append(h2("Schaltplan"))
    c.append(img("logic_sch", "Abb. 1 – Logic Board Rev 2.1 Schaltplan (nixieclocklogic_V2-1)"))

    c.append(h2("PCB-Layout"))
    c.append(img("logic_pcb_top", "Abb. 2a – Logic Board Rev 2.1 PCB-Layout Oberseite"))
    if (IMGS / "logic_pcb-2.png").exists():
        c.append(img("logic_pcb_bot", "Abb. 2b – Logic Board Rev 2.1 PCB-Layout Unterseite"))

    c.append(h2("Komponenten"))
    c.append(table(
        ["Ref.", "Bauteil", "Gehäuse", "Funktion"],
        [
            ["U1",  "ESP32-S3-WROOM-1",   "Modul",      "Mikrocontroller, WiFi/BT, 4 MB Flash"],
            ["U2",  "DS1302N+",            "DIP-8",      "Batteriegepufferte Echtzeituhr (ThreeWire)"],
            ["U3",  "VS1838B",             "TO-92",      "IR-Empfänger/-Demodulator 38 kHz"],
            ["U4",  "HV-MOD",              "Custom",     "Boost-Converter 10 V → ~170 V DC für Nixie-Anoden"],
            ["U5",  "AMS1117-3.3",         "SOT-223",    "LDO-Linearregler 5 V → 3,3 V"],
            ["U6",  "DC-DC-MOD",           "Custom",     "Boost-Konverter 5 V → 10 V (Vorstufe für HV-Modul)"],
            ["–",   "TLP627",              "DIP-4",      "Optokoppler schaltet Anodenspannung per Hardware-PWM (Nacht-Modus-Dimmung); aktuell handverdrahtet, noch nicht mit eigener Referenz im Schaltplan"],
            ["U22", "USBLC6-2SC6",         "SOT-23-6",   "USB-ESD-/TVS-Schutz"],
            ["BT1", "CR2032",              "SMD-Halter", "RTC-Backup-Batterie (~3 V)"],
            ["Y1",  "32,768 kHz Quarz",    "Radial",     "RTC-Taktquelle"],
            ["SW1–4","Taster SET/UP/DOWN/LIGHT","SMD",   "Bedienpanel"],
            ["SW5", "BOOT",                "SMD",        "ESP32-Bootmodus (Programmierung)"],
            ["J2",  "USB Micro-B",         "Custom",     "5 V-Einspeisung & Firmware-Upload"],
            ["J3",  "PinHeader 1×8",       "2,54 mm",    "Inter-Board Logik-Signale"],
            ["J4",  "PinHeader 1×4",       "2,54 mm",    "Inter-Board HV-Versorgung"],
            ["J5",  "PinHeader 1×2",       "2,54 mm",    "LDR-Helligkeitssensor (neu in Rev 2.1): LDR→VCC, 100 kΩ→GND, Messung GPIO6"],
        ],
        [1.2, 3.5, 2.5, 8.8]
    ))

    c.append(h2("Spannungsversorgungskonzept"))
    c.append(p("Die Versorgung erfolgt über den Micro-USB-Anschluss J2 (5 V). "
               "Aus der 5-V-Schiene erzeugen drei Wandler die benötigten Spannungen:"))
    c.append(h3("3,3-V-Logikversorgung (AMS1117-3.3)"))
    c.append(p("Der AMS1117-3.3 (U5) ist ein Low-Dropout-Regler im SOT-223-Gehäuse. "
               "Er versorgt den ESP32-S3, den DS1302, den IR-Empfänger sowie alle "
               "Pull-up-Widerstände. Abblockkondensatoren (100 nF + 100 µF) "
               "stabilisieren die Ausgangsspannung."))
    c.append(h3("10-V-Zwischenstufe (DC-DC-MOD)"))
    c.append(p("Das HV-Boost-Modul (U4) benötigt eine Eingangsspannung von mindestens "
               "10 V, um zuverlässig ~170 V für die IN-12A-Röhren zu erzeugen — die "
               "USB-Einspannung von 5 V reicht dafür nicht aus. Der DC-DC-Konverter "
               "(U6) hebt die 5-V-USB-Schiene auf stabile 10 V an. "
               "Diese 10-V-Schiene speist ausschließlich das HV-Modul."))
    c.append(h3("Hochspannung ~170 V DC (HV-MOD)"))
    c.append(p("Der HV-MOD (U4) ist ein Boost-Converter-Modul, das aus 10 V eine "
               "geregelte Gleichspannung von ca. 170–180 V erzeugt. Diese Spannung "
               "wird auf dem Logic Board über einen TLP627-Optokoppler geführt, bevor "
               "sie über J4 an die Anoden aller sechs Nixie-Röhren gelangt. "
               "Die Kathoden der jeweils anzuzeigenden Ziffer werden durch "
               "NPN-Transistoren auf dem Display Board auf GND gezogen."))
    c.append(h3("Anoden-Dimmung (TLP627, Hardware-PWM)"))
    c.append(p("Der TLP627-Optokoppler schaltet die vom HV-MOD kommende Anodenspannung "
               "per Hardware-PWM (LEDC, ~200 Hz) auf GPIO7 (HV_SWITCH_PIN). Im "
               "Nacht-Modus reduziert die Firmware den Duty-Cycle auf einen "
               "einstellbaren Wert zwischen 2 % und 60 % (Standard 25 %, siehe "
               "hv_dimmer.ino in firmware.md) statt wie zuvor die Kathoden per "
               "Software-PWM zu takten."))
    c.append(note("Hochspannungswarnung: Der HV-Ausgang führt ~170 V DC — "
                  "lebensgefährlich bei Berührung. Vor Arbeiten an der Schaltung "
                  "die Versorgung trennen und Kondensatoren entladen."))

    c.append(h2("ESP32-S3 GPIO-Belegung"))
    c.append(table(
        ["GPIO", "Signal", "Richtung", "Funktion"],
        [
            ["2",  "RTC_CE",    "Output", "DS1302 Chip Enable (ThreeWire)"],
            ["4",  "RTC_IO",    "Bi-Dir.", "DS1302 Datenleitung (ThreeWire)"],
            ["5",  "RTC_CLK",   "Output", "DS1302 Takt (ThreeWire)"],
            ["6",  "LDR_ADC",   "Input",  "LDR-Helligkeitssensor (ADC1, LDR→VCC, 100 kΩ→GND)"],
            ["7",  "HV_SWITCH", "Output", "TLP627-Optokoppler: Hardware-PWM (~200 Hz, LEDC) schaltet Anodenspannung"],
            ["8",  "I2C_SDA",   "Bi-Dir.", "I²C Daten → 4× MCP23017"],
            ["9",  "I2C_SCL",   "Output", "I²C Takt → 4× MCP23017"],
            ["10", "BTN_LIGHT", "Input",  "Taster LIGHT (INPUT_PULLUP, aktiv LOW)"],
            ["11", "BTN_DOWN",  "Input",  "Taster DOWN (INPUT_PULLUP, aktiv LOW)"],
            ["12", "BTN_UP",    "Input",  "Taster UP (INPUT_PULLUP, aktiv LOW)"],
            ["13", "BTN_SET",   "Input",  "Taster SET (INPUT_PULLUP, aktiv LOW)"],
            ["21", "NEO_DATA",  "Output", "WS2812B DIN (10 LEDs, verkettete Kette)"],
            ["48", "IR_RECV",   "Input",  "VS1838B demodulierter 38-kHz-Ausgang"],
        ],
        [1.2, 2.5, 2.0, 10.3]
    ))

    c.append(h2("Echtzeituhr DS1302"))
    c.append(p("Der DS1302 (U2) ist eine batteriegepufferte Echtzeituhr mit "
               "ThreeWire-Interface (kein I²C/SPI-Standard). Die Kommunikation "
               "erfolgt über drei GPIOs: CE (Chip Enable, GPIO2), IO (Daten, GPIO4) "
               "und CLK (Takt, GPIO5). Der 32,768-kHz-Quarz Y1 liefert die Zeitbasis. "
               "Die CR2032-Backup-Batterie (BT1) hält die Uhrzeit auch bei "
               "Stromausfall. In der Firmware wird die Bibliothek «Rtc by Makuna» "
               "verwendet (Klasse RtcDS1302)."))

    c.append(h2("IR-Empfänger VS1838B"))
    c.append(p("Der VS1838B (U3) ist ein integrierter IR-Empfänger mit eingebautem "
               "38-kHz-Bandpassfilter und Demodulator. Er liefert am Ausgang (GPIO48) "
               "ein NF-Signal, das direkt von der IRremoteESP8266-Bibliothek "
               "dekodiert wird. Unterstützte Protokolle: NEC, Samsung, Sony, RC5, "
               "RC6 u.a. Alle empfangenen Codes werden gegen die im NVS gespeicherten "
               "angelernten Codes verglichen."))

    # ── Kapitel 3: PCB 2 – Nixie Display Board ─────────────────────────────
    c.append(h1("PCB 2 – Nixie Display Board"))
    c.append(p("Das Nixie Display Board (Rev 2.01, 2026-04-06) enthält die gesamte "
               "Anzeigeelektronik. Vier MCP23017 I²C-GPIO-Expander stellen 64 digitale "
               "Ausgänge bereit, von denen 60 jeweils einen SMBTA42-NPN-Transistor "
               "ansteuern. Jeder Transistor schaltet eine Nixie-Kathode auf GND. "
               "Sechs WS2812B-SMD-LEDs (Pixel 0–5, GRB) liefern die Röhrenhintergrundbeleuchtung; "
               "vier WS2812B-THT-LEDs YF923 (Pixel 6–9, RGB) dienen als Trennpunkt-LEDs "
               "zwischen den Zifferngruppen."))

    c.append(h2("Schaltplan"))
    c.append(img("nixie_sch", "Abb. 3 – Nixie Display Board Rev 2.01 Schaltplan"))

    c.append(h2("PCB-Layout"))
    c.append(img("nixie_pcb_top", "Abb. 4a – Nixie Display Board Rev 2.01 PCB-Layout Oberseite"))
    if (IMGS / "nixie_pcb-2.png").exists():
        c.append(img("nixie_pcb_bot", "Abb. 4b – Nixie Display Board Rev 2.01 PCB-Layout Unterseite"))

    c.append(h2("Komponenten"))
    c.append(table(
        ["Ref.", "Bauteil", "Anzahl", "Funktion"],
        [
            ["U1–U4",   "MCP23017-E/SO (SOIC-28)", "4",  "16-bit I²C GPIO-Expander, je 16 Ausgänge"],
            ["Q1–Q60",  "SMBTA42 (TSOT-23)",        "60", "NPN 300 V Hochvolt-Transistor als Kathodenschalter"],
            ["NX1–NX6", "IN-12A",                   "6",  "Numerische Nixie-Röhre, Aufsicht, 10 Kathoden (0–9)"],
            ["D1–D6",   "WS2812B-SMD",               "6",  "Röhrenhintergrundbeleuchtung, Pixel 0–5 (GRB)"],
            ["D7–D10",  "WS2812B-THT (YF923)",       "4",  "Trennpunkt-LEDs, Pixel 6–9 (RGB-Farbreihenfolge)"],
            ["R1–R64",  "3,3 kΩ (0805)",             "64", "Basis-Vorwiderstände für SMBTA42"],
            ["R65–R70", "10 kΩ (THT axial)",         "6",  "I²C-Bus-Pull-ups + MCP-Adress-Pull-ups"],
            ["C1–C4",   "100 nF (0805)",              "4",  "Abblockkondensatoren je MCP23017 (VCC gegen GND)"],
            ["J1",      "PinHeader 1×8 (2,54 mm)",   "1",  "Inter-Board Logik-Signale ← Logic Board J3"],
            ["J2",      "PinHeader 1×4 (2,54 mm)",   "1",  "Inter-Board HV-Versorgung ← Logic Board J4"],
        ],
        [1.8, 4.5, 1.5, 8.2]
    ))

    c.append(h2("MCP23017 Ansteuerungskonzept"))
    c.append(p("Die vier MCP23017 (U1–U4) sind am gemeinsamen I²C-Bus "
               "(SDA=GPIO8, SCL=GPIO9) angeschlossen. Die I²C-Adressen werden "
               "durch die Hardware-Adresspins A0, A1, A2 festgelegt:"))
    c.append(table(
        ["IC", "I²C-Adresse", "A2", "A1", "A0", "Zuständige Röhre(n)"],
        [
            ["U1", "0x20", "0", "0", "0", "Tube 0 (Stundenzehner), Tube 1 Bits 10–15"],
            ["U2", "0x21", "0", "0", "1", "Tube 1 Bits 0–3, Tube 2 (Minutenzehner)"],
            ["U3", "0x22", "0", "1", "0", "Tube 3 (Minuteneiner), Tube 4 Bits 10–15"],
            ["U4", "0x23", "0", "1", "1", "Tube 4 Bits 0–3, Tube 5 (Sekundeneiner)"],
        ],
        [1.0, 2.5, 1.0, 1.0, 1.0, 9.5]
    ))
    c.append(p("Jeder MCP23017 verfügt über zwei 8-Bit-Ports (GPA0–7, GPB0–7), "
               "also 16 Ausgänge pro IC. Alle 64 Ausgänge werden beim Start im "
               "`nixieInit()`-Aufruf als Output konfiguriert (IODIR-Register = 0x00). "
               "Die Ansteuerung erfolgt über das OLATA-Register (0x14) mit einem "
               "einzigen I²C-Schreibzugriff, der beide Ports gleichzeitig setzt "
               "(16 Bits, Low-Byte GPA, High-Byte GPB)."))

    c.append(h3("Tube-zu-MCP-Bit-Zuordnung"))
    c.append(p("Die Verteilung der 60 Kathoden (6 Röhren × 10 Ziffern) auf die "
               "MCP-Bits ist in der Firmware als Lookup-Tabelle `digitPin[6][10]` "
               "implementiert (Struktur `DigitPin {uint8_t mcp; uint8_t bit;}`):"))
    c.append(table(
        ["Tube", "Anzeige", "MCP", "GPA/GPB-Bits"],
        [
            ["0", "Stundenzehner",  "0x20", "Bits 0–9 (Ziffern 1,2,3,4,5,6,7,8,9,0)"],
            ["1", "Stundeneiner",   "0x20+0x21", "MCP0 Bits 10–15 + MCP1 Bits 0–3"],
            ["2", "Minutenzehner",  "0x21", "Bits 4–13"],
            ["3", "Minuteneiner",   "0x22", "Bits 0–9"],
            ["4", "Sekundenzehner", "0x22+0x23", "MCP2 Bits 10–15 + MCP3 Bits 0–3"],
            ["5", "Sekundeneiner",  "0x23", "Bits 4–13"],
        ],
        [1.0, 3.5, 2.5, 9.0]
    ))
    c.append(p("Ein Digit-Wert von 10 bedeutet «Röhre blank» — es wird kein Bit "
               "gesetzt, alle Kathoden bleiben HIGH, die Röhre zeigt nichts an."))

    c.append(h2("Transistor-Kathodenschaltung"))
    c.append(p("Jede Nixie-Kathode wird über einen SMBTA42-NPN-Transistor auf GND "
               "gezogen. Der SMBTA42 ist für 300 V Kollektor-Emitter-Spannung "
               "ausgelegt — notwendig, da die Nixie-Anoden auf ~170 V liegen. "
               "Der Basis-Vorwiderstand beträgt 3,3 kΩ. Bei einem GPIO-High von "
               "3,3 V fließt ein Basisstrom von ca. 1 mA, was den Transistor sicher "
               "in die Sättigung treibt (typischer Verstärkungsfaktor hFE ≥ 100). "
               "Der Kollektor-Strom (Katodenleuchtstrom) beträgt typisch 1–3 mA, "
               "begrenzt durch den internen Widerstand der Nixie-Röhre."))

    c.append(h2("WS2812B NeoPixel-Konfiguration"))
    c.append(p("Die 10 WS2812B-LEDs sind als verkettete Kette an GPIO21 angeschlossen. "
               "SMD- und THT-Varianten haben unterschiedliche Farbreihenfolgen, "
               "was in der Firmware berücksichtigt wird:"))
    c.append(table(
        ["Pixel", "Typ", "Farbreihenfolge", "Position / Funktion"],
        [
            ["0–5", "WS2812B-SMD",       "GRB", "Röhrenhintergrundbeleuchtung (6 LEDs)"],
            ["6–9", "WS2812B-THT YF923", "RGB", "Trennpunkt-LEDs zwischen Zifferngruppen (4 LEDs)"],
        ],
        [1.5, 3.5, 3.5, 7.5]
    ))

    # ── Kapitel 4: Firmware-Architektur ─────────────────────────────────────
    c.append(h1("Firmware-Architektur"))
    c.append(p("Die Firmware ist als Arduino-Sketch für den ESP32-S3 implementiert "
               "und in mehrere `.ino`-Module aufgeteilt. Arduino kompiliert alle "
               "`.ino`-Dateien eines Verzeichnisses automatisch zu einer einzigen "
               "Compilation Unit — globale Variablen und Funktionen sind "
               "dateienübergreifend sichtbar."))

    c.append(h2("Modulübersicht"))
    c.append(table(
        ["Datei", "Zeilen", "Inhalt / Verantwortlichkeit"],
        [
            ["NixieClockUltra.ino", "471", "Globals, setup(), loop(), Edit-Mode-FSM (Zeit+Datum), Nacht-Modus-Globals"],
            ["nixie_driver.ino",    "91",  "nixieInit(), nixieWrite(), MCP23017-I²C-Abstraktion, Shadow-Register, Mutex"],
            ["display.ino",         "88",  "setDisplayTime(), setDisplayDate(), commitDigits() (weicher Ziffernwechsel), Slot-Animation"],
            ["digit_fade.ino",      "77",  "startDigitFade(), updateDigitFade(), cancelDigitFade() — non-blocking Crossfade über HV-Dimmer-Duty"],
            ["buttons.ino",         "127", "Entprell-FSM für 4 Taster, Kurz-/Langdruck, Edit-Mode Zeit+Datum"],
            ["rtc.ino",             "18",  "readRTC(), writeRTC() via DS1302/ThreeWire, liest auch Tag/Monat/Jahr"],
            ["night_mode.ino",      "34",  "LDR-Abtastung (GPIO6, ADC1), updateNightMode(), Zeitbereich-Logik"],
            ["hv_dimmer.ino",       "12",  "hvDimmerInit(), hvDimmerSetDuty() — LEDC-Hardware-PWM für TLP627 auf Anodenspannung"],
            ["neo_animation.ino",   "107", "Rainbow, Statisch, Puls, Slot, Nacht-Modus-Dimming, Datumsanzeige-Override"],
            ["ir_remote.ino",       "107", "executeAction(), dispatchIRAction(), handleIR(), 8 IR-Aktionen"],
            ["web_server.ino",      "748", "Eingebettetes HTML/JS, alle API-Handler, WiFi-Setup, NTP, mDNS"],
        ],
        [4.5, 1.5, 10.0]
    ))
    c.append(p("Reine Interpolationsmathematik für den Crossfade liegt zusätzlich in "
               "digit_fade_math.h (header-only, host-testbar ohne Arduino-Framework, "
               "siehe test/digit_fade_math_test.cpp)."))

    c.append(h2("Bibliotheksabhängigkeiten"))
    c.append(table(
        ["Bibliothek", "Autor", "Version", "Funktion"],
        [
            ["Adafruit NeoPixel",  "Adafruit",          "aktuell", "WS2812B-Ansteuerung (GRB + RGB)"],
            ["Rtc by Makuna",      "Michael C. Miller",  "aktuell", "DS1302 RTC über ThreeWire-Protokoll"],
            ["AsyncTCP",           "me-no-dev",          "aktuell", "Async-TCP-Stack für ESP32"],
            ["ESPAsyncWebServer",  "me-no-dev",          "aktuell", "Nicht-blockierender HTTP-Server"],
            ["ArduinoJson",        "Benoit Blanchon",    "v6.x",    "JSON-Serialisierung (v7 inkompatibel!)"],
            ["IRremoteESP8266",    "David Conran",       "aktuell", "IR-Empfang NEC/Samsung/Sony/RC5/RC6 …"],
        ],
        [4.0, 4.0, 2.0, 6.0]
    ))
    c.append(note("ArduinoJson muss in Version 6.x installiert sein — "
                  "Version 7 hat eine inkompatible API."))

    c.append(h2("Setup-Ablauf"))
    c.append(p("Die setup()-Funktion wird einmalig nach dem Einschalten ausgeführt:"))
    c.append(table(
        ["Schritt", "Funktion / Aktion", "Details"],
        [
            ["1",  "Serial.begin(115200)",        "Debug-Ausgabe aktivieren"],
            ["2",  "pinMode(BTN_*, INPUT_PULLUP)", "Alle 4 Taster-GPIOs konfigurieren (IO10–IO13)"],
            ["3",  "strip.begin() / clear()",      "NeoPixel initialisieren, alle LEDs aus"],
            ["4",  "hvDimmerInit()",               "LEDC-PWM auf HV_SWITCH_PIN (GPIO7) anlegen, Duty zunächst 255 (volle Anodenspannung)"],
            ["5",  "prefs.begin(\"nixie\")",       "NVS öffnen: Helligkeit, Anim, SlotIval, IR-Codes, WiFi, Nacht-Modus-Konfiguration inkl. hvDimPct, sowie softFadeSecondEnabled/softFadeDateEnabled/slotSpeedPct laden"],
            ["6",  "nixieInit()",                  "I²C starten, alle 4 MCP23017 auf Output, alle Bits 0"],
            ["7",  "Rtc.Begin() + readRTC()",      "DS1302 starten, Uhrzeit + Datum in curHour/Min/Sec/Day/Month/Year laden"],
            ["8",  "setupWifi()",                  "AP starten (SSID NixieClockCS); STA verbinden + NTP; mDNS als nixieclockcs.local"],
            ["9",  "setupWebServer()",             "Alle API-Routen registrieren, server.begin()"],
            ["10", "irrecv.enableIRIn()",          "IR-Empfänger aktivieren (GPIO48)"],
            ["11", "startFadeIn = true",           "Fade-In-Flag setzen → Röhren blenden in loop() ein"],
        ],
        [1.2, 4.5, 10.3]
    ))

    c.append(h2("Globale State-Variablen"))
    c.append(table(
        ["Variable", "Typ", "Beschreibung"],
        [
            ["displayDigits[6]",  "uint8_t[6]",  "Aktuell angezeigte Ziffern (0–9; 10 = blank)"],
            ["brightLevel",        "uint8_t",     "Helligkeitsstufe 0–3 (Index in BRIGHTNESS_LEVELS[])"],
            ["neoBright",          "uint8_t",     "NeoPixel-Feinwert 10–255 (Hintergrund Pixel 0–5)"],
            ["colonBright",        "uint8_t",     "NeoPixel-Feinwert (Trennpunkte Pixel 6–9)"],
            ["neoHue",             "uint8_t",     "Auto-inkrement für Rainbow-Farbverlauf"],
            ["animMode",           "AnimMode",    "Aktueller Animationsmodus (Enum, s. u.)"],
            ["editState",          "EditState",   "Aktueller Einstellschritt (NONE/HOUR/MIN/SEC/DAY/MONTH/YEAR)"],
            ["colonAlwaysOn",      "bool",        "Trennpunkte dauerhaft an (kein Blinken)"],
            ["colonStatic",        "bool",        "Trennpunkte statisch warmweiß"],
            ["curDay/Month/Year",  "uint8_t",     "Lokal gecachtes Datum (Tag 1–31, Monat 1–12, Jahr 0–99)"],
            ["dateShowActive",     "bool",        "Datumsanzeige nach Slot-Animation gerade aktiv"],
            ["dateShowStart",      "uint32_t",    "millis()-Zeitstempel beim Start der Datumsanzeige"],
            ["nightState",         "NightState",  "Aktueller Nacht-Modus-Zustand (NORMAL/DIM/DARK)"],
            ["nightTimeEnabled",   "bool",        "Zeitbereich-Nacht-Modus aktiv"],
            ["nightStart/End",     "uint8_t",     "Start-/Endstunde des Nacht-Bereichs (0–23)"],
            ["nightTimeMode",      "uint8_t",     "0 = Gedimmt, 1 = Dunkel"],
            ["ldrEnabled",         "bool",        "Lichtsensor-Steuerung aktiv"],
            ["ldrThreshold",       "uint16_t",    "ADC-Schwellwert (0–4095); <= Schwellwert → Dimmen"],
            ["ldrReading",         "uint16_t",    "Aktueller ADC-Messwert (alle 500 ms aktualisiert)"],
            ["hvDimPct",           "uint8_t",     "Röhren-Dimm-Helligkeit im Nacht-Modus in % (2–60, Default 25)"],
            ["irCodes[8]",         "uint64_t[8]", "Angelernte IR-Codes, Index = IrAction-Enum (0–7)"],
            ["wifiStaConnected",   "bool",        "STA-Verbindung zum Heimnetz aktiv"],
            ["ntpSynced",          "bool",        "NTP-Synchronisierung erfolgreich"],
            ["slotActive",         "bool",        "Slot-Machine-Animation läuft"],
            ["startFadeIn",        "bool",        "Einblend-Sequenz beim Start aktiv"],
            ["softFadeSecondEnabled", "bool",     "Weicher Ziffernwechsel im Sekundentakt aktiv"],
            ["softFadeDateEnabled",   "bool",     "Weicher Ziffernwechsel bei Zeit/Datum-Übergang aktiv"],
            ["slotSpeedPct",       "uint8_t",     "Slot-Machine-Rollgeschwindigkeit 20–100 % (100 = Standard)"],
        ],
        [4.0, 2.5, 9.5]
    ))

    c.append(h2("Enumerationen"))
    c.append(code_block(
        "enum AnimMode   { ANIM_RAINBOW, ANIM_STATIC, ANIM_PULSE, ANIM_COUNT };",
        "",
        "enum EditState  { EDIT_NONE,",
        "                  EDIT_HOUR, EDIT_MIN, EDIT_SEC,",
        "                  EDIT_DAY, EDIT_MONTH, EDIT_YEAR };",
        "",
        "enum NightState { NIGHT_NORMAL, NIGHT_DIM, NIGHT_DARK };",
        "",
        "enum IrAction   {",
        "    IR_LEARN_NONE          = -1,",
        "    IR_ACTION_SET          = 0,  IR_ACTION_UP          = 1,",
        "    IR_ACTION_DOWN         = 2,  IR_ACTION_BRIGHTNESS  = 3,",
        "    IR_ACTION_ANIM_NEXT    = 4,  IR_ACTION_SLOT        = 5,",
        "    IR_ACTION_COLON_TOGGLE = 6,  IR_ACTION_DATE        = 7,",
        "    IR_ACTION_COUNT        = 8",
        "};",
    ))

    c.append(h2("hv_dimmer.ino – Hardware-PWM-Dimmung der Anodenspannung"))
    c.append(p("Die Röhren-Dimmung im Nacht-Modus erfolgt über einen TLP627-Optokoppler, "
               "der die Anodenspannung selbst per LEDC-Hardware-PWM (~200 Hz) auf "
               "HV_SWITCH_PIN (GPIO7) schaltet — nicht mehr über eine Software-PWM auf "
               "den Kathoden:"))
    c.append(code_block(
        "void hvDimmerInit() {",
        "    ledcAttach(HV_SWITCH_PIN, HV_PWM_FREQ_HZ, 8);",
        "    ledcWrite(HV_SWITCH_PIN, 255);   // volle Helligkeit (Anode dauerhaft an)",
        "}",
        "",
        "void hvDimmerSetDuty(uint8_t duty0to255) {",
        "    ledcWrite(HV_SWITCH_PIN, duty0to255);",
        "}",
    ))
    c.append(p("loop() ruft hvDimmerSetDuty() nur bei einem Wechsel von nightState auf "
               "(Guard nightState != prevNightState), nicht bei jedem Durchlauf: "
               "NIGHT_NORMAL → Duty 255, NIGHT_DIM → Duty hvDimPct*255/100 (2–60 %), "
               "NIGHT_DARK → Duty 0. Ein separater Blitzschutz für Sekundenwechsel ist "
               "nicht nötig, da die Kathoden-Ansteuerung unabhängig von der "
               "Anodendimmung läuft."))

    c.append(h2("digit_fade.ino – Weicher Ziffernwechsel"))
    c.append(p("commitDigits() in display.ino ist die zentrale Stelle, über die alle "
               "Ziffernänderungen laufen (setDisplayTime(), setDisplayDate() sowie die "
               "Soft-Varianten setDisplayTimeSoft()/setDisplayDateSoft()). Sie vergleicht "
               "per memcmp() gegen displayDigits, um unveränderte Aufrufe zu ignorieren, "
               "und entscheidet dann: fadeMs == 0 oder nightState != NIGHT_NORMAL → "
               "sofortiger Hart-Wechsel über nixieWrite() (im Nacht-Modus kein Fade, da "
               "die Anodenspannung dort bereits gedimmt/aus ist); fadeMs > 0 → "
               "startDigitFade()."))
    c.append(p("startDigitFade()/updateDigitFade() bilden eine non-blocking "
               "State-Machine (angetrieben aus loop()), die den HV-Dimmer-Duty "
               "(hvDimmerSetDuty()) in 5-ms-Schritten (DIGIT_FADE_STEP_MS) erst auf "
               "DIGIT_FADE_MIN_DUTY (13, ≈5 %) abblendet, bei Minimalhelligkeit die "
               "Zielziffern schreibt (nixieWrite()), und wieder auf 255 aufblendet. "
               "Die Dauer wird über fadeMs gesteuert (je zur Hälfte Ab-/Aufblenden); "
               "aktuell fest verdrahtet auf 400 ms (softFadeSecondEnabled bzw. "
               "softFadeDateEnabled in NixieClockUltra.ino). Die reine "
               "Interpolationsmathematik (fadeDutyForStep()) liegt in digit_fade_math.h "
               "und ist per Host-Unit-Test ohne Arduino-Framework testbar."))
    c.append(p("cancelDigitFade() schließt einen laufenden Fade sofort ab (Zielziffern "
               "schreiben, Duty unangetastet) und wird vor startSlotAnimation() sowie "
               "beim Eintritt in den Edit-Modus aufgerufen — Absicherung gegen eine Race "
               "zwischen laufendem Fade und neuer Display-Aktivität."))

    c.append(h2("Slot-Machine-Geschwindigkeit"))
    c.append(p("slotSpeedPct (20–100 %, Standard 100) skaliert sowohl "
               "slotRollIntervalMs als auch die gestaffelten Stopp-Zeitpunkte je Röhre "
               "(slotStopMs[i]) in startSlotAnimation() (display.ino) — bei kleineren "
               "Werten rollen die Ziffern langsamer und die Animation dauert insgesamt "
               "länger."))

    c.append(h2("Nacht-Modus (night_mode.ino)"))
    c.append(p("updateNightMode() wird jeden Loop-Durchlauf aufgerufen und "
               "bestimmt den nightState in drei Stufen:"))
    c.append(table(
        ["Priorität", "Bedingung", "Ergebnis"],
        [
            ["1 (höchste)", "nightTimeEnabled && Stunde im konfigurierten Bereich", "NIGHT_DIM oder NIGHT_DARK (je nach nightTimeMode)"],
            ["2",           "ldrEnabled && ldrReading <= ldrThreshold",             "NIGHT_DIM (LDR kann nur dimmen, nicht dunkel)"],
            ["3 (Fallback)", "keine der obigen Bedingungen",                        "NIGHT_NORMAL"],
        ],
        [2.5, 6.5, 7.0]
    ))
    c.append(p("Der LDR (GPIO6, ADC1) wird immer alle 500 ms abgetastet — unabhängig "
               "davon ob ldrEnabled gesetzt ist. So ist der Rohwert im Web-UI immer "
               "aktuell. LDR→VCC, 100 kΩ→GND: Helligkeit = hoher ADC-Wert, Dunkelheit = niedriger Wert."))

    c.append(h2("Datumsanzeige"))
    c.append(p("Nach jeder Slot-Animation und bei IR-Taste DATUM wird dateShowActive=true gesetzt. "
               "Für 5000 ms (DATE_SHOW_MS) zeigen die Röhren das Datum im Format TT MM JJ. "
               "Die NeoPixel-Ausgabe in neo_animation.ino überschreibt dabei alle Pixel: "
               "Hintergrund (0–5) und obere Trennpunkte (Pixel 6, 8) werden gelöscht. "
               "Nur die unteren Trennpunkte (Pixel 7 und 9) leuchten in warmweiß — "
               "ein visueller Hinweis, dass Datum statt Uhrzeit angezeigt wird."))

    c.append(h2("nixie_driver.ino – Direkte MCP23017-Ansteuerung"))
    c.append(p("Der Nixie-Treiber implementiert die direkte, multiplexingfreie "
               "Ansteuerung der 6 Röhren. Kernfunktionen:"))
    c.append(h3("nixieInit()"))
    c.append(p("Initialisiert den I²C-Bus (Wire.begin(SDA=8, SCL=9)) und konfiguriert "
               "alle vier MCP23017 durch Schreiben von 0x00 in die IODIRA- (0x00) und "
               "IODIRB- (0x01) Register — alle 16 Pins werden zu Ausgängen. "
               "Anschließend werden alle Ausgänge auf LOW (0) gesetzt und ein "
               "FreeRTOS-Mutex (nixieMutex) für thread-sichere Zugriffe erstellt."))
    c.append(h3("nixieWrite(uint8_t digits[6])"))
    c.append(p("Berechnet für jede der 6 Röhren das zugehörige MCP-Bit via "
               "`digitPin[tube][digit]`-Lookup (O(1)), baut den neuen Zustand in "
               "`newState[4]` auf und schreibt nur die MCPs, deren Shadow-Register "
               "(`mcpState[]`) sich geändert haben — minimiert I²C-Traffic. "
               "Der gesamte Zugriff ist durch den FreeRTOS-Mutex geschützt, da "
               "nixieWrite() sowohl aus dem Loop-Task als auch aus dem asynchronen "
               "Web-Server-Task aufgerufen werden kann."))
    c.append(code_block(
        "// Einziger I²C-Schreibzugriff pro geändertem MCP (beide Ports gleichzeitig):",
        "Wire.beginTransmission(addr);",
        "Wire.write(MCP_OLATA);          // Register 0x14",
        "Wire.write((uint8_t)(val & 0xFF));     // GPA (Low-Byte)",
        "Wire.write((uint8_t)((val >> 8)&0xFF));// GPB (High-Byte)",
        "Wire.endTransmission();",
    ))

    c.append(h2("Web-API"))
    c.append(p("Der HTTP-Server läuft auf Port 80, erreichbar über 192.168.4.1 (AP) "
               "oder http://nixieclockcs.local (Heimnetz via mDNS). "
               "POST-Endpunkte erwarten JSON (Content-Type: application/json)."))
    c.append(table(
        ["Pfad", "Methode", "Request-Body / Antwort"],
        [
            ["/",                  "GET",  "Vollständiges Web-Interface (HTML/CSS/JS eingebettet als PROGMEM)"],
            ["/api/status",        "GET",  "{time, date, neoBright, animMode, slotIval, slotSpeed, colonOn, colonStatic, colonBright, slot, nightState, ldrVal, wifiSta, ntpSynced}"],
            ["/api/settime",       "POST", "{\"h\":H,\"m\":M,\"s\":S} + optional {\"d\":D,\"mo\":Mo,\"y\":Y} → RTC schreiben"],
            ["/api/neobright",     "POST", "{\"val\":10..255}  → NeoPixel-Feinwert + NVS"],
            ["/api/anim",          "POST", "{\"mode\":0..2}  → Animationsmodus + NVS"],
            ["/api/slotinterval",  "POST", "{\"interval\":0..4}  → Slot-Intervall (SlotInterval-Enum) + NVS"],
            ["/api/colonbright",   "POST", "{\"val\":1..100}  → Trennpunkt-Helligkeit + NVS"],
            ["/api/colonon",       "POST", "{\"enabled\":true|false}  → Dauerhaft an/aus + NVS"],
            ["/api/colonstatic",   "POST", "{\"enabled\":true|false}  → Statisch warmweiß + NVS"],
            ["/api/slot",          "POST", "(kein Body) → Slot-Machine-Animation sofort starten"],
            ["/api/slotspeed",     "POST", "{\"val\":20..100}  → Slot-Machine-Rollgeschwindigkeit + NVS"],
            ["/api/softfade",      "GET",  "{sec, date} — weicher Ziffernwechsel Sekundentakt/Datum-Übergang"],
            ["/api/softfade",      "POST", "{\"sec\":true|false,\"date\":true|false} → weicher Ziffernwechsel + NVS"],
            ["/api/nightmode",     "GET",  "{ntEn, ntFrom, ntTo, ntMode, hvDimPct, ldrEn, ldrThr, ldrVal, state}"],
            ["/api/nightmode",     "POST", "{ntEn, ntFrom, ntTo, ntMode, hvDimPct, ldrEn, ldrThr} → Nacht-Modus + NVS"],
            ["/api/wifi",          "GET",  "{mode, staSsid, staIp, ntp}"],
            ["/api/wifi",          "POST", "{\"ssid\":\"…\",\"pass\":\"…\"} → STA verbinden, Neustart"],
            ["/api/ir/status",     "GET",  "8 IR-Code-Einträge (je action + code)"],
            ["/api/ir/learn",      "POST", "{\"action\":\"SET\"|\"UP\"|\"DOWN\"|\"BRIGHTNESS\"|\"ANIM_NEXT\"|\"SLOT\"|\"COLON_TOGGLE\"|\"DATUM\"}"],
            ["/api/ir/clear",      "POST", "{\"action\":\"…\"} → IR-Code löschen + NVS"],
        ],
        [4.0, 1.8, 10.2]
    ))

    c.append(h2("NVS-Persistenz"))
    c.append(p("Alle Einstellungen werden über die Arduino-`Preferences`-Bibliothek "
               "im ESP32-NVS-Flash gespeichert (Namespace \"nixie\"). "
               "Beim Start werden sie automatisch geladen."))
    c.append(table(
        ["NVS-Schlüssel", "Typ", "Default", "Inhalt"],
        [
            ["bright",        "UInt8",  "3",     "Helligkeitsstufe (0–3)"],
            ["neoBright",     "UInt8",  "30",    "NeoPixel-Feinwert Hintergrund"],
            ["colonBright",   "UInt8",  "80",    "NeoPixel-Feinwert Trennpunkte"],
            ["animMode",      "UInt8",  "0",     "Animationsmodus (0=Rainbow, 1=Statisch, 2=Puls)"],
            ["slotIval",      "UInt8",  "0",     "Slot-Intervall (SlotInterval-Enum: 0=Aus, 1=10s, 2=1min, 3=15min, 4=1h)"],
            ["slotSpeed",     "UInt8",  "100",   "Slot-Machine-Rollgeschwindigkeit (20–100 %)"],
            ["sfSecEn",       "Bool",   "false", "Weicher Ziffernwechsel Sekundentakt aktiv"],
            ["sfDateEn",      "Bool",   "false", "Weicher Ziffernwechsel Zeit/Datum aktiv"],
            ["colonOn",       "Bool",   "false", "Trennpunkte dauerhaft an"],
            ["colonStatic",   "Bool",   "false", "Trennpunkte statisch warmweiß"],
            ["ntEn",          "Bool",   "false", "Nacht-Modus Zeitbereich aktiv"],
            ["ntFrom",        "UInt8",  "23",    "Nacht-Modus Startzeit (Stunde 0–23)"],
            ["ntTo",          "UInt8",  "7",     "Nacht-Modus Endzeit (Stunde 0–23)"],
            ["ntMode",        "UInt8",  "0",     "Nacht-Modus Typ (0=Gedimmt, 1=Dunkel)"],
            ["hvDimPct",      "UInt8",  "25",    "Röhren-Dimm-Helligkeit (2–60 %)"],
            ["ldrEn",         "Bool",   "false", "Lichtsensor-Steuerung aktiv"],
            ["ldrThr",        "UInt16", "512",   "LDR-Schwellwert ADC (0–4095)"],
            ["wifiSsid",      "String", "\"\"",  "Heimnetz SSID"],
            ["wifiPass",      "String", "\"\"",  "Heimnetz Passwort"],
            ["ir_SET",        "UInt64", "0",     "IR-Code für IR_ACTION_SET"],
            ["ir_UP",         "UInt64", "0",     "IR-Code für IR_ACTION_UP"],
            ["ir_DOWN",       "UInt64", "0",     "IR-Code für IR_ACTION_DOWN"],
            ["ir_BRIGHTNESS", "UInt64", "0",     "IR-Code für IR_ACTION_BRIGHTNESS"],
            ["ir_ANIM_NEXT",  "UInt64", "0",     "IR-Code für IR_ACTION_ANIM_NEXT"],
            ["ir_SLOT",       "UInt64", "0",     "IR-Code für IR_ACTION_SLOT"],
            ["ir_COLTOGGLE",  "UInt64", "0",     "IR-Code für IR_ACTION_COLON_TOGGLE"],
            ["ir_DATE",       "UInt64", "0",     "IR-Code für IR_ACTION_DATE (Datumsanzeige)"],
        ],
        [3.0, 1.8, 2.0, 9.2]
    ))

    # ── Kapitel 5: Entwickler-Workflow ──────────────────────────────────────
    c.append(h1("Entwickler-Workflow"))

    c.append(h2("Arduino IDE einrichten"))
    c.append(p("Voraussetzung: Arduino IDE 2.x. ESP32-Boardunterstützung installieren:"))
    c.append(code_block(
        "Datei → Einstellungen → Zusätzliche Boardverwalter-URLs:",
        "https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json",
    ))
    c.append(p("Dann: Werkzeuge → Board → Boardverwalter → «esp32» suchen → "
               "Paket von Espressif Systems installieren (Version 3.x). "
               "Board auswählen: ESP32S3 Dev Module."))

    c.append(h2("Bibliotheken installieren"))
    c.append(p("Alle Bibliotheken über den Arduino Library Manager installieren "
               "(Werkzeuge → Bibliotheken verwalten):"))
    c.append(table(
        ["Bibliothek", "Suchbegriff im Library Manager"],
        [
            ["Adafruit NeoPixel",  "Adafruit NeoPixel"],
            ["Rtc by Makuna",      "Rtc by Makuna"],
            ["AsyncTCP",           "AsyncTCP (me-no-dev)"],
            ["ESPAsyncWebServer",  "ESPAsyncWebServer (me-no-dev)"],
            ["ArduinoJson",        "ArduinoJson (v6.x!)"],
            ["IRremoteESP8266",    "IRremoteESP8266"],
        ],
        [7.0, 9.0]
    ))
    c.append(note("ArduinoJson: Explizit Version 6.x wählen — Version 7 ist API-inkompatibel."))

    c.append(h2("ESP32-S3 USB-Einstellungen — kritisch!"))
    c.append(p("Das Logic Board verwendet die native USB-Schnittstelle des ESP32-S3 "
               "(USB-OTG, GPIO19/20) — es gibt keinen separaten USB-UART-Chip. "
               "Der ROM-Bootloader des ESP32-S3 meldet sich ab Werk als USB-CDC-Port, "
               "sodass das erste Flashen problemlos funktioniert."))
    c.append(p("Die Gefahr liegt im zweiten Schritt: Wird die Firmware mit der Einstellung "
               "«USB CDC On Boot: Disabled» kompiliert und geflasht, schaltet die "
               "laufende Firmware das CDC ab. Beim nächsten Start erscheint kein "
               "COM-Port mehr — die Arduino IDE kann keinen automatischen Reset in den "
               "Bootloader-Modus auslösen. Da der BOOT-Taster SW5 nicht bestückt ist, "
               "gibt es keinen manuellen Ausweg: der ESP32-S3 ist danach über USB "
               "nicht mehr programmierbar."))
    c.append(note("SW5 (BOOT) ist nicht bestückt. Einmal mit «USB CDC On Boot: Disabled» "
                  "geflasht bedeutet: nie wieder über USB flashbar — bis GPIO0 über "
                  "einen Lötpunkt manuell auf GND gezogen wird."))
    c.append(p("Folgende Einstellungen müssen unter Werkzeuge vor jedem Kompilieren "
               "und Hochladen gesetzt sein:"))
    c.append(table(
        ["Einstellung", "Wert", "Bemerkung"],
        [
            ["Board",            "ESP32S3 Dev Module",     "Espressif ESP32-S3-Paket v3.x"],
            ["USB Mode",         "Hardware CDC and JTAG",  "Natives USB des ESP32-S3 aktivieren"],
            ["Upload Mode",      "UART0 / Hardware CDC",   "Upload über dieselbe USB-Buchse"],
            ["USB CDC On Boot",  "Enabled",                "KRITISCH: Disabled sperrt USB-Zugang dauerhaft (SW5 nicht bestückt)"],
            ["USB DFU On Boot",  "Disabled",               "Nicht benötigt"],
            ["Flash Size",       "4 MB (32 Mb)",           "Passend zum ESP32-S3-WROOM-1"],
            ["Upload Speed",     "921600",                  ""],
        ],
        [4.0, 4.5, 7.5]
    ))

    c.append(h2("Firmware flashen"))
    c.append(p("1. Micro-USB-Kabel am Logic Board J2 anschließen (5 V + Datenleitungen)."))
    c.append(p("2. Sicherstellen, dass alle USB-Einstellungen korrekt gesetzt sind "
               "(siehe Abschnitt oben) — insbesondere «USB CDC On Boot: Enabled»."))
    c.append(p("3. Sketch → Hochladen (Strg+U). Die IDE erkennt den COM-Port "
               "automatisch und löst den Reset in den Bootloader-Modus aus."))

    c.append(h2("Serial Monitor / Debugging"))
    c.append(p("Baudrate: 115200. Nach dem Start gibt die Firmware auf dem seriellen "
               "Monitor den Initialisierungsablauf aus, u.a.:"))
    c.append(code_block(
        "[Nixie] MCP23017 initialisiert.",
        "[WiFi] AP gestartet: 192.168.4.1",
        "[WiFi] STA verbunden: 192.168.1.42",
        "[NTP] Synchronisiert: 14:32:07",
    ))

    c.append(h2("Häufige Probleme"))
    c.append(table(
        ["Symptom", "Ursache", "Lösung"],
        [
            ["Röhren leuchten nicht",     "HV-MOD liefert keine Spannung",    "5-V-Versorgung und HV-MOD prüfen; Spannungsmessung an J4"],
            ["Ghosting / Doppelziffern",  "I²C-Fehler, MCP nicht antwortend", "I²C-Bus prüfen (Pull-ups R65–R70), Serial Monitor auf Fehlermeldungen"],
            ["Web-UI nicht erreichbar",   "WiFi AP nicht gestartet",           "Serial Monitor prüfen; auf 192.168.4.1 verbinden"],
            ["Uhrzeit falsch nach Reset", "RTC leer / Batterie schwach",       "Zeit über Web-UI stellen; CR2032 BT1 prüfen"],
            ["Kompilierungsfehler JSON",  "ArduinoJson v7 installiert",        "ArduinoJson auf v6.x downgraden"],
            ["IR-Fernbedienung reagiert nicht", "Code noch nicht angelernt",   "Im Web-UI unter IR-Fernbedienung Codes anlernen"],
            ["ESP32 nach erstem Flash nicht mehr erreichbar, kein COM-Port", "Mit «USB CDC On Boot: Disabled» kompiliert → Firmware schaltet CDC ab", "GPIO0-Testpunkt auf GND halten, USB neu einstecken → ROM-Bootloader → mit korrekten Einstellungen neu flashen"],
        ],
        [4.5, 4.5, 7.0]
    ))

    return "".join(c)


# ─── ODT-Aufbau ─────────────────────────────────────────────────────────────

CONTENT_NS = """\
<?xml version="1.0" encoding="UTF-8"?>
<office:document-content
  xmlns:office="urn:oasis:names:tc:opendocument:xmlns:office:1.0"
  xmlns:style="urn:oasis:names:tc:opendocument:xmlns:style:1.0"
  xmlns:text="urn:oasis:names:tc:opendocument:xmlns:text:1.0"
  xmlns:table="urn:oasis:names:tc:opendocument:xmlns:table:1.0"
  xmlns:draw="urn:oasis:names:tc:opendocument:xmlns:drawing:1.0"
  xmlns:fo="urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0"
  xmlns:xlink="http://www.w3.org/1999/xlink"
  xmlns:svg="urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0"
  office:version="1.3">
"""

TABLE_AUTO_STYLES = """\
  <!-- Tabellen-Zellenstile -->
  <style:style style:name="TableStyle" style:family="table">
    <style:table-properties style:width="16cm" fo:margin-bottom="0.4cm" fo:margin-top="0.2cm"/>
  </style:style>
  <style:style style:name="TH" style:family="table-cell">
    <style:table-cell-properties fo:background-color="#1a3060"
      fo:padding-top="0.12cm" fo:padding-bottom="0.12cm"
      fo:padding-left="0.15cm" fo:padding-right="0.15cm"
      fo:border="0.5pt solid #8899aa"/>
  </style:style>
  <style:style style:name="TD" style:family="table-cell">
    <style:table-cell-properties fo:background-color="#ffffff"
      fo:padding-top="0.1cm" fo:padding-bottom="0.1cm"
      fo:padding-left="0.12cm" fo:padding-right="0.12cm"
      fo:border="0.3pt solid #cccccc"/>
  </style:style>
  <style:style style:name="TD_Alt" style:family="table-cell">
    <style:table-cell-properties fo:background-color="#eef2fa"
      fo:padding-top="0.1cm" fo:padding-bottom="0.1cm"
      fo:padding-left="0.12cm" fo:padding-right="0.12cm"
      fo:border="0.3pt solid #cccccc"/>
  </style:style>
"""

CONTENT_FOOTER = "</office:text></office:body></office:document-content>"


def build_auto_styles(col_widths: set) -> str:
    lines = ['<office:automatic-styles>\n', TABLE_AUTO_STYLES]
    for w in sorted(col_widths):
        sn = f"Col{str(round(w,2)).replace('.','p')}"
        lines.append(
            f'  <style:style style:name="{sn}" style:family="table-column">\n'
            f'    <style:table-column-properties style:column-width="{w}cm"/>\n'
            f'  </style:style>\n'
        )
    lines.append('</office:automatic-styles>\n')
    return "".join(lines)

META_XML = """\
<?xml version="1.0" encoding="UTF-8"?>
<office:document-meta
  xmlns:office="urn:oasis:names:tc:opendocument:xmlns:office:1.0"
  xmlns:dc="http://purl.org/dc/elements/1.1/"
  xmlns:meta="urn:oasis:names:tc:opendocument:xmlns:meta:1.0"
  office:version="1.3">
<office:meta>
  <dc:title>Nixie Clock Ultra – Systemdokumentation</dc:title>
  <dc:creator>broed digital media</dc:creator>
  <dc:date>2026-06-29</dc:date>
  <meta:generator>gen_sysdoc.py</meta:generator>
</office:meta>
</office:document-meta>"""


def build_manifest(images):
    entries = [
        ('manifest:full-path="/"',                           'manifest:media-type="application/vnd.oasis.opendocument.text"'),
        ('manifest:full-path="content.xml"',                 'manifest:media-type="text/xml"'),
        ('manifest:full-path="styles.xml"',                  'manifest:media-type="text/xml"'),
        ('manifest:full-path="meta.xml"',                    'manifest:media-type="text/xml"'),
    ]
    for img in images:
        entries.append((f'manifest:full-path="Pictures/{img}"', 'manifest:media-type="image/png"'))
    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        '<manifest:manifest xmlns:manifest="urn:oasis:names:tc:opendocument:xmlns:manifest:1.0" manifest:version="1.3">',
    ]
    for path, mime in entries:
        lines.append(f'  <manifest:file-entry {path} {mime}/>')
    lines.append('</manifest:manifest>')
    return "\n".join(lines)


def create_odt():
    global _used_col_widths, _img_paths
    _used_col_widths = set()   # reset before each run

    images = {
        "logic_sch.png":     IMGS / "logic_sch-1.png",
        "logic_pcb_top.png": IMGS / "logic_pcb-1.png",
        "nixie_sch.png":     IMGS / "nixie_sch-1.png",
        "nixie_pcb_top.png": IMGS / "nixie_pcb-1.png",
    }
    for key, path in [
        ("logic_pcb_bot.png", IMGS / "logic_pcb-2.png"),
        ("nixie_pcb_bot.png", IMGS / "nixie_pcb-2.png"),
    ]:
        if path.exists():
            images[key] = path

    _img_paths = images   # img() in build_content() schlägt hier nach

    body = build_content()    # populates _used_col_widths as side-effect
    auto_styles = build_auto_styles(_used_col_widths)
    content_xml = (CONTENT_NS + auto_styles +
                   "<office:body><office:text>\n" + body + CONTENT_FOOTER)
    manifest_xml = build_manifest(list(images.keys()))

    OUT.parent.mkdir(parents=True, exist_ok=True)

    with zipfile.ZipFile(OUT, "w", zipfile.ZIP_DEFLATED) as z:
        # mimetype must be first and uncompressed
        z.writestr(zipfile.ZipInfo("mimetype"), "application/vnd.oasis.opendocument.text",
                   compress_type=zipfile.ZIP_STORED)
        z.writestr("META-INF/manifest.xml", manifest_xml)
        z.writestr("meta.xml",              META_XML)
        z.writestr("styles.xml",            STYLES_XML)
        z.writestr("content.xml",           content_xml)
        for name, path in images.items():
            z.write(path, f"Pictures/{name}")

    print(f"Erstellt: {OUT}  ({OUT.stat().st_size // 1024} KB)")


if __name__ == "__main__":
    create_odt()
