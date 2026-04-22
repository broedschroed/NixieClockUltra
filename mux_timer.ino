// ═══════════════════════════════════════════════════════════
//  TIMER-ISR – MULTIPLEX
// ═══════════════════════════════════════════════════════════
void IRAM_ATTR onMuxTimer() {
  portENTER_CRITICAL_ISR(&timerMux);

  if (inBlank) {
    // --- BLANK-Phase beendet: Röhre aktivieren ---
    inBlank = false;

    uint8_t digit = displayDigits[muxIndex];

    if (digit > 9) {
      // Blink-Modus: Röhre komplett aus lassen (keine Anode, keine Kathode)
      // Nächsten Alarm trotzdem setzen
      timerAlarm(muxTimer, MUX_DIGIT_US, false, 0);
      portEXIT_CRITICAL_ISR(&timerMux);
      return;
    }

    // Anode einschalten
    digitalWrite(ANODE_PIN[muxIndex], HIGH);
    // Kathode einschalten
    digitalWrite(CATHODE_PIN[digit], HIGH);

    // Nächsten Alarm: Anzeigezeit
    timerAlarm(muxTimer, MUX_DIGIT_US, false, 0);

  } else {
    // --- DIGIT-Phase beendet: Alles aus (Anti-Ghosting) ---
    // Anode aus
    digitalWrite(ANODE_PIN[muxIndex], LOW);
    // Kathode aus (nur wenn digit gültig)
    uint8_t digit = displayDigits[muxIndex];
    if (digit <= 9) {
      digitalWrite(CATHODE_PIN[digit], LOW);
    }

    // Nächste Röhre
    muxIndex = (muxIndex + 1) % 6;

    inBlank = true;
    // Nächsten Alarm: Blanking-Zeit
    timerAlarm(muxTimer, MUX_BLANK_US, false, 0);
  }

  portEXIT_CRITICAL_ISR(&timerMux);
}
