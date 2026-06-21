// ═══════════════════════════════════════════════════════════
//  RTC HILFSFUNKTIONEN
// ═══════════════════════════════════════════════════════════
void readRTC() {
  if (!Rtc.IsDateTimeValid()) return;
  RtcDateTime now = Rtc.GetDateTime();
  curHour  = now.Hour();
  curMin   = now.Minute();
  curSec   = now.Second();
  curDay   = now.Day();
  curMonth = now.Month();
  curYear  = now.Year() % 100;
}

void writeRTC() {
  RtcDateTime dt(2000 + curYear, curMonth, curDay, curHour, curMin, curSec);
  Rtc.SetDateTime(dt);
}
