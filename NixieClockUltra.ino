/*
 * ╔══════════════════════════════════════════════════════════════════════╗
 * ║               NIXIE TUBE CLOCK – ESP32-S3 Firmware                   ║
 * ║         6x Nixie | DS1302 RTC | 10x NeoPixel | Web UI                ║
 * ║                                                                      ║
 * ║  Architektur:                                                        ║
 * ║   • Direkte Ansteuerung via 4× MCP23017 (I2C)                        ║
 * ║   • Entprellte Taster (FSM)                                          ║
 * ║   • Fade-In / Fade-Out je Röhre                                      ║
 * ║   • NeoPixel Farbverlauf + Animationen                               ║
 * ║   • Helligkeitssteuerung (4 Stufen)                                  ║
 * ║   • Web-Interface (WiFi AP)                                          ║
 * ║   • IR-Interface                                                     ║
 * ║   • DS1302 RTC Handling + Einstellmodus                              ║
 * ╚══════════════════════════════════════════════════════════════════════╝
 *
 *  Benötigte Bibliotheken (Arduino Library Manager):
 *    - Adafruit NeoPixel   (Adafruit)
 *    - Rtc by Makuna        (Michael C. Miller)   ← DS1302 Support
 *    - AsyncTCP             (me-no-dev)
 *    - ESPAsyncWebServer    (me-no-dev)
 *    - ArduinoJson          (Benoit Blanchon)
 *    - IRremoteESP8266      (David Conran)
 */

// ═══════════════════════════════════════════════════════════
//  INCLUDES
// ═══════════════════════════════════════════════════════════
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <RtcDS1302.h>
#include <ThreeWire.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// IR-Empfänger
#include <IRremoteESP8266.h>
#include <IRrecv.h>

// ═══════════════════════════════════════════════════════════
//  PIN-DEFINITIONEN
// ═══════════════════════════════════════════════════════════

// DS1302
#define RTC_IO_PIN   4
#define RTC_CLK_PIN  5
#define RTC_CE_PIN   2

// Taster (LOW-aktiv, Pull-Up intern)
#define BTN_SET      13
#define BTN_UP       14
#define BTN_DOWN     15
#define BTN_LIGHT    16

// NeoPixel
#define NEO_PIN      21
#define NEO_COUNT    10   // 6 Hintergrund + 4 Doppelpunkte

// IR-Empfänger
#define IR_RECV_PIN  48

// ═══════════════════════════════════════════════════════════
//  KONSTANTEN & KONFIGURATION
// ═══════════════════════════════════════════════════════════

// Fade
#define FADE_STEPS           20   // Schritte für Fade-In/Out
#define FADE_INTERVAL_MS      2   // ms zwischen Fade-Schritten

// Helligkeitsstufen (PWM 0–255 → werden als Duty-Cycle-Anteil genutzt)
const uint8_t BRIGHTNESS_LEVELS[4] = {10, 30, 50, 80};

// WiFi AP (Fallback-Hotspot, immer aktiv)
#define WIFI_SSID   "NixieClock"
#define WIFI_PASS   "nixie1234"

// NTP
#define NTP_SERVER  "pool.ntp.org"
#define NTP_TZ      "CET-1CEST,M3.5.0,M10.5.0/3"  // Mitteleuropäische Zeit

// Einstellmodus Timeout (ms)
#define EDIT_TIMEOUT_MS   15000

// ═══════════════════════════════════════════════════════════
//  OBJEKTE
// ═══════════════════════════════════════════════════════════

ThreeWire    rtcWire(RTC_IO_PIN, RTC_CLK_PIN, RTC_CE_PIN);
RtcDS1302<ThreeWire> Rtc(rtcWire);

Adafruit_NeoPixel strip(NEO_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

AsyncWebServer server(80);

Preferences prefs;

// ═══════════════════════════════════════════════════════════
//  GLOBALE VARIABLEN
// ═══════════════════════════════════════════════════════════

// Anzuzeigende Ziffern [0]=HZ [1]=HE [2]=MZ [3]=ME [4]=SZ [5]=SE
uint8_t displayDigits[6] = {0, 0, 0, 0, 0, 0};

// Helligkeit (Index in BRIGHTNESS_LEVELS)
uint8_t brightLevel = 3;

// NeoPixel Helligkeit (getrennt für Hintergrund- und Trennpunkt-LEDs)
uint8_t  neoBright   = 30;   // Pixel 0–5 (Hintergrund)
uint8_t  colonBright = 15;   // Pixel 6–9 (Trennpunkte)
uint8_t  neoHue      = 0;    // auto-inkrement in Animationen
uint8_t  neoSat      = 255;

// Animations-Modus
enum AnimMode { ANIM_RAINBOW, ANIM_STATIC, ANIM_PULSE, ANIM_SLOTS, ANIM_COUNT };
AnimMode animMode = ANIM_RAINBOW;

// Einstellmodus
enum EditState { EDIT_NONE, EDIT_HOUR, EDIT_MIN, EDIT_SEC };
EditState editState = EDIT_NONE;
unsigned long editEnterTime = 0;

// Uhrzeit (lokal gecacht)
uint8_t  curHour = 0, curMin = 0, curSec = 0;

// WiFi / NTP Status
bool wifiStaConnected = false;
bool ntpSynced        = false;
bool pendingRestart   = false;
unsigned long restartAt = 0;

// Letzter NeoPixel-Update
unsigned long lastNeoUpdate = 0;

// Slot-Machine Animation
bool slotActive = false;
unsigned long slotStartMs = 0;
uint8_t slotTarget[6] = {0};
uint8_t slotCurrent[6] = {0};

// Fade-In beim Start
bool startFadeIn = true;
uint8_t startFadeStep = 0;

// ═══════════════════════════════════════════════════════════
//  IR-FERNBEDIENUNG
// ═══════════════════════════════════════════════════════════
enum IrAction {
  IR_LEARN_NONE           = -1,
  IR_ACTION_SET           = 0,
  IR_ACTION_UP            = 1,
  IR_ACTION_DOWN          = 2,
  IR_ACTION_BRIGHTNESS    = 3,
  IR_ACTION_ANIM_NEXT     = 4,
  IR_ACTION_SLOT          = 5,
  IR_ACTION_COLON_TOGGLE  = 6,
  IR_ACTION_COUNT         = 7
};

const char* IR_ACTION_KEYS[IR_ACTION_COUNT] = {
  "ir_SET", "ir_UP", "ir_DOWN", "ir_BRIGHTNESS",
  "ir_ANIM_NEXT", "ir_SLOT", "ir_COLTOGGLE"
};

const char* IR_ACTION_LABELS[IR_ACTION_COUNT] = {
  "SET", "UP", "DOWN", "BRIGHTNESS",
  "ANIM_NEXT", "SLOT", "COLON_TOGGLE"
};

IRrecv   irrecv(IR_RECV_PIN);
decode_results irResults;

uint64_t irCodes[IR_ACTION_COUNT] = {0};
IrAction irLearnTarget  = IR_LEARN_NONE;
unsigned long irLearnStartMs = 0;
#define IR_LEARN_TIMEOUT_MS 10000
portMUX_TYPE irMux = portMUX_INITIALIZER_UNLOCKED;

// Trennpunkte dauerhaft an (statt Sekundenblinken)
bool colonAlwaysOn = false;
// Trennpunkte statisch warmweiß (ignoriert Animationsfarbe)
bool colonStatic   = false;

// ═══════════════════════════════════════════════════════════
//  TASTER-STRUKTUR (muss vor auto-generierten Prototypen stehen)
// ═══════════════════════════════════════════════════════════
struct Button {
  uint8_t      pin;
  bool         lastState;
  bool         pressed;
  bool         held;
  unsigned long pressTime;
  unsigned long lastRepeat;
  uint16_t     debounceMs;
  uint16_t     repeatMs;
};

Button btnSet   = {BTN_SET,   HIGH, false, false, 0, 0, 30, 400};
Button btnUp    = {BTN_UP,    HIGH, false, false, 0, 0, 30, 200};
Button btnDown  = {BTN_DOWN,  HIGH, false, false, 0, 0, 30, 200};
Button btnLight = {BTN_LIGHT, HIGH, false, false, 0, 0, 30, 500};

// Forward-Deklarationen für Funktionen, die der Arduino-Präprozessor
// nicht automatisch erkennt (Raw-String-Literal in web_server.ino)
void nixieInit();
void setupWifi();
void setupWebServer();

// ═══════════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[NixieClock] Booting...");

  // --- Taster ---
  pinMode(BTN_SET,   INPUT_PULLUP);
  pinMode(BTN_UP,    INPUT_PULLUP);
  pinMode(BTN_DOWN,  INPUT_PULLUP);
  pinMode(BTN_LIGHT, INPUT_PULLUP);

  // --- NeoPixel ---
  strip.begin();
  strip.setBrightness(255);  // Skalierung erfolgt per Pixel in neo_animation
  strip.clear();
  strip.show();

  // --- Preferences laden ---
  prefs.begin("nixie", false);
  brightLevel = prefs.getUChar("bright",    3);
  neoBright   = prefs.getUChar("neoBright",   30);
  colonBright = prefs.getUChar("colonBright", 15);
  animMode    = (AnimMode)prefs.getUChar("animMode", 0);

  for (int i = 0; i < IR_ACTION_COUNT; i++) {
    irCodes[i] = prefs.getULong64(IR_ACTION_KEYS[i], 0);
  }
  colonAlwaysOn    = prefs.getBool("colonOn",     false);
  colonStatic      = prefs.getBool("colonStatic", false);

  // --- Nixie Direct Drive via MCP23017 ---
  nixieInit();    // Wire.begin() muss vor readRTC()/setDisplayTime() stehen

  // --- RTC ---
  Rtc.Begin();
  if (!Rtc.IsDateTimeValid()) {
    Serial.println("[RTC] Nicht initialisiert – setze Defaultzeit");
    RtcDateTime compiled = RtcDateTime(2024, 1, 1, 0, 0, 0);
    Rtc.SetDateTime(compiled);
  }
  Rtc.SetIsWriteProtected(false);
  Rtc.SetIsRunning(true);
  readRTC();
  setDisplayTime(curHour, curMin, curSec);

  // --- WiFi + Web-Server ---
  setupWifi();
  setupWebServer();

  // --- IR-Empfänger ---
  irrecv.enableIRIn();
  Serial.printf("[IR] Empfänger gestartet auf Pin %d\n", IR_RECV_PIN);

  // --- Fade-In Vorbereitung ---
  startFadeIn   = true;
  startFadeStep = 0;

  Serial.println("[NixieClock] Bereit.");
}

// ═══════════════════════════════════════════════════════════
//  LOOP
// ═══════════════════════════════════════════════════════════

uint8_t  lastSec = 255;
uint32_t lastRtcRead = 0;
uint32_t lastFadeMs  = 0;

void loop() {

  // --- Start-Fade-In ---
  if (startFadeIn) {
    if (millis() - lastFadeMs >= FADE_INTERVAL_MS) {
      lastFadeMs = millis();
      // NeoPixel langsam aufblenden
      uint8_t b = map(startFadeStep, 0, FADE_STEPS, 0, neoBright);
      strip.setBrightness(b);
      strip.show();
      startFadeStep++;
      if (startFadeStep >= FADE_STEPS) startFadeIn = false;
    }
    return;
  }

  // --- Taster einlesen ---
  updateButton(btnSet);
  updateButton(btnUp);
  updateButton(btnDown);
  updateButton(btnLight);

  // --- IR ---
  handleIR();

  // --- Helligkeitssteuerung ---
  handleBrightness();

  // --- Einstellmodus ---
  handleEditMode();

  // --- RTC alle 500ms lesen (außerhalb Einstellmodus) ---
  if (editState == EDIT_NONE && millis() - lastRtcRead >= 500) {
    lastRtcRead = millis();
    readRTC();

    if (curSec != lastSec) {
      lastSec = curSec;

      if (animMode == ANIM_SLOTS && curSec % 10 == 0) {
        // Alle 10 Sekunden Slot-Animation
        startSlotAnimation(curHour, curMin, curSec);
      } else if (!slotActive) {
        setDisplayTime(curHour, curMin, curSec);
      }
    }
  }

  // --- Slot-Animation ---
  if (slotActive) updateSlotAnimation();

  // --- NeoPixel ---
  updateNeoPixel();

  // --- NTP → RTC-Sync (einmalig nach Verbindung) ---
  if (wifiStaConnected && !ntpSynced) {
    struct tm ti;
    if (getLocalTime(&ti, 0)) {
      ntpSynced = true;
      RtcDateTime ntpTime(ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday,
                          ti.tm_hour, ti.tm_min, ti.tm_sec);
      Rtc.SetDateTime(ntpTime);
      Serial.println("[NTP] RTC synchronisiert.");
    }
  }

  // --- Neustart nach WiFi-Änderung ---
  if (pendingRestart && millis() >= restartAt) {
    ESP.restart();
  }

  // Kurze Pause um CPU zu entlasten
  delay(1);
}

