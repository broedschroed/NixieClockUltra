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

void updateNeoPixel() {
  unsigned long now = millis();
  if (now - lastNeoUpdate < 20) return;
  lastNeoUpdate = now;

  uint8_t effectBright = powerSaveActive ? (neoBright / 4) : neoBright;
  strip.setBrightness(effectBright);

  switch (animMode) {

    case ANIM_RAINBOW: {
      // Farbverlauf über alle Hintergrund-Pixel
      for (int i = 0; i < 6; i++) {
        uint8_t hue = neoHue + i * 40;
        strip.setPixelColor(i, strip.ColorHSV(hue * 256, neoSat, 255));
      }
      neoHue++;
      // Doppelpunkte: synchron blinkend zum Sekundentakt
      bool colonOn = (curSec % 2 == 0);
      uint32_t colonColor = colonOn
        ? strip.ColorHSV((neoHue + 128) * 256, 200, 255)
        : strip.Color(0, 0, 0);
      for (int i = 6; i < 10; i++) strip.setPixelColor(i, rgbSwap(colonColor));
      break;
    }

    case ANIM_STATIC: {
      // Statische Farbe (warmweiß)
      uint32_t warm = strip.Color(255, 180, 80);
      for (int i = 0; i < 6; i++) strip.setPixelColor(i, warm);
      bool colonOn = (curSec % 2 == 0);
      uint32_t colonColor = colonOn ? strip.Color(255, 100, 0) : strip.Color(0, 0, 0);
      for (int i = 6; i < 10; i++) strip.setPixelColor(i, rgbSwap(colonColor));
      break;
    }

    case ANIM_PULSE: {
      // Puls-Effekt (sinusförmig)
      float phase = (millis() % 2000) / 2000.0f;
      uint8_t val = (uint8_t)(127.5f + 127.5f * sinf(phase * 2 * M_PI));
      uint32_t col = strip.ColorHSV(neoHue * 256, neoSat, val);
      for (int i = 0; i < 6; i++) strip.setPixelColor(i, col);
      neoHue++;
      bool colonOn = (curSec % 2 == 0);
      for (int i = 6; i < 10; i++)
        strip.setPixelColor(i, colonOn ? rgbSwap(col) : strip.Color(0,0,0));
      break;
    }

    case ANIM_SLOTS: {
      // Schnelles Farbwechseln während Slot-Animation
      if (slotActive) {
        uint32_t col = strip.ColorHSV((millis() / 5) % 65536, 255, 255);
        for (int i = 0; i < 6; i++) strip.setPixelColor(i, col);
        for (int i = 6; i < 10; i++) strip.setPixelColor(i, rgbSwap(col));
      } else {
        // Fallback auf Rainbow
        for (int i = 0; i < 6; i++) {
          uint8_t hue = neoHue + i * 40;
          strip.setPixelColor(i, strip.ColorHSV(hue * 256, neoSat, 255));
        }
        neoHue++;
        bool colonOn = (curSec % 2 == 0);
        uint32_t colonColor = colonOn
          ? strip.ColorHSV((neoHue + 128) * 256, 200, 255)
          : strip.Color(0, 0, 0);
        for (int i = 6; i < 10; i++) strip.setPixelColor(i, rgbSwap(colonColor));
      }
      break;
    }

    default: break;
  }

  irrecv.pause();
  strip.show();
  irrecv.resume();
}
