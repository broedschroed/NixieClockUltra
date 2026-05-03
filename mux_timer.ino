// ═══════════════════════════════════════════════════════════
//  TIMER-ISR – MULTIPLEX (auto-reload, MUX_BLANK_US Periode)
//
//  MUX_DIGIT_US=4400, MUX_BLANK_US=400 → 12 Ticks pro Röhre:
//
//    Tick 0          → Anode HIGH (alle Kathoden noch LOW aus Blank-Phase)
//                      Kein Zünden möglich → dV/dt-Transiente bleibt harmlos
//    Tick 1          → Kathode HIGH: Röhre zündet sauber
//    Tick 1..6       → Digit leuchtet (6 × 400 µs = 2,4 ms)
//    Tick 7          → Blank: Anode LOW, alle Kathoden LOW, nächste Röhre
//    Tick 7..11      → Blank (5 × 400 µs = 2,0 ms)
//    Tick 12         → tick-Überlauf → muxIndex++, tick=0
//
//  Warum Anode VOR Kathode (entgegen der alten Reihenfolge):
//    Der dV/dt-Sprung der Anode (0V→170V) koppelt kapazitiv auf
//    benachbarte Anodenleitungen. Wenn in diesem Moment bereits eine
//    Kathode HIGH ist, zündet die betroffene Röhre kurz mit dem falschen
//    Zeichen (klassischer "0-in-allen-Röhren"-Ghost).
//    Da die Blank-Phase alle Kathoden auf LOW setzt, ist Tick 0 garantiert
//    kathoden-frei → die Transiente ist ungefährlich.
//    Erst in Tick 1, wenn die Anode stabil auf 170V liegt, wird die
//    Kathode gesetzt.
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
    // Anode einschalten – Kathoden noch LOW → kein Ghost durch dV/dt-Transiente
    if (digit <= 9)
      gpio_set_level((gpio_num_t)ANODE_PIN[muxIndex], 1);

  } else if (tick == 1) {
    // Kathode erst nach Anode-Einschwingen setzen
    if (digit <= 9)
      gpio_set_level((gpio_num_t)CATHODE_PIN[digit], 1);

  } else if (tick == MUX_TICKS_PER_TUBE - 5) {
    // Blank-Phase: Anode aus, alle Kathoden LOW
    gpio_set_level((gpio_num_t)ANODE_PIN[muxIndex], 0);
    for (uint8_t i = 0; i < 10; i++)
      gpio_set_level((gpio_num_t)CATHODE_PIN[i], 0);
  }

  if (++tick >= MUX_TICKS_PER_TUBE) {
    muxIndex = (muxIndex + 1) % 6;
    tick = 0;
  }

  portEXIT_CRITICAL_ISR(&timerMux);
}
