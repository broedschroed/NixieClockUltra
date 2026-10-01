// ═══════════════════════════════════════════════════════════
//  BOOT-DIAGNOSE: dauerhaftes Startprotokoll im NVS
//  Zur Fehlersuche bei unsauberem Start (Verdacht: Brownout durch
//  Einschalt-Lastspitzen). Pro Start wird ein Eintrag mit Reset-Grund,
//  zuletzt erreichtem Start-Schritt und Laufzeit gespeichert (Ringpuffer).
//  Das Protokoll überlebt auch vollständigen Spannungsverlust: Uhr am
//  problematischen Netzteil betreiben, danach am PC starten – die
//  Historie wird beim Booten auf der seriellen Konsole ausgegeben.
// ═══════════════════════════════════════════════════════════

#include <esp_system.h>
#include <Preferences.h>

#include "boot_diag.h"

#define BOOT_LOG_ENTRIES  20

static const char *const BOOT_STAGE_NAMES[BOOT_STAGE_COUNT] = {
  "setup begonnen", "Einstellungen geladen", "MCP23017 init",
  "Funk/AP gestartet", "Roehren aufgeblendet", "Heimnetz fertig", "Web-Server gestartet",
  "setup fertig", "laeuft 10 s", "laeuft 60 s", "laeuft 10 min"
};

struct BootLogEntry {
  uint16_t seq;      // fortlaufende Startnummer
  uint8_t  reason;   // esp_reset_reason_t
  uint8_t  stage;    // BootStage
  uint32_t ms;       // millis() beim Erreichen des Schritts
};

struct BootLog {
  uint16_t     nextSeq;
  uint8_t      head;     // Index des aktuellen (jüngsten) Eintrags
  uint8_t      count;
  BootLogEntry e[BOOT_LOG_ENTRIES];
};

static Preferences bootLogPrefs;
static BootLog     bootLog;
static bool        bootLogOk = false;

static const char *resetReasonName(uint8_t r) {
  switch ((esp_reset_reason_t)r) {
    case ESP_RST_POWERON:    return "POWERON";
    case ESP_RST_EXT:        return "EXT";
    case ESP_RST_SW:         return "SW";
    case ESP_RST_PANIC:      return "PANIC";
    case ESP_RST_INT_WDT:    return "INT_WDT";
    case ESP_RST_TASK_WDT:   return "TASK_WDT";
    case ESP_RST_WDT:        return "WDT";
    case ESP_RST_DEEPSLEEP:  return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:   return "BROWNOUT";
    case ESP_RST_SDIO:       return "SDIO";
    case ESP_RST_USB:        return "USB";
    case ESP_RST_JTAG:       return "JTAG";
    case ESP_RST_EFUSE:      return "EFUSE";
    case ESP_RST_PWR_GLITCH: return "PWR_GLITCH";
    case ESP_RST_CPU_LOCKUP: return "CPU_LOCKUP";
    default:                 return "UNBEKANNT";
  }
}

static void bootLogSave() {
  if (bootLogOk) bootLogPrefs.putBytes("log", &bootLog, sizeof(bootLog));
}

// Nach Serial.begin() aufrufen: Historie ausgeben, neuen Eintrag anlegen.
void bootDiagInit() {
  bootLogOk = bootLogPrefs.begin("bootlog", false);
  if (!bootLogOk) {
    Serial.println("[Boot] NVS-Namespace 'bootlog' nicht verfuegbar");
    return;
  }
  if (bootLogPrefs.getBytesLength("log") != sizeof(bootLog) ||
      bootLogPrefs.getBytes("log", &bootLog, sizeof(bootLog)) != sizeof(bootLog) ||
      bootLog.count > BOOT_LOG_ENTRIES || bootLog.head >= BOOT_LOG_ENTRIES) {
    memset(&bootLog, 0, sizeof(bootLog));
  }

  bootDiagPrint();   // Historie der bisherigen Starts

  bootLog.head = (bootLog.count == 0) ? 0 : (uint8_t)((bootLog.head + 1) % BOOT_LOG_ENTRIES);
  if (bootLog.count < BOOT_LOG_ENTRIES) bootLog.count++;
  BootLogEntry &cur = bootLog.e[bootLog.head];
  cur.seq    = bootLog.nextSeq++;
  cur.reason = (uint8_t)esp_reset_reason();
  cur.stage  = BOOT_STAGE_START;
  cur.ms     = millis();
  bootLogSave();

  Serial.printf("[Boot] Aktueller Start #%u, Reset-Grund: %s\n",
                cur.seq, resetReasonName(cur.reason));
}

void bootDiagStage(uint8_t stage) {
  if (!bootLogOk || bootLog.count == 0 || stage >= BOOT_STAGE_COUNT) return;
  BootLogEntry &cur = bootLog.e[bootLog.head];
  cur.stage = stage;
  cur.ms    = millis();
  bootLogSave();
}

// Aus loop() aufrufen: markiert, wie lange der Start stabil gelaufen ist.
void bootDiagLoop() {
  static uint8_t next = BOOT_STAGE_RUN_10S;
  static const uint32_t at[] = {10000UL, 60000UL, 600000UL};
  if (next > BOOT_STAGE_RUN_10MIN) return;
  if (millis() >= at[next - BOOT_STAGE_RUN_10S]) {
    bootDiagStage(next);
    next++;
  }
}

// Gibt das gespeicherte Protokoll aus (ältester Eintrag zuerst). Der
// jüngste Eintrag ist nach bootDiagInit() der laufende Start.
void bootDiagPrint() {
  if (!bootLogOk) return;
  Serial.printf("[Boot] ── Startprotokoll (%u Eintraege) ──\n", bootLog.count);
  for (uint8_t i = 0; i < bootLog.count; i++) {
    uint8_t idx = (uint8_t)((bootLog.head + BOOT_LOG_ENTRIES - bootLog.count + 1 + i) % BOOT_LOG_ENTRIES);
    const BootLogEntry &en = bootLog.e[idx];
    Serial.printf("[Boot]  #%-5u Reset: %-10s  zuletzt: %-22s bei %lu ms\n",
                  en.seq, resetReasonName(en.reason),
                  en.stage < BOOT_STAGE_COUNT ? BOOT_STAGE_NAMES[en.stage] : "?",
                  (unsigned long)en.ms);
  }
  Serial.println("[Boot] ────────────────────────────────");
}
