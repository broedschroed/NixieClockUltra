// ═══════════════════════════════════════════════════════════
//  TASTER-ENTPRELLUNG
// ═══════════════════════════════════════════════════════════
void updateButton(Button &b) {
  b.pressed = false;
  b.held    = false;
  bool state = digitalRead(b.pin);

  if (state == LOW && b.lastState == HIGH) {
    // Fallende Flanke – Entprellzeit starten
    b.pressTime  = millis();
    b.debounced  = false;
  }

  if (state == LOW) {
    if (!b.debounced && millis() - b.pressTime >= b.debounceMs) {
      // Entprellzeit abgelaufen: einmaliges Press-Event
      b.pressed    = true;
      b.debounced  = true;
      b.lastRepeat = millis();
    }
    if (b.debounced && millis() - b.lastRepeat >= b.repeatMs) {
      // Auto-Repeat solange gehalten
      b.held       = true;
      b.lastRepeat = millis();
    }
  }

  if (state == HIGH) b.debounced = false;

  b.lastState = state;
}

// ═══════════════════════════════════════════════════════════
//  EINSTELLMODUS (FSM)
// ═══════════════════════════════════════════════════════════
void handleEditMode() {
  if (btnSet.pressed) {
    switch (editState) {
      case EDIT_NONE:   editState = EDIT_HOUR;  break;
      case EDIT_HOUR:   editState = EDIT_MIN;   break;
      case EDIT_MIN:    editState = EDIT_SEC;   break;
      case EDIT_SEC:    editState = EDIT_DAY;   break;
      case EDIT_DAY:    editState = EDIT_MONTH; break;
      case EDIT_MONTH:  editState = EDIT_YEAR;  break;
      case EDIT_YEAR:   writeRTC(); editState = EDIT_NONE; return;
    }
    editEnterTime = millis();
  }

  if (editState != EDIT_NONE && millis() - editEnterTime > EDIT_TIMEOUT_MS) {
    writeRTC();
    editState = EDIT_NONE;
  }
  if (editState == EDIT_NONE) return;

  int delta = 0;
  if (btnUp.pressed   || btnUp.held)   delta = +1;
  if (btnDown.pressed || btnDown.held) delta = -1;

  if (delta != 0) {
    switch (editState) {
      case EDIT_HOUR:  curHour  = (curHour  + 24  + delta) % 24; break;
      case EDIT_MIN:   curMin   = (curMin   + 60  + delta) % 60; break;
      case EDIT_SEC:   curSec   = (curSec   + 60  + delta) % 60; break;
      case EDIT_DAY:   curDay   = (uint8_t)((curDay   - 1 + 31 + delta) % 31) + 1; break;
      case EDIT_MONTH: curMonth = (uint8_t)((curMonth - 1 + 12 + delta) % 12) + 1; break;
      case EDIT_YEAR:  curYear  = (uint8_t)((curYear  + 100 + delta) % 100);       break;
      default: break;
    }
    editEnterTime = millis();
  }

  bool blinkOn = ((millis() / 250) % 2 == 0);

  if (editState == EDIT_HOUR || editState == EDIT_MIN || editState == EDIT_SEC) {
    uint8_t dH = curHour, dM = curMin, dS = curSec;
    if (!blinkOn) {
      if      (editState == EDIT_HOUR) dH = 99;
      else if (editState == EDIT_MIN)  dM = 99;
      else if (editState == EDIT_SEC)  dS = 99;
    }
    displayDigits[0] = (dH == 99) ? 10 : dH / 10;
    displayDigits[1] = (dH == 99) ? 10 : dH % 10;
    displayDigits[2] = (dM == 99) ? 10 : dM / 10;
    displayDigits[3] = (dM == 99) ? 10 : dM % 10;
    displayDigits[4] = (dS == 99) ? 10 : dS / 10;
    displayDigits[5] = (dS == 99) ? 10 : dS % 10;
  } else {
    uint8_t dD = curDay, dMo = curMonth, dY = curYear;
    if (!blinkOn) {
      if      (editState == EDIT_DAY)   dD  = 99;
      else if (editState == EDIT_MONTH) dMo = 99;
      else if (editState == EDIT_YEAR)  dY  = 99;
    }
    displayDigits[0] = (dD  == 99) ? 10 : dD  / 10;
    displayDigits[1] = (dD  == 99) ? 10 : dD  % 10;
    displayDigits[2] = (dMo == 99) ? 10 : dMo / 10;
    displayDigits[3] = (dMo == 99) ? 10 : dMo % 10;
    displayDigits[4] = (dY  == 99) ? 10 : dY  / 10;
    displayDigits[5] = (dY  == 99) ? 10 : dY  % 10;
  }
  nixieWrite(displayDigits);
}

// ═══════════════════════════════════════════════════════════
//  HELLIGKEITSSTEUERUNG & COLON-TOGGLE
// ═══════════════════════════════════════════════════════════
void handleBrightness() {
  // Kurzer Druck: Helligkeit weiterschalten
  if (btnLight.pressed) {
    brightLevel = (brightLevel + 1) % 4;
    neoBright   = BRIGHTNESS_LEVELS[brightLevel];
    prefs.putUChar("bright", brightLevel);
    prefs.putUChar("neoBright", neoBright);
  }

  // Langer Druck: Trennpunkte dauerhaft an/aus (einmalig pro Druck)
  static bool colonToggleDone = false;
  if (btnLight.held && !colonToggleDone) {
    colonAlwaysOn = !colonAlwaysOn;
    prefs.putBool("colonOn", colonAlwaysOn);
    colonToggleDone = true;
  }
  if (btnLight.lastState == HIGH) colonToggleDone = false;
}

