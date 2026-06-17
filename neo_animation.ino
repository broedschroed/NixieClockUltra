// ═══════════════════════════════════════════════════════════
//  NEOPIXEL ANIMATIONEN
// ═══════════════════════════════════════════════════════════

// Hintergrund: Pixel 0–5 (unter den Röhren)  → SMD WS2812B, NEO_GRB
// Doppelpunkte: Pixel 6–9                     → THT WS2812,  NEO_RGB (R/G vertauscht)

// Tauscht R und G eines gepackten Color-Werts, damit THT-Pixel (RGB-Reihenfolge)
// trotz GRB-Strip die richtige Farbe zeigen.
static inline uint32_t rgbSwap(uint32_t c) {
  return strip.Color((c >> 8) & 0xFF, (c >> 16) & 0xFF, c & 0xFF);
}

// Skaliert einen gepackten Color-Wert auf einen Helligkeitswert 0–255.
static inline uint32_t scaleColor(uint32_t c, uint8_t brightness) {
  return strip.Color(
    ((c >> 16) & 0xFF) * brightness / 255,
    ((c >>  8) & 0xFF) * brightness / 255,
    ( c        & 0xFF) * brightness / 255
  );
}

void updateNeoPixel() {
  unsigned long now = millis();
  if (now - lastNeoUpdate < 20) return;
  lastNeoUpdate = now;

  switch (animMode) {

    case ANIM_RAINBOW: {
      for (int i = 0; i < 6; i++) {
        uint8_t hue = neoHue + i * 40;
        strip.setPixelColor(i, scaleColor(strip.ColorHSV(hue * 256, neoSat, 255), neoBright));
      }
      neoHue++;
      bool colonOn = colonAlwaysOn || (curSec % 2 == 0);
      uint32_t colonColor = colonOn
        ? strip.ColorHSV((neoHue + 128) * 256, 200, 255)
        : strip.Color(0, 0, 0);
      for (int i = 6; i < 10; i++) strip.setPixelColor(i, rgbSwap(scaleColor(colonColor, colonBright)));
      break;
    }

    case ANIM_STATIC: {
      uint32_t warm = strip.Color(255, 180, 80);
      for (int i = 0; i < 6; i++) strip.setPixelColor(i, scaleColor(warm, neoBright));
      bool colonOn = colonAlwaysOn || (curSec % 2 == 0);
      uint32_t colonColor = colonOn ? strip.Color(255, 100, 0) : strip.Color(0, 0, 0);
      for (int i = 6; i < 10; i++) strip.setPixelColor(i, rgbSwap(scaleColor(colonColor, colonBright)));
      break;
    }

    case ANIM_PULSE: {
      float phase = (millis() % 2000) / 2000.0f;
      uint8_t val = (uint8_t)(127.5f + 127.5f * sinf(phase * 2 * M_PI));
      uint32_t col = strip.ColorHSV(neoHue * 256, neoSat, val);
      for (int i = 0; i < 6; i++) strip.setPixelColor(i, scaleColor(col, neoBright));
      neoHue++;
      bool colonOn = colonAlwaysOn || (curSec % 2 == 0);
      uint32_t colonColor = colonOn ? col : strip.Color(0, 0, 0);
      for (int i = 6; i < 10; i++)
        strip.setPixelColor(i, rgbSwap(scaleColor(colonColor, colonBright)));
      break;
    }

    default: break;
  }

  // Slot-Effekt überschreibt Hintergrundanimation solange slotActive
  if (slotActive) {
    uint32_t col = strip.ColorHSV((millis() / 5) % 65536, 255, 255);
    for (int i = 0; i < 6;  i++) strip.setPixelColor(i, scaleColor(col, neoBright));
    for (int i = 6; i < 10; i++) strip.setPixelColor(i, rgbSwap(scaleColor(col, colonBright)));
  }

  // Statischer Modus: Trennpunkte warmweiß – colonAlwaysOn steuert weiterhin das Blinken
  if (colonStatic && !slotActive) {
    bool colonOn = colonAlwaysOn || (curSec % 2 == 0);
    uint32_t warmwhite = colonOn ? strip.Color(255, 100, 0) : strip.Color(0, 0, 0);
    for (int i = 6; i < 10; i++)
      strip.setPixelColor(i, rgbSwap(scaleColor(warmwhite, colonBright)));
  }

  strip.show();
}
