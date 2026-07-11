// ═══════════════════════════════════════════════════════════
//  DISPLAY-UPDATE (displayDigits befüllen)
// ═══════════════════════════════════════════════════════════
#include <string.h>

// Schreibt neue Zielziffern. Bei fadeMs>0 und nightState==NIGHT_NORMAL
// läuft ein weicher HV-Dimmer-Crossfade (digit_fade.ino), sonst sofort
// hart über nixieWrite(). Kein Effekt, wenn sich nichts ändert.
static void commitDigits(uint8_t newDigits[6], uint16_t fadeMs) {
  bool changed = memcmp(newDigits, displayDigits, 6) != 0;
  memcpy(displayDigits, newDigits, 6);
  if (!changed) return;
  if (fadeMs > 0 && nightState == NIGHT_NORMAL) {
    startDigitFade(newDigits, fadeMs);
  } else {
    nixieWrite(displayDigits);
  }
}

void setDisplayTime(uint8_t h, uint8_t m, uint8_t s) {
  uint8_t d[6] = { (uint8_t)(h / 10), (uint8_t)(h % 10),
                    (uint8_t)(m / 10), (uint8_t)(m % 10),
                    (uint8_t)(s / 10), (uint8_t)(s % 10) };
  commitDigits(d, 0);
}

void setDisplayTimeSoft(uint8_t h, uint8_t m, uint8_t s, uint16_t fadeMs) {
  uint8_t d[6] = { (uint8_t)(h / 10), (uint8_t)(h % 10),
                    (uint8_t)(m / 10), (uint8_t)(m % 10),
                    (uint8_t)(s / 10), (uint8_t)(s % 10) };
  commitDigits(d, fadeMs);
}

void setDisplayDate() {
  uint8_t d[6] = { (uint8_t)(curDay / 10),   (uint8_t)(curDay % 10),
                    (uint8_t)(curMonth / 10), (uint8_t)(curMonth % 10),
                    (uint8_t)(curYear / 10),  (uint8_t)(curYear % 10) };
  commitDigits(d, 0);
}

void setDisplayDateSoft(uint16_t fadeMs) {
  uint8_t d[6] = { (uint8_t)(curDay / 10),   (uint8_t)(curDay % 10),
                    (uint8_t)(curMonth / 10), (uint8_t)(curMonth % 10),
                    (uint8_t)(curYear / 10),  (uint8_t)(curYear % 10) };
  commitDigits(d, fadeMs);
}

// ═══════════════════════════════════════════════════════════
//  SLOT-MACHINE ANIMATION
// ═══════════════════════════════════════════════════════════
void startSlotAnimation(uint8_t h, uint8_t m, uint8_t s) {
  slotTarget[0] = h / 10; slotTarget[1] = h % 10;
  slotTarget[2] = m / 10; slotTarget[3] = m % 10;
  slotTarget[4] = s / 10; slotTarget[5] = s % 10;
  for (int i = 0; i < 6; i++) slotCurrent[i] = random(10);
  slotActive   = true;
  slotStartMs  = millis();
}

void updateSlotAnimation() {
  if (!slotActive) return;
  unsigned long elapsed = millis() - slotStartMs;

  // Jede Röhre stoppt zu einem anderen Zeitpunkt
  bool allDone = true;
  for (int i = 0; i < 6; i++) {
    unsigned long stopTime = 600 + i * 180; // ms
    if (elapsed < stopTime) {
      // Noch rollend
      if ((millis() % 60) < 30) {
        slotCurrent[i] = (slotCurrent[i] + 1) % 10;
      }
      displayDigits[i] = slotCurrent[i];
      allDone = false;
    } else {
      displayDigits[i] = slotTarget[i];
    }
  }
  if (allDone) slotActive = false;
  nixieWrite(displayDigits);
}
