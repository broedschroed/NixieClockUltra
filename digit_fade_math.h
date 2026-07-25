#pragma once
#include <stdint.h>

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
  uint16_t span = (uint16_t)(maxDuty - minDuty) * stepsDone / stepsTotal;
  return risingUp ? (uint8_t)(minDuty + span) : (uint8_t)(maxDuty - span);
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
