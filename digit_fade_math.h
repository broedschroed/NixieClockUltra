#pragma once
#include <stdint.h>
#include <math.h>

// Reine Interpolations-Mathematik für den HV-Dimmer-Crossfade beim
// weichen Ziffernwechsel. Keine Arduino-Abhängigkeiten, damit sie mit
// einem Host-Compiler (g++) unit-testbar ist — siehe
// test/digit_fade_math_test.cpp.
//
// risingUp=false (Fade-Down): stepsDone=0 → maxDuty, stepsDone=stepsTotal → minDuty
// risingUp=true  (Fade-Up):   stepsDone=0 → minDuty, stepsDone=stepsTotal → maxDuty
inline uint8_t fadeDutyForStep(bool risingUp, uint8_t stepsDone, uint8_t stepsTotal,
                                uint8_t minDuty, uint8_t maxDuty) {
  if (stepsTotal == 0) stepsTotal = 1;
  if (stepsDone > stepsTotal) stepsDone = stepsTotal;
  if (minDuty > maxDuty) minDuty = maxDuty;
  uint16_t span = (uint16_t)(maxDuty - minDuty) * stepsDone / stepsTotal;
  return risingUp ? (uint8_t)(minDuty + span) : (uint8_t)(maxDuty - span);
}

// Potenzkurve für den weichen Ziffernwechsel, 10-Bit-Duty (0–1023).
// Nixies werden erst bei kleinem Tastverhältnis sichtbar dunkler (Auge
// logarithmisch, TLP627-Abschaltverzögerung) — eine lineare Rampe wirkt
// deshalb wie ein kurzes Wegtauchen. Mit gamma > 1 verbringt die Kurve mehr
// Zeit im dunklen Bereich:
//   risingUp=false (Fade-Down): min + (max-min)·(1-t)^gamma, t = stepsDone/stepsTotal
//   risingUp=true  (Fade-Up):   min + (max-min)·t^gamma
// gamma = 1 entspricht der linearen Rampe. Endwerte sind exakt min bzw. max.
inline uint16_t fadeDutyCurve(bool risingUp, uint16_t stepsDone, uint16_t stepsTotal,
                              uint16_t minDuty, uint16_t maxDuty, float gamma) {
  if (stepsTotal == 0) stepsTotal = 1;
  if (stepsDone > stepsTotal) stepsDone = stepsTotal;
  if (minDuty > maxDuty) minDuty = maxDuty;
  float t     = (float)stepsDone / (float)stepsTotal;
  float level = risingUp ? t : 1.0f - t;
  float f     = powf(level, gamma);
  return (uint16_t)(minDuty + (float)(maxDuty - minDuty) * f + 0.5f);
}

// Vergleicht alte gegen neue Ziffern und liefert eine Bitmaske der Röhren,
// deren Ziffer sich geändert hat. Bit i (i=0..5) entspricht Tube-Index i
// (0=HZ, 1=HE, 2=MZ, 3=ME, 4=SZ, 5=SE — dieselbe Reihenfolge wie
// displayDigits[6] und digitPin[6][10] in nixie_driver.ino).
inline uint8_t computeChangedMask(const uint8_t oldDigits[6], const uint8_t newDigits[6]) {
  uint8_t mask = 0;
  for (uint8_t i = 0; i < 6; i++) {
    if (oldDigits[i] != newDigits[i]) mask |= (uint8_t)(1u << i);
  }
  return mask;
}
