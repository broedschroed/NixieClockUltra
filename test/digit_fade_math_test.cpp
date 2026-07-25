// Host-seitiger Unit-Test für die reine Interpolations-Mathematik des
// weichen Ziffernwechsels (kein Arduino-Framework nötig).
// Kompilieren & ausführen:
//   g++ -std=c++17 -Wall -o /tmp/digit_fade_math_test test/digit_fade_math_test.cpp && /tmp/digit_fade_math_test
#include "../digit_fade_math.h"
#include <cassert>
#include <cstdio>

int main() {
  // Fade-down: Start bei maxDuty, Ende exakt bei minDuty
  assert(fadeDutyForStep(false, 0, 10, 13, 255) == 255);
  assert(fadeDutyForStep(false, 10, 10, 13, 255) == 13);

  // Fade-up: Start bei minDuty, Ende exakt bei maxDuty
  assert(fadeDutyForStep(true, 0, 10, 13, 255) == 13);
  assert(fadeDutyForStep(true, 10, 10, 13, 255) == 255);

  // Monoton fallend über alle Zwischenschritte (fade-down)
  uint8_t prev = 255;
  for (uint8_t s = 1; s <= 10; s++) {
    uint8_t d = fadeDutyForStep(false, s, 10, 13, 255);
    assert(d <= prev);
    prev = d;
  }

  // Monoton steigend über alle Zwischenschritte (fade-up)
  prev = 13;
  for (uint8_t s = 1; s <= 10; s++) {
    uint8_t d = fadeDutyForStep(true, s, 10, 13, 255);
    assert(d >= prev);
    prev = d;
  }

  // Division-Guard: stepsTotal=0 darf nicht durch Null teilen
  assert(fadeDutyForStep(false, 0, 0, 13, 255) == 255);
  assert(fadeDutyForStep(true, 0, 0, 13, 255) == 13);

  // 200ms-Fade (20 Schritte) an einer Zwischenposition
  assert(fadeDutyForStep(false, 5, 20, 13, 255) == 255 - (uint8_t)((255 - 13) * 5 / 20));

  // computeChangedMask: keine Änderung → mask 0
  {
    uint8_t a[6] = {1,2,3,4,5,6};
    uint8_t b[6] = {1,2,3,4,5,6};
    assert(computeChangedMask(a, b) == 0);
  }

  // computeChangedMask: nur Tube-Index 0 (HZ) geändert → Bit 0
  {
    uint8_t a[6] = {1,2,3,4,5,6};
    uint8_t b[6] = {9,2,3,4,5,6};
    assert(computeChangedMask(a, b) == 0b000001);
  }

  // computeChangedMask: nur Tube-Index 5 (SE) geändert → Bit 5
  {
    uint8_t a[6] = {1,2,3,4,5,6};
    uint8_t b[6] = {1,2,3,4,5,9};
    assert(computeChangedMask(a, b) == 0b100000);
  }

  // computeChangedMask: Tube-Index 2 und 3 geändert (MZ+ME) → Bits 2+3
  {
    uint8_t a[6] = {1,2,3,4,5,6};
    uint8_t b[6] = {1,2,9,9,5,6};
    assert(computeChangedMask(a, b) == 0b001100);
  }

  // computeChangedMask: alle 6 geändert → 0b111111
  {
    uint8_t a[6] = {0,0,0,0,0,0};
    uint8_t b[6] = {1,1,1,1,1,1};
    assert(computeChangedMask(a, b) == 0b111111);
  }

  printf("computeChangedMask: alle Assertions OK\n");

  printf("digit_fade_math_test: alle Assertions OK\n");
  return 0;
}
