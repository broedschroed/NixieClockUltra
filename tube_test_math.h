#pragma once
#include <stdint.h>

// Reine Logik für den Röhrentest (kein Arduino-Bezug), damit sie mit
// einem Host-Compiler (g++) unit-testbar ist — siehe
// test/tube_test_math_test.cpp.

// Füllt alle 6 Röhren-Slots mit derselben Testziffer.
inline void tubeTestFillDigits(uint8_t digits[6], uint8_t testDigit) {
  for (uint8_t i = 0; i < 6; i++) digits[i] = testDigit;
}

// true, sobald die Ziffernfolge 0-9 durchlaufen ist (testDigit > 9).
inline bool tubeTestIsFinished(uint8_t testDigit) {
  return testDigit > 9;
}
