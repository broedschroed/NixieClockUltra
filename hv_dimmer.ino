// ═══════════════════════════════════════════════════════════
//  HARDWARE-DIMMUNG DER ANODENSPANNUNG (TLP627, LEDC-PWM)
// ═══════════════════════════════════════════════════════════

void hvDimmerInit() {
  ledcAttach(HV_SWITCH_PIN, HV_PWM_FREQ_HZ, 8);
  ledcWrite(HV_SWITCH_PIN, 255);   // volle Helligkeit (Anode dauerhaft an)
}

void hvDimmerSetDuty(uint8_t duty0to255) {
  ledcWrite(HV_SWITCH_PIN, duty0to255);
}
