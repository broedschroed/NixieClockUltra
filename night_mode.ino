// ═══════════════════════════════════════════════════════════
//  NACHT-MODUS – LDR + Zeitbereich
//  LDR-Schaltung: LDR→VCC, 100kΩ→GND, ADC an GPIO 6
//  Dunkle Umgebung = niedriger ADC-Wert
// ═══════════════════════════════════════════════════════════

#define LDR_PIN        6
#define LDR_SAMPLE_MS  500

// NightState-Enum ist in NixieClockUltra.ino definiert
NightState nightState = NIGHT_NORMAL;

// Konfiguration (aus NVS geladen)
bool     nightTimeEnabled = false;
uint8_t  nightStart       = 23;
uint8_t  nightEnd         = 7;
uint8_t  nightTimeMode    = 0;    // 0 = DIM, 1 = DARK
bool     ldrEnabled       = false;
uint16_t ldrThreshold     = 512;

// Interner Zustand
uint16_t ldrReading    = 4095;   // default = hell → kein Trigger
uint32_t lastLdrRead   = 0;

static bool timeInNightRange(uint8_t h) {
  if (nightStart <= nightEnd)
    return (h >= nightStart && h < nightEnd);
  else
    return (h >= nightStart || h < nightEnd);
}

void updateNightMode() {
  if (ldrEnabled && millis() - lastLdrRead >= LDR_SAMPLE_MS) {
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
