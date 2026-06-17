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
  // SET drücken wechselt den Edit-State
  if (btnSet.pressed) {
    if (editState == EDIT_NONE) {
      editState    = EDIT_HOUR;
      editEnterTime = millis();
    } else if (editState == EDIT_HOUR) {
      editState = EDIT_MIN;
      editEnterTime = millis();
    } else if (editState == EDIT_MIN) {
      editState = EDIT_SEC;
      editEnterTime = millis();
    } else {
      // Speichern & beenden
      writeRTC();
      editState = EDIT_NONE;
    }
  }

  // Timeout
  if (editState != EDIT_NONE && (millis() - editEnterTime > EDIT_TIMEOUT_MS)) {
    writeRTC();
    editState = EDIT_NONE;
  }

  if (editState == EDIT_NONE) return;

  // UP / DOWN (mit Auto-Repeat)
  int delta = 0;
  if (btnUp.pressed   || btnUp.held)   delta = +1;
  if (btnDown.pressed || btnDown.held) delta = -1;

  if (delta != 0) {
    if (editState == EDIT_HOUR) {
      curHour = (curHour + 24 + delta) % 24;
    } else if (editState == EDIT_MIN) {
      curMin = (curMin + 60 + delta) % 60;
    } else if (editState == EDIT_SEC) {
      curSec = (curSec + 60 + delta) % 60;
    }
    editEnterTime = millis();
  }

  // Blinken der aktiven Stelle (250ms)
  bool blinkOn = ((millis() / 250) % 2 == 0);

  uint8_t dH = curHour, dM = curMin, dS = curSec;

  // Aktive Stelle ausblenden (Blink-Effekt)
  if (!blinkOn) {
    if      (editState == EDIT_HOUR) { dH = 99; }  // 99 = keine Anzeige
    else if (editState == EDIT_MIN)  { dM = 99; }
    else if (editState == EDIT_SEC)  { dS = 99; }
  }

  // Sonderfall 99: Blanke Röhren
  displayDigits[0] = (dH == 99) ? 10 : dH / 10;  // 10 = keine Kathode aktiv
  displayDigits[1] = (dH == 99) ? 10 : dH % 10;
  displayDigits[2] = (dM == 99) ? 10 : dM / 10;
  displayDigits[3] = (dM == 99) ? 10 : dM % 10;
  displayDigits[4] = (dS == 99) ? 10 : dS / 10;
  displayDigits[5] = (dS == 99) ? 10 : dS % 10;
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

