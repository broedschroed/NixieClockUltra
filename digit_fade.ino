// ═══════════════════════════════════════════════════════════
//  WEICHER ZIFFERNWECHSEL – HV-Dimmer-Crossfade (pro Röhre)
//  Non-blocking State-Machine, angetrieben von updateDigitFade()
//  in loop(). Nutzt hv_dimmer.ino (Duty) + nixie_driver.ino
//  (Ziffern schreiben). Reine Interpolations-/Masken-Mathematik
//  in digit_fade_math.h (host-testbar, siehe test/).
//
//  Ohne HV_PER_TUBE_DIMMER dimmt hvDimmerSetDutyTube() ohnehin den
//  einen gemeinsamen Schalter (siehe hv_dimmer.ino) — dieser Code
//  bleibt für beide Hardware-Varianten identisch.
// ═══════════════════════════════════════════════════════════

#include <string.h>
#include "digit_fade_math.h"

#define DIGIT_FADE_MIN_DUTY   13   // ~5% von 255
#define DIGIT_FADE_STEP_MS    5    // Schrittintervall

enum FadeDir { FADE_DOWN, FADE_UP };

static bool     fadeRunning    = false;
static FadeDir  fadeDir        = FADE_DOWN;
static uint8_t  fadeTargetDigits[6];
static uint8_t  fadeMask       = 0;     // Bitmaske der Röhren, die diesen Fade durchlaufen
static uint8_t  fadeMaxDuty    = 255;   // Ziel-Helligkeit oben (255 normal, hvDimPct-skaliert im Dimm-Modus)
static uint32_t fadeLastStepMs = 0;
static uint8_t  fadeStepsTotal = 1;
static uint8_t  fadeStepsDone  = 0;

// Setzt jede Röhre aus mask auf duty.
static void applyDutyToMask(uint8_t mask, uint8_t duty) {
  for (uint8_t tube = 0; tube < 6; tube++) {
    if (mask & (uint8_t)(1u << tube)) hvDimmerSetDutyTube(tube, duty);
  }
}

// Schließt einen laufenden Fade sofort ab: schreibt die Zielziffern und setzt
// die betroffenen Röhren auf ihre Ziel-Helligkeit. Wird intern verwendet, wenn
// ein neuer Fade angefordert wird während einer noch läuft.
static void fadeFinishImmediately() {
  if (!fadeRunning) return;
  nixieWrite(fadeTargetDigits);
  applyDutyToMask(fadeMask, fadeMaxDuty);
  fadeRunning = false;
}

// Schließt einen laufenden Fade sofort ab, OHNE die Helligkeit anzufassen
// (der Aufrufer setzt sie danach selbst, z.B. beim Nacht-Modus-Wechsel).
// No-op wenn kein Fade läuft.
void cancelDigitFade() {
  if (!fadeRunning) return;
  nixieWrite(fadeTargetDigits);
  fadeRunning = false;
}

// Startet einen neuen Fade zu newDigits über fadeMs (aufgeteilt in
// fadeMs/2 Abblenden + fadeMs/2 Aufblenden). changedMask markiert die Röhren,
// deren Ziffer sich ändert (siehe computeChangedMask() in digit_fade_math.h)
// — nur diese werden während des Fades angefasst, unveränderte Röhren bleiben
// auf ihrer aktuellen Helligkeit. fadeMs muss > 0 sein (Aufrufer in
// display.ino prüft das bereits vor dem Aufruf).
void startDigitFade(uint8_t newDigits[6], uint8_t changedMask, uint16_t fadeMs) {
  fadeFinishImmediately();
  memcpy(fadeTargetDigits, newDigits, 6);
  fadeMask       = changedMask;
  fadeMaxDuty    = (nightState == NIGHT_DIM) ? (uint8_t)(hvDimPct * 255 / 100) : 255;
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
                                  DIGIT_FADE_MIN_DUTY, fadeMaxDuty);
  applyDutyToMask(fadeMask, duty);

  if (fadeStepsDone >= fadeStepsTotal) {
    if (fadeDir == FADE_DOWN) {
      nixieWrite(fadeTargetDigits);   // Ziffern bei Minimalhelligkeit umschalten
      fadeDir       = FADE_UP;
      fadeStepsDone = 0;
    } else {
      fadeRunning = false;            // Fade komplett, Duty ist bereits fadeMaxDuty
    }
  }
}
