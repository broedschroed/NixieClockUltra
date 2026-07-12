# Nixie Clock Ultra

A handcrafted Nixie tube clock built around an ESP32-S3, combining the warm,
unmistakable glow of six IN-12A Nixie tubes with modern connectivity: a full
web interface, IR remote, RGB ambient lighting, and automatic NTP time sync.

**🌐 Project website:** https://broedschroed.github.io/NixieClockUltra/

## Highlights

- **Direct drive, zero ghosting** — every cathode is switched by its own
  transistor (via MCP23017 I/O expanders), so there's no multiplexing and no
  flicker or ghosting between digits.
- **Web interface** — full control from any browser on the local network, no
  app required.
- **IR remote** — seven learnable functions (set time, brightness, animation,
  slot effect, colon toggle, ...).
- **RGB ambient lighting** — 10x WS2812B NeoPixels with rainbow, static, and
  pulse animation modes, plus a slot-machine digit-shuffle effect.
- **Night mode** — dims the tubes via hardware PWM on the anode voltage,
  triggered by a schedule or an onboard light sensor (LDR).
- **Soft digit transitions** — smooth crossfade between digits instead of an
  abrupt switch.
- **DS1302 RTC** with automatic NTP sync when connected to WiFi.

## Hardware

- ESP32-S3 (native USB)
- 6x IN-12A Nixie tubes, driven directly through 4x MCP23017 I2C expanders
- ~170V HV supply for the tubes, switched via a TLP627 opto-coupler for
  hardware-PWM dimming
- DS1302 RTC
- 10x WS2812B NeoPixels
- VS1838 IR receiver
- 4 physical buttons (Set / Up / Down / Light)

KiCad sources for the logic board and the Nixie display board are included
under `docs/system/`.

## Repository layout

- `*.ino` / `*.h` — firmware (Arduino/ESP32 sketch)
- `test/` — host-side unit tests for the fade math
- `docs/manual/` — user manual
- `docs/system/` — system documentation and KiCad hardware sources
- `docs/website/` — source of the project website (deployed via GitHub Pages)

## Building

Open the sketch in the Arduino IDE (or arduino-cli) with the ESP32 core
installed, board set to an ESP32-S3 module with **USB CDC on Boot: Enabled**,
and the required libraries (ArduinoJson, ESPAsyncWebServer, IRremoteESP8266,
Adafruit NeoPixel). Flash and connect to the `NixieClockCS` WiFi hotspot (or
your home network once configured) to reach the web interface.

## License

No license has been chosen yet — all rights reserved by the author unless
stated otherwise.
