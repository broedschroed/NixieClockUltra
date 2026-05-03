// ═══════════════════════════════════════════════════════════
//  IR-AKTIONEN
// ═══════════════════════════════════════════════════════════
void executeAction(IrAction action) {
  switch (action) {
    case IR_ACTION_SET:
      if (editState == EDIT_NONE) {
        editState = EDIT_HOUR;
        editEnterTime = millis();
      } else if (editState == EDIT_HOUR) {
        editState = EDIT_MIN;
        editEnterTime = millis();
      } else if (editState == EDIT_MIN) {
        editState = EDIT_SEC;
        editEnterTime = millis();
      } else {
        writeRTC();
        editState = EDIT_NONE;
      }
      break;

    case IR_ACTION_UP:
      if (editState == EDIT_HOUR) { curHour = (curHour + 1) % 24; editEnterTime = millis(); }
      else if (editState == EDIT_MIN) { curMin = (curMin + 1) % 60; editEnterTime = millis(); }
      else if (editState == EDIT_SEC) { curSec = (curSec + 1) % 60; editEnterTime = millis(); }
      break;

    case IR_ACTION_DOWN:
      if (editState == EDIT_HOUR) { curHour = (curHour + 23) % 24; editEnterTime = millis(); }
      else if (editState == EDIT_MIN) { curMin = (curMin + 59) % 60; editEnterTime = millis(); }
      else if (editState == EDIT_SEC) { curSec = (curSec + 59) % 60; editEnterTime = millis(); }
      break;

    case IR_ACTION_BRIGHTNESS:
      brightLevel = (brightLevel + 1) % 4;
      neoBright   = BRIGHTNESS_LEVELS[brightLevel];
      prefs.putUChar("bright", brightLevel);
      prefs.putUChar("neoBright", neoBright);
      break;

    case IR_ACTION_ANIM_NEXT:
      animMode = (AnimMode)((int(animMode) + 1) % ANIM_COUNT);
      prefs.putUChar("animMode", (uint8_t)animMode);
      break;

    case IR_ACTION_SLOT:
      startSlotAnimation(curHour, curMin, curSec);
      break;

    case IR_ACTION_COLON_TOGGLE:
      colonAlwaysOn = !colonAlwaysOn;
      prefs.putBool("colonOn", colonAlwaysOn);
      break;

    default: break;
  }
}

void dispatchIRAction(uint64_t code) {
  for (int i = 0; i < IR_ACTION_COUNT; i++) {
    portENTER_CRITICAL(&irMux);
    bool match = (irCodes[i] != 0 && irCodes[i] == code);
    portEXIT_CRITICAL(&irMux);
    if (match) {
      executeAction((IrAction)i);
      return;
    }
  }
}

void handleIR() {
  // Lernmodus-Timeout
  if (irLearnTarget != IR_LEARN_NONE &&
      millis() - irLearnStartMs > IR_LEARN_TIMEOUT_MS) {
    irLearnTarget = IR_LEARN_NONE;
  }

  if (!irrecv.decode(&irResults)) return;

  uint64_t code = irResults.value;
  irrecv.resume();

  // Repeat-Codes ignorieren (NEC sendet 0xFFFFFFFF als Repeat)
  if (code == 0xFFFFFFFFULL || code == 0xFFFFFFFFFFFFFFFFULL) return;

  portENTER_CRITICAL(&irMux);
  if (irLearnTarget != IR_LEARN_NONE) {
    int target = (int)irLearnTarget;
    irCodes[target] = code;
    irLearnTarget = IR_LEARN_NONE;
    portEXIT_CRITICAL(&irMux);
    prefs.putULong64(IR_ACTION_KEYS[target], code);
    Serial.printf("[IR] Gelernt: %s = 0x%016llX\n",
      IR_ACTION_LABELS[target], (unsigned long long)code);
  } else {
    portEXIT_CRITICAL(&irMux);
    dispatchIRAction(code);
  }
}
