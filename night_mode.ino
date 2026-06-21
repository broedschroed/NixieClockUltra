// ═══════════════════════════════════════════════════════════
//  NACHT-MODUS – LDR + Zeitbereich
//  LDR-Schaltung: LDR→VCC, 100kΩ→GND, ADC an GPIO 6
//  Dunkle Umgebung = niedriger ADC-Wert
// ═══════════════════════════════════════════════════════════

#define LDR_PIN        6
#define LDR_SAMPLE_MS  500

// Alle globalen Variablen sind in NixieClockUltra.ino deklariert

static bool timeInNightRange(uint8_t h) {
  if (nightStart <= nightEnd)
    return (h >= nightStart && h < nightEnd);
  else
    return (h >= nightStart || h < nightEnd);
}

void updateNightMode() {
  if (millis() - lastLdrRead >= LDR_SAMPLE_MS) {
    lastLdrRead = millis();
    uint32_t sum = 0;
    for (int i = 0; i < 4; i++) sum += analogRead(LDR_PIN);
    ldrReading = (uint16_t)(sum / 4);
  }

  if (nightTimeEnabled && timeInNightRange(curHour)) {
    nightState = (nightTimeMode == 1) ? NIGHT_DARK : NIGHT_DIM;
  } else if (ldrEnabled && ldrReading <= ldrThreshold) {
    nightState = NIGHT_DIM;
  } else {
    nightState = NIGHT_NORMAL;
  }
}
