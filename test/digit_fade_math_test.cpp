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

  // Guard: maxDuty < minDuty darf nicht unterlaufen (Duty-Fenster kollabiert auf maxDuty)
  assert(fadeDutyForStep(false, 0, 10, 13, 5) == 5);
  assert(fadeDutyForStep(false, 10, 10, 13, 5) == 5);
  assert(fadeDutyForStep(true, 0, 10, 13, 5) == 5);
  assert(fadeDutyForStep(true, 10, 10, 13, 5) == 5);

  // maxDuty == minDuty: flacher Fade (kein sichtbarer Unterschied, aber kein Crash)
  assert(fadeDutyForStep(false, 5, 10, 20, 20) == 20);
  assert(fadeDutyForStep(true, 5, 10, 20, 20) == 20);

  // ── fadeDutyCurve: Potenzkurve in 10 Bit ──────────────────────────────
  // Endwerte exakt (Fade-down: max → min, Fade-up: min → max)
  assert(fadeDutyCurve(false, 0,  90, 0, 1023, 2.5f) == 1023);
  assert(fadeDutyCurve(false, 90, 90, 0, 1023, 2.5f) == 0);
  assert(fadeDutyCurve(true,  0,  90, 0, 1023, 2.5f) == 0);
  assert(fadeDutyCurve(true,  90, 90, 0, 1023, 2.5f) == 1023);
  // mit minDuty > 0 und gedimmtem maxDuty (Nacht-Modus)
  assert(fadeDutyCurve(false, 90, 90, 40, 400, 2.5f) == 40);
  assert(fadeDutyCurve(true,  90, 90, 40, 400, 2.5f) == 400);

  // Monoton über alle Schritte
  {
    uint16_t prevDown = 1023, prevUp = 0;
    for (uint16_t s = 1; s <= 90; s++) {
      uint16_t d = fadeDutyCurve(false, s, 90, 0, 1023, 2.5f);
      uint16_t u = fadeDutyCurve(true,  s, 90, 0, 1023, 2.5f);
      assert(d <= prevDown);
      assert(u >= prevUp);
      prevDown = d;
      prevUp   = u;
    }
  }

  // Kurve verbringt mehr Zeit im dunklen Bereich: zur Halbzeit deutlich
  // unter der linearen Mitte (0,5^2,5 ≈ 0,177 → ~181 von 1023)
  {
    uint16_t mid = fadeDutyCurve(true, 45, 90, 0, 1023, 2.5f);
    assert(mid > 150 && mid < 210);
  }

  // gamma = 1 entspricht der linearen Rampe
  assert(fadeDutyCurve(true, 45, 90, 0, 1000, 1.0f) == 500);

  // Guards: stepsTotal=0, stepsDone > stepsTotal, minDuty > maxDuty
  assert(fadeDutyCurve(false, 0, 0, 0, 1023, 2.5f) == 1023);
  assert(fadeDutyCurve(true, 200, 90, 0, 1023, 2.5f) == 1023);
  assert(fadeDutyCurve(true, 0, 90, 500, 100, 2.5f) == 100);

  printf("digit_fade_math_test: alle Assertions OK\n");
  return 0;
}
