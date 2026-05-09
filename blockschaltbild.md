# NixieClockUltra – Blockschaltbild

```mermaid
flowchart TD
    subgraph PWR ["Stromversorgung"]
        USB["5V USB / Netzteil"]
        HV["HV-Modul\n~170V DC"]
    end

    subgraph MCU ["ESP32-S3"]
        CORE["loop()\nHauptlogik"]
        ISR["Mux-Timer ISR\n1 MHz Basis"]
        WIFISTACK["WiFi-Stack\nAP + STA"]
        NVS["NVS Flash\nPreferences"]
    end

    subgraph NIXIE ["6× Nixie-Röhren IN-14"]
        AN["Anoden × 6\nPins 11–16"]
        KA["Kathoden × 10\nPins 17,18,38–42,45–47"]
    end

    subgraph NEO ["10× NeoPixel (Pin 21)"]
        NEO_BG["6× SMD WS2812B\nHintergrund (GRB)"]
        NEO_DP["4× THT WS2812\nTrennpunkte (RGB)"]
    end

    RTC["DS1302 RTC\nThreeWire IO=4 CLK=5 CE=2"]
    BTN["4× Taster\nSET UP DOWN LIGHT\nPins 7–10"]
    IR["VS1838B\nIR-Empfänger\nPin 48"]
    NTP["NTP-Server\npool.ntp.org"]
    BROWSER["Web-Browser\n192.168.4.1"]

    USB --> MCU
    USB --> HV
    HV --> AN

    ISR -- "Anode schalten" --> AN
    ISR -- "Kathode schalten" --> KA
    CORE -- "displayDigits\nfadeBrightness" --> ISR
    CORE -- "readRTC / writeRTC" --> RTC
    CORE -- "setPixelColor / show" --> NEO
    CORE -- "laden / speichern" --> NVS
    BTN -- "Interrupt / Polling" --> CORE
    IR -- "IRremoteESP8266" --> CORE
    CORE -- "setupWifi\nsetupWebServer" --> WIFISTACK
    WIFISTACK -- "NTP-Sync\n(einmalig)" --> NTP
    WIFISTACK -- "HTTP API\n/api/..." --> BROWSER
```

## Legende

| Block | Beschreibung |
|---|---|
| **HV-Modul** | Boost-Converter, erzeugt ~170 V DC für die Nixie-Anoden |
| **Mux-Timer ISR** | Hardware-Timer (1 MHz), schaltet alle 2800 µs zur nächsten Röhre, 200 µs Blank-Phase gegen Ghosting |
| **DS1302 RTC** | Batteriegepufferte Echtzeituhr, ThreeWire-Interface |
| **NeoPixel SMD** | WS2812B, GRB-Farbreihenfolge, Pixel 0–5 (Röhren-Hintergrund) |
| **NeoPixel THT** | WS2812 (Durchsteck), RGB-Farbreihenfolge, Pixel 6–9 (Trennpunkte) |
| **NVS Flash** | Speichert Helligkeit, Animationsmodus, WiFi-Zugangsdaten, IR-Codes |
| **WiFi AP** | Immer aktiv: SSID `NixieClock`, PW `nixie1234`, IP `192.168.4.1` |
| **WiFi STA** | Optional: Heimnetz-Verbindung für NTP-Sync |
