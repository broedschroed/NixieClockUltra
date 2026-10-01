#pragma once
#include <stdint.h>

// Start-Schritte für das dauerhafte Startprotokoll (boot_diag.ino).
enum BootStage : uint8_t {
  BOOT_STAGE_START = 0,   // setup() begonnen
  BOOT_STAGE_PREFS,       // Einstellungen geladen
  BOOT_STAGE_MCP,         // MCP23017 initialisiert
  BOOT_STAGE_RADIO,       // Funkmodul + AP gestartet (Röhren noch aus)
  BOOT_STAGE_TUBES_ON,    // Röhren aufgeblendet
  BOOT_STAGE_WIFI,        // Heimnetz-Verbindung abgeschlossen
  BOOT_STAGE_WEB,         // Web-Server gestartet
  BOOT_STAGE_READY,       // setup() fertig
  BOOT_STAGE_RUN_10S,     // läuft seit 10 s
  BOOT_STAGE_RUN_60S,     // läuft seit 60 s
  BOOT_STAGE_RUN_10MIN,   // läuft seit 10 min
  BOOT_STAGE_COUNT
};

void bootDiagInit();
void bootDiagStage(uint8_t stage);
void bootDiagLoop();
void bootDiagPrint();
