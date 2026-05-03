// ═══════════════════════════════════════════════════════════
//  TIMER-ISR – MULTIPLEX (auto-reload, MUX_BLANK_US Periode)
//
//  Der Timer läuft mit fester Periode (MUX_BLANK_US = 200 µs, auto-reload).
//  Ein Tick-Zähler steuert die zwei Phasen pro Röhre:
//
//    Tick 0           → Digit-Phase Start: Anode + Kathode einschalten
//    Tick 1 .. N-2    → Digit-Phase halten: nichts tun
//    Tick N-1 (= 9)   → Blank-Phase: alles aus, nächste Röhre wählen
//
//  Warum kein timerAlarm() im ISR:
//    timerAlarm() ist in Arduino Core 3.x nicht IRAM_ATTR → stalled beim
//    Flash-Zugriff → hält portENTER_CRITICAL_ISR → Interrupt-WDT.
//
//  Warum gpio_set_level() statt digitalWrite():
//    Die äußere digitalWrite()-Wrapper ist nicht ARDUINO_ISR_ATTR → Flash.
//    gpio_set_level() (driver/gpio.h) ist in IDF 5.x IRAM_ATTR.
// ═══════════════════════════════════════════════════════════

#define MUX_TICKS_PER_TUBE  ((MUX_BLANK_US + MUX_DIGIT_US) / MUX_BLANK_US)

void IRAM_ATTR onMuxTimer() {
  portENTER_CRITICAL_ISR(&timerMux);

  static uint8_t tick = 0;
  uint8_t digit = displayDigits[muxIndex];

  if (tick == 0) {
    // Digit-Phase: erst Kathode, dann Anode – kein Zeitfenster mit Anode HIGH ohne selektierte Kathode
    if (digit <= 9) {
      gpio_set_level((gpio_num_t)CATHODE_PIN[digit], 1);
      gpio_set_level((gpio_num_t)ANODE_PIN[muxIndex], 1);
    }
  } else if (tick == MUX_TICKS_PER_TUBE - MUX_BLANK_TICKS) {
    // Blank-Phase: erst Anode aus, dann alle Kathoden aktiv auf LOW – entlädt parasitäre Restladungen
    gpio_set_level((gpio_num_t)ANODE_PIN[muxIndex], 0);
    for (uint8_t i = 0; i < 10; i++) {
      gpio_set_level((gpio_num_t)CATHODE_PIN[i], 0);
    }
    muxIndex = (muxIndex + 1) % 6;
  } 

  if (++tick >= MUX_TICKS_PER_TUBE) tick = 0;

  portEXIT_CRITICAL_ISR(&timerMux);

//  timerAlarm(muxTimer, MUX_BLANK_US, false, 0);  // kein auto-reload: ISR muss timerAlarm() selbst aufrufen
}
