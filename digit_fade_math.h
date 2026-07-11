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
