// ═══════════════════════════════════════════════════════════
//  RTC HILFSFUNKTIONEN
// ═══════════════════════════════════════════════════════════
void readRTC() {
  if (!Rtc.IsDateTimeValid()) return;
  RtcDateTime now = Rtc.GetDateTime();
  curHour = now.Hour();
  curMin  = now.Minute();
  curSec  = now.Second();
}

void writeRTC() {
  RtcDateTime dt(2024, 1, 1, curHour, curMin, curSec);
  Rtc.SetDateTime(dt);
}
