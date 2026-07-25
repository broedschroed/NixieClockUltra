// ═══════════════════════════════════════════════════════════
//  HARDWARE-DIMMUNG DER ANODENSPANNUNG (TLP627, LEDC-PWM)
//  Ohne HV_PER_TUBE_DIMMER: ein gemeinsamer Schalter für alle 6 Anoden.
//  Mit HV_PER_TUBE_DIMMER: 6 unabhängige Schalter, einer pro Röhre.
// ═══════════════════════════════════════════════════════════

#ifdef HV_PER_TUBE_DIMMER

static const uint8_t hvTubePin[6] = {
  HV_TUBE_PIN_0, HV_TUBE_PIN_1, HV_TUBE_PIN_2,
  HV_TUBE_PIN_3, HV_TUBE_PIN_4, HV_TUBE_PIN_5
};

void hvDimmerInit() {
  // Gemeinsamen Logic-Board-Schalter dauerhaft offen halten, falls er bei der
  // Umrüstung nicht physisch entfernt/überbrückt wurde — sonst bliebe er als
  // schwebender Eingang im Reset-Zustand und könnte die Anodenspannung sperren.
  pinMode(HV_SWITCH_PIN, OUTPUT);
  digitalWrite(HV_SWITCH_PIN, HIGH);
  for (uint8_t i = 0; i < 6; i++) {
    ledcAttach(hvTubePin[i], HV_PWM_FREQ_HZ, 8);
    ledcWrite(hvTubePin[i], 255);   // volle Helligkeit (Anode dauerhaft an)
  }
}

void hvDimmerSetDutyAll(uint8_t duty0to255) {
  for (uint8_t i = 0; i < 6; i++) ledcWrite(hvTubePin[i], duty0to255);
}

void hvDimmerSetDutyTube(uint8_t tube, uint8_t duty0to255) {
  ledcWrite(hvTubePin[tube], duty0to255);
}

#else  // heutige Hardware: ein gemeinsamer Schalter

void hvDimmerInit() {
  ledcAttach(HV_SWITCH_PIN, HV_PWM_FREQ_HZ, 8);
  ledcWrite(HV_SWITCH_PIN, 255);   // volle Helligkeit (Anode dauerhaft an)
}

void hvDimmerSetDutyAll(uint8_t duty0to255) {
  ledcWrite(HV_SWITCH_PIN, duty0to255);
}

// Ohne Pro-Röhre-Hardware gibt es nur den einen gemeinsamen Schalter — der
// Röhrenindex wird ignoriert, jeder Aufruf dimmt alle Anoden gemeinsam
// (reproduziert exakt das bisherige Verhalten).
void hvDimmerSetDutyTube(uint8_t /*tube*/, uint8_t duty0to255) {
  ledcWrite(HV_SWITCH_PIN, duty0to255);
}

#endif
