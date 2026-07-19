// ═══════════════════════════════════════════════════════════
//  RÖHRENTEST – alle 6 Röhren zeigen synchron dieselbe Ziffer
//  0..9 für je 2s, um defekte Kathoden/Lötstellen sichtbar zu machen
// ═══════════════════════════════════════════════════════════
#include "tube_test_math.h"
#include <string.h>

void startTubeTest() {
  cancelDigitFade();
  slotActive     = false;   // laufende Slot-Animation abbrechen
  dateShowActive = false;   // laufende Datumsanzeige abbrechen
  editState      = EDIT_NONE;   // Einstellmodus verlassen

  tubeTestActive    = true;
  tubeTestDigit     = 0;
  tubeTestStepStart = millis();

  hvDimmerSetDuty(255);   // volle Helligkeit erzwingen, unabhängig von Nacht-Modus

  tubeTestFillDigits(displayDigits, tubeTestDigit);
  nixieWrite(displayDigits);   // sofort hart, kein Fade
}

void updateTubeTest() {
  if (!tubeTestActive) return;
  if (millis() - tubeTestStepStart < TUBE_TEST_STEP_MS) return;

  tubeTestDigit++;
  if (tubeTestIsFinished(tubeTestDigit)) {
    stopTubeTest();
    return;
  }
  tubeTestStepStart = millis();
  tubeTestFillDigits(displayDigits, tubeTestDigit);
  nixieWrite(displayDigits);
}

void stopTubeTest() {
  if (!tubeTestActive) return;
  tubeTestActive = false;

  // HV-Duty passend zum aktuellen Nacht-Modus-Zustand wiederherstellen
  switch (nightState) {
    case NIGHT_DARK:   hvDimmerSetDuty(0);                    break;
    case NIGHT_DIM:    hvDimmerSetDuty(hvDimPct * 255 / 100); break;
    case NIGHT_NORMAL: hvDimmerSetDuty(255);                  break;
  }
  prevNightState = nightState;   // verhindert doppelte Duty-Anwendung im nächsten loop()

  setDisplayTime(curHour, curMin, curSec);   // Uhrzeit sofort wieder anzeigen
}
