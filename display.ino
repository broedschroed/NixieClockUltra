// ═══════════════════════════════════════════════════════════
//  DISPLAY-UPDATE (displayDigits befüllen)
// ═══════════════════════════════════════════════════════════

// Schreibt displayDigits nur wenn Nixie-PWM gerade eingeschaltet ist.
// Verhindert kurze Aufblitzer während der PWM-Off-Phase im Dimm-Modus.
static void nixieWriteSafe() {
  if (nightState == NIGHT_DARK) return;
  if (nightState == NIGHT_DIM && !nixiePwmOn) return;
  nixieWrite(displayDigits);
}

void setDisplayTime(uint8_t h, uint8_t m, uint8_t s) {
  displayDigits[0] = h / 10;
  displayDigits[1] = h % 10;
  displayDigits[2] = m / 10;
  displayDigits[3] = m % 10;
  displayDigits[4] = s / 10;
  displayDigits[5] = s % 10;
  nixieWriteSafe();
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

void setDisplayDate() {
  displayDigits[0] = curDay   / 10;
  displayDigits[1] = curDay   % 10;
  displayDigits[2] = curMonth / 10;
  displayDigits[3] = curMonth % 10;
  displayDigits[4] = curYear  / 10;
  displayDigits[5] = curYear  % 10;
  nixieWriteSafe();
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
  nixieWriteSafe();
}
