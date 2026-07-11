// ═══════════════════════════════════════════════════════════
//  WEICHER ZIFFERNWECHSEL – HV-Dimmer-Crossfade
//  Non-blocking State-Machine, angetrieben von updateDigitFade()
//  in loop(). Nutzt hv_dimmer.ino (Duty) + nixie_driver.ino
//  (Ziffern schreiben). Reine Interpolations-Mathematik in
//  digit_fade_math.h (siehe Task 1, host-testbar).
// ═══════════════════════════════════════════════════════════

#include <string.h>
#include "digit_fade_math.h"

#define DIGIT_FADE_MIN_DUTY   13   // ~5% von 255
#define DIGIT_FADE_STEP_MS    5    // Schrittintervall

enum FadeDir { FADE_DOWN, FADE_UP };

static bool     fadeRunning    = false;
static FadeDir  fadeDir        = FADE_DOWN;
static uint8_t  fadeTargetDigits[6];
static uint32_t fadeLastStepMs = 0;
static uint8_t  fadeStepsTotal = 1;
static uint8_t  fadeStepsDone  = 0;

// Schließt einen laufenden Fade sofort ab: schreibt die Zielziffern
// und setzt volle Helligkeit. Wird intern verwendet, wenn ein neuer
// Fade angefordert wird während einer noch läuft.
static void fadeFinishImmediately() {
  if (!fadeRunning) return;
  nixieWrite(fadeTargetDigits);
  hvDimmerSetDuty(255);
  fadeRunning = false;
}

// Schließt einen laufenden Fade sofort ab, OHNE die Helligkeit
// anzufassen (der Aufrufer setzt sie danach selbst, z.B. beim
// Nacht-Modus-Wechsel). No-op wenn kein Fade läuft.
void cancelDigitFade() {
  if (!fadeRunning) return;
  nixieWrite(fadeTargetDigits);
  fadeRunning = false;
}

// Startet einen neuen Fade zu newDigits über fadeMs (aufgeteilt in
// fadeMs/2 Abblenden + fadeMs/2 Aufblenden). fadeMs muss > 0 sein
// (Aufrufer in display.ino prüft das bereits vor dem Aufruf).
void startDigitFade(uint8_t newDigits[6], uint16_t fadeMs) {
  fadeFinishImmediately();
  memcpy(fadeTargetDigits, newDigits, 6);
  fadeStepsTotal = (uint8_t)((fadeMs / 2) / DIGIT_FADE_STEP_MS);
  if (fadeStepsTotal < 1) fadeStepsTotal = 1;
  fadeStepsDone  = 0;
  fadeDir        = FADE_DOWN;
  fadeRunning    = true;
  fadeLastStepMs = millis();
}

// In loop() bei jedem Durchlauf aufrufen. No-op wenn kein Fade läuft.
void updateDigitFade() {
  if (!fadeRunning) return;
  if (millis() - fadeLastStepMs < DIGIT_FADE_STEP_MS) return;
  fadeLastStepMs = millis();
  fadeStepsDone++;

  uint8_t duty = fadeDutyForStep(fadeDir == FADE_UP, fadeStepsDone, fadeStepsTotal,
                                  DIGIT_FADE_MIN_DUTY, 255);
  hvDimmerSetDuty(duty);

  if (fadeStepsDone >= fadeStepsTotal) {
    if (fadeDir == FADE_DOWN) {
      nixieWrite(fadeTargetDigits);   // Ziffern bei Minimalhelligkeit umschalten
      fadeDir       = FADE_UP;
      fadeStepsDone = 0;
    } else {
      fadeRunning = false;            // Fade komplett, Duty ist bereits 255
    }
  }
}
