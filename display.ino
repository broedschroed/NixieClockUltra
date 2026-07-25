// ═══════════════════════════════════════════════════════════
//  DISPLAY-UPDATE (displayDigits befüllen)
// ═══════════════════════════════════════════════════════════
#include <string.h>
#include "digit_fade_math.h"

// Schreibt neue Zielziffern. Bei fadeMs>0 und nightState!=NIGHT_DARK läuft ein
// weicher HV-Dimmer-Crossfade (digit_fade.ino) nur für die Röhren, deren
// Ziffer sich ändert; in NIGHT_DARK ist die Anodenspannung ohnehin aus, dort
// wird sofort hart über nixieWrite() geschrieben. Kein Effekt, wenn sich
// nichts ändert.
static void commitDigits(uint8_t newDigits[6], uint16_t fadeMs) {
  // Maske VOR dem Überschreiben von displayDigits berechnen (vergleicht
  // alt gegen neu) — bestimmt zugleich, ob sich überhaupt etwas ändert.
  uint8_t changedMask = computeChangedMask(displayDigits, newDigits);
  if (changedMask == 0) return;
  memcpy(displayDigits, newDigits, 6);
  if (fadeMs > 0 && nightState != NIGHT_DARK) {
    startDigitFade(newDigits, changedMask, fadeMs);
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
  cancelDigitFade();
  slotTarget[0] = h / 10; slotTarget[1] = h % 10;
  slotTarget[2] = m / 10; slotTarget[3] = m % 10;
  slotTarget[4] = s / 10; slotTarget[5] = s % 10;
  for (int i = 0; i < 6; i++) slotCurrent[i] = random(10);
  slotRollIntervalMs = (uint16_t)(60UL * 100 / slotSpeedPct);
  for (int i = 0; i < 6; i++) {
    slotStopMs[i] = (uint16_t)((600UL + (unsigned long)i * 180UL) * 100 / slotSpeedPct);
  }
  slotActive   = true;
  slotStartMs  = millis();
}

void updateSlotAnimation() {
  if (!slotActive) return;
  unsigned long elapsed = millis() - slotStartMs;

  // Jede Röhre stoppt zu einem anderen (skalierten) Zeitpunkt
  bool allDone = true;
  for (int i = 0; i < 6; i++) {
    unsigned long stopTime = slotStopMs[i];
    if (elapsed < stopTime) {
      // Noch rollend
      if ((millis() % slotRollIntervalMs) < (slotRollIntervalMs / 2)) {
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
