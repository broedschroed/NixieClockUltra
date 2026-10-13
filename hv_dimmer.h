#pragma once
#include <stdint.h>

// 10 Bit statt 8: Der ESP32-S3-Core taktet LEDC mit 40 MHz (XTAL); mit 8 Bit
// sind darunter nur Frequenzen ab ~153 Hz einrichtbar. Die Firmware arbeitet
// überwiegend mit Helligkeitswerten 0–255, hvDuty() skaliert auf die Auflösung.
// Der weiche Ziffernwechsel rechnet direkt in 10 Bit (hvDimmerSetDutyTube10).
#define HV_PWM_RES_BITS  10
#define HV_DUTY_MAX      ((1u << HV_PWM_RES_BITS) - 1)   // 1023

static inline uint32_t hvDuty(uint8_t duty0to255) {
  return (uint32_t)duty0to255 * HV_DUTY_MAX / 255;
}

void hvDimmerSetDutyTube10(uint8_t tube, uint16_t duty10);
