// ═══════════════════════════════════════════════════════════
//  HARDWARE-DIMMUNG DER ANODENSPANNUNG (TLP627, LEDC-PWM)
//  Ohne HV_PER_TUBE_DIMMER: ein gemeinsamer Schalter für alle 6 Anoden.
//  Mit HV_PER_TUBE_DIMMER: 6 unabhängige Schalter, einer pro Röhre.
//
//  hvDimmerInit() startet mit geschlossenen Anoden (Duty 0), damit beim
//  Einschalten keine Röhrenlast auf den Netzteil-Einschaltstoß trifft.
//  hvDimmerSoftStart() blendet die Röhren danach nacheinander auf.
// ═══════════════════════════════════════════════════════════

#include "digit_fade_math.h"

// 10 Bit statt 8: Der ESP32-S3-Core taktet LEDC mit 40 MHz (XTAL); mit 8 Bit
// sind darunter nur Frequenzen ab ~153 Hz einrichtbar. Die Firmware arbeitet
// weiter mit Helligkeitswerten 0–255, hvDuty() skaliert auf die Auflösung.
#define HV_PWM_RES_BITS  10

static inline uint32_t hvDuty(uint8_t duty0to255) {
  return (uint32_t)duty0to255 * ((1u << HV_PWM_RES_BITS) - 1) / 255;
}

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
    if (!ledcAttach(hvTubePin[i], HV_PWM_FREQ_HZ, HV_PWM_RES_BITS))
      Serial.printf("[HV] FEHLER: LEDC %u Hz an GPIO %u nicht einrichtbar\n", HV_PWM_FREQ_HZ, hvTubePin[i]);
    ledcWrite(hvTubePin[i], 0);     // Anode zu – Aufblenden per hvDimmerSoftStart()
  }
  Serial.printf("[HV] PWM %u Hz, tatsaechlich: %lu Hz\n", HV_PWM_FREQ_HZ, (unsigned long)ledcReadFreq(hvTubePin[0]));
}

// Blockierend, nur aus setup(): Röhren nacheinander von 0 auf 255 rampen,
// damit die Last am Netzteil stufenweise statt schlagartig ansteigt.
void hvDimmerSoftStart() {
  const uint8_t steps = HV_SOFTSTART_TUBE_MS / HV_SOFTSTART_STEP_MS;
  for (uint8_t i = 0; i < 6; i++) {
    for (uint8_t s = 1; s <= steps; s++) {
      ledcWrite(hvTubePin[i], hvDuty(fadeDutyForStep(true, s, steps, 0, 255)));
      delay(HV_SOFTSTART_STEP_MS);
    }
  }
}

void hvDimmerSetDutyAll(uint8_t duty0to255) {
  for (uint8_t i = 0; i < 6; i++) ledcWrite(hvTubePin[i], hvDuty(duty0to255));
}

void hvDimmerSetDutyTube(uint8_t tube, uint8_t duty0to255) {
  ledcWrite(hvTubePin[tube], hvDuty(duty0to255));
}

#else  // heutige Hardware: ein gemeinsamer Schalter

void hvDimmerInit() {
  if (!ledcAttach(HV_SWITCH_PIN, HV_PWM_FREQ_HZ, HV_PWM_RES_BITS))
    Serial.printf("[HV] FEHLER: LEDC %u Hz an GPIO %u nicht einrichtbar\n", HV_PWM_FREQ_HZ, HV_SWITCH_PIN);
  ledcWrite(HV_SWITCH_PIN, 0);     // Anoden zu – Aufblenden per hvDimmerSoftStart()
  Serial.printf("[HV] PWM %u Hz, tatsaechlich: %lu Hz\n", HV_PWM_FREQ_HZ, (unsigned long)ledcReadFreq(HV_SWITCH_PIN));
}

// Blockierend, nur aus setup(): mit nur einem gemeinsamen Schalter können
// die Röhren nicht einzeln starten – alle zusammen über die gleiche
// Gesamtdauer aufblenden wie die Pro-Röhre-Variante.
void hvDimmerSoftStart() {
  const uint8_t steps = 6 * HV_SOFTSTART_TUBE_MS / HV_SOFTSTART_STEP_MS;
  for (uint8_t s = 1; s <= steps; s++) {
    ledcWrite(HV_SWITCH_PIN, hvDuty(fadeDutyForStep(true, s, steps, 0, 255)));
    delay(HV_SOFTSTART_STEP_MS);
  }
}

void hvDimmerSetDutyAll(uint8_t duty0to255) {
  ledcWrite(HV_SWITCH_PIN, hvDuty(duty0to255));
}

// Ohne Pro-Röhre-Hardware gibt es nur den einen gemeinsamen Schalter — der
// Röhrenindex wird ignoriert, jeder Aufruf dimmt alle Anoden gemeinsam
// (reproduziert exakt das bisherige Verhalten).
void hvDimmerSetDutyTube(uint8_t /*tube*/, uint8_t duty0to255) {
  ledcWrite(HV_SWITCH_PIN, hvDuty(duty0to255));
}

#endif
