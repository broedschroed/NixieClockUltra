// ═══════════════════════════════════════════════════════════
//  NIXIE DIRECT DRIVE – MCP23017 via I2C
//  4× MCP23017 an I2C-Bus (SDA=GPIO8, SCL=GPIO9)
//  Adressen: 0x20, 0x21, 0x22, 0x23
// ═══════════════════════════════════════════════════════════

#include <Wire.h>

#define MCP_BASE_ADDR  0x20
#define MCP_IODIRA     0x00
#define MCP_IODIRB     0x01
#define MCP_OLATA      0x14

// Shadow-Register: aktueller Ausgangszustand der 4 MCPs
// Bit 0–7 = GPA0–7, Bit 8–15 = GPB0–7
static uint16_t mcpState[4] = {0, 0, 0, 0};

// Zuordnung (tube, digit_value) → MCP-Index und Bit (0–15)
struct DigitPin { uint8_t mcp; uint8_t bit; };

static const DigitPin digitPin[6][10] = {
  // Index: [tube][digit_value]  digit_value 0=Ziffer0, 1=Ziffer1, ..., 9=Ziffer9

  // Tube 0: Stundenzehner (HZ) – MCP 0x20, Bits 0–9
  { {0, 9}, {0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {0, 7}, {0, 8} },

  // Tube 1: Stundeneiner  (HE) – MCP 0x20 Bits 10–15, MCP 0x21 Bits 0–3
  { {1, 3}, {0,10}, {0,11}, {0,12}, {0,13}, {0,14}, {0,15}, {1, 0}, {1, 1}, {1, 2} },

  // Tube 2: Minutenzehner (MZ) – MCP 0x21, Bits 4–13
  { {1,13}, {1, 4}, {1, 5}, {1, 6}, {1, 7}, {1, 8}, {1, 9}, {1,10}, {1,11}, {1,12} },

  // Tube 3: Minuteneiner  (ME) – MCP 0x22, Bits 0–9
  { {2, 9}, {2, 0}, {2, 1}, {2, 2}, {2, 3}, {2, 4}, {2, 5}, {2, 6}, {2, 7}, {2, 8} },

  // Tube 4: Sekundenzehner(SZ) – MCP 0x22 Bits 10–15, MCP 0x23 Bits 0–3
  { {3, 3}, {2,10}, {2,11}, {2,12}, {2,13}, {2,14}, {2,15}, {3, 0}, {3, 1}, {3, 2} },

  // Tube 5: Sekundeneiner (SE) – MCP 0x23, Bits 4–13
  { {3,13}, {3, 4}, {3, 5}, {3, 6}, {3, 7}, {3, 8}, {3, 9}, {3,10}, {3,11}, {3,12} },
};

static void mcpWriteAB(uint8_t addr, uint16_t value) {
  Wire.beginTransmission(addr);
  Wire.write(MCP_OLATA);
  Wire.write((uint8_t)(value & 0xFF));         // GPA (Low-Byte)
  Wire.write((uint8_t)((value >> 8) & 0xFF));  // GPB (High-Byte)
  Wire.endTransmission();
}

void nixieInit() {
  Wire.begin(8, 9);
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t addr = MCP_BASE_ADDR + i;
    Wire.beginTransmission(addr);
    Wire.write(MCP_IODIRA);
    Wire.write(0x00);  // GPA: alle Pins als Output
    Wire.endTransmission();
    Wire.beginTransmission(addr);
    Wire.write(MCP_IODIRB);
    Wire.write(0x00);  // GPB: alle Pins als Output
    Wire.endTransmission();
    mcpState[i] = 0;
    mcpWriteAB(addr, 0x0000);
  }
  Serial.println("[Nixie] MCP23017 initialisiert.");
}

// digits[6]: Werte 0–9 = Ziffer anzeigen, 10 = Röhre blank
void nixieWrite(uint8_t digits[6]) {
  uint16_t newState[4] = {0, 0, 0, 0};
  for (uint8_t tube = 0; tube < 6; tube++) {
    uint8_t d = digits[tube];
    if (d <= 9) {
      const DigitPin &dp = digitPin[tube][d];
      newState[dp.mcp] |= (1u << dp.bit);
    }
  }
  for (uint8_t i = 0; i < 4; i++) {
    if (newState[i] != mcpState[i]) {
      mcpWriteAB(MCP_BASE_ADDR + i, newState[i]);
      mcpState[i] = newState[i];
    }
  }
}
