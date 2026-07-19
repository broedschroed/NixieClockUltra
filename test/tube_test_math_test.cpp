// Host-seitiger Unit-Test für die reine Röhrentest-Logik (kein Arduino-Framework nötig).
// Kompilieren & ausführen:
//   g++ -std=c++17 -Wall -o /tmp/tube_test_math_test test/tube_test_math_test.cpp && /tmp/tube_test_math_test
#include "../tube_test_math.h"
#include <cassert>
#include <cstdio>

int main() {
  // tubeTestFillDigits: alle 6 Slots erhalten denselben Wert
  uint8_t digits[6];
  tubeTestFillDigits(digits, 0);
  for (int i = 0; i < 6; i++) assert(digits[i] == 0);

  tubeTestFillDigits(digits, 7);
  for (int i = 0; i < 6; i++) assert(digits[i] == 7);

  tubeTestFillDigits(digits, 9);
  for (int i = 0; i < 6; i++) assert(digits[i] == 9);

  // tubeTestIsFinished: 0..9 laufen weiter, erst >9 ist Ende
  for (uint8_t d = 0; d <= 9; d++) assert(!tubeTestIsFinished(d));
  assert(tubeTestIsFinished(10));
  assert(tubeTestIsFinished(255));

  printf("tube_test_math_test: alle Assertions OK\n");
  return 0;
}
