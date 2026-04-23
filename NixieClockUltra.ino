/*
 * ╔══════════════════════════════════════════════════════════════════════╗
 * ║               NIXIE TUBE CLOCK – ESP32-S3 Firmware                   ║
 * ║         6x Nixie | DS1302 RTC | 10x NeoPixel | Web UI                ║
 * ║                                                                      ║
 * ║  Architektur:                                                        ║
 * ║   • Multiplexing via ISR (Timer)  → kein Flimmern                    ║
 * ║   • Anti-Ghosting (Blank-Phase)                                      ║
 * ║   • Entprellte Taster (FSM)                                          ║
 * ║   • Fade-In / Fade-Out je Röhre                                      ║
 * ║   • NeoPixel Farbverlauf + Animationen                               ║
 * ║   • Helligkeitssteuerung (4 Stufen)                                  ║
 * ║   • Web-Interface (WiFi AP)                                          ║
 * ║   • Power-Saving (Light-Sleep)                                       ║
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

// Kathoden (Ziffern 0–9) – gemeinsam für alle Röhren
const uint8_t CATHODE_PIN[10] = {47, 17, 18, 38, 39, 40, 41, 42, 45, 46};

// Anoden (Röhren-Auswahl im Multiplex)
const uint8_t ANODE_PIN[6]    = {16, 15, 14, 13, 12, 11};
//                                HZ  HE  MZ  ME  SZ  SE

// DS1302
#define RTC_IO_PIN   4
#define RTC_CLK_PIN  5
#define RTC_CE_PIN   2

// Taster (LOW-aktiv, Pull-Up intern)
#define BTN_SET      10
#define BTN_UP        9
#define BTN_DOWN      8
#define BTN_LIGHT     7

// NeoPixel
#define NEO_PIN      21
#define NEO_COUNT    10   // 6 Hintergrund + 4 Doppelpunkte

// IR-Empfänger
#define IR_RECV_PIN  48

// ═══════════════════════════════════════════════════════════
//  KONSTANTEN & KONFIGURATION
// ═══════════════════════════════════════════════════════════

// Multiplex-Timing (µs)
#define MUX_DIGIT_US       1800   // Zeit pro aktiver Röhre
#define MUX_BLANK_US        200   // Anti-Ghosting Blanking-Phase

// Fade
#define FADE_STEPS           20   // Schritte für Fade-In/Out
#define FADE_INTERVAL_MS      2   // ms zwischen Fade-Schritten

// Helligkeitsstufen (PWM 0–255 → werden als Duty-Cycle-Anteil genutzt)
const uint8_t BRIGHTNESS_LEVELS[4] = {30, 80, 160, 255};

// WiFi AP (Fallback-Hotspot, immer aktiv)
#define WIFI_SSID   "NixieClock"
#define WIFI_PASS   "nixie1234"

// NTP
#define NTP_SERVER  "pool.ntp.org"
#define NTP_TZ      "CET-1CEST,M3.5.0,M10.5.0/3"  // Mitteleuropäische Zeit

// Einstellmodus Timeout (ms)
#define EDIT_TIMEOUT_MS   15000

// Power-Save: Dimm-Schwelle (Sekunden ohne Tastendruck)
#define POWER_SAVE_TIMEOUT_S  120

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
volatile uint8_t displayDigits[6] = {0, 0, 0, 0, 0, 0};

// Fade-Werte je Röhre (0–255)
volatile uint8_t fadeBrightness[6] = {255, 255, 255, 255, 255, 255};

// Aktueller Multiplex-Index
volatile uint8_t muxIndex = 0;

// Blanking-Flag (Anti-Ghosting)
volatile bool    inBlank  = false;

// Helligkeit (Index in BRIGHTNESS_LEVELS)
uint8_t brightLevel = 3;

// NeoPixel Farbverlauf
uint8_t  neoHue    = 0;
uint8_t  neoSat    = 255;
uint8_t  neoBright = 80;

// Animations-Modus
enum AnimMode { ANIM_RAINBOW, ANIM_STATIC, ANIM_PULSE, ANIM_SLOTS, ANIM_COUNT };
AnimMode animMode = ANIM_RAINBOW;

// Einstellmodus
enum EditState { EDIT_NONE, EDIT_HOUR, EDIT_MIN, EDIT_SEC };
EditState editState = EDIT_NONE;
unsigned long editEnterTime = 0;

// Uhrzeit (lokal gecacht)
uint8_t  curHour = 0, curMin = 0, curSec = 0;

// Letzter Tastendruck (Power-Save)
unsigned long lastInteractionMs = 0;
bool          powerSaveActive   = false;

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

// Hardware-Timer
hw_timer_t *muxTimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

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
  IR_ACTION_POWER_SAVE_TOGGLE = 6,
  IR_ACTION_COUNT         = 7
};

const char* IR_ACTION_KEYS[IR_ACTION_COUNT] = {
  "ir_SET", "ir_UP", "ir_DOWN", "ir_BRIGHTNESS",
  "ir_ANIM_NEXT", "ir_SLOT", "ir_PSTOGGLE"
};

const char* IR_ACTION_LABELS[IR_ACTION_COUNT] = {
  "SET", "UP", "DOWN", "BRIGHTNESS",
  "ANIM_NEXT", "SLOT", "POWER_SAVE_TOGGLE"
};

IRrecv   irrecv(IR_RECV_PIN);
decode_results irResults;

uint64_t irCodes[IR_ACTION_COUNT] = {0};
IrAction irLearnTarget  = IR_LEARN_NONE;
unsigned long irLearnStartMs = 0;
#define IR_LEARN_TIMEOUT_MS 10000
portMUX_TYPE irMux = portMUX_INITIALIZER_UNLOCKED;

// Power-Save dauerhaft aktiviert/deaktiviert
bool powerSaveEnabled = true;

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

    case IR_ACTION_POWER_SAVE_TOGGLE:
      powerSaveEnabled = !powerSaveEnabled;
      prefs.putBool("psEnabled", powerSaveEnabled);
      if (!powerSaveEnabled) powerSaveActive = false;
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
      lastInteractionMs = millis();
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

// ═══════════════════════════════════════════════════════════
//  WEB-INTERFACE
// ═══════════════════════════════════════════════════════════

// HTML komprimiert als PROGMEM
const char WEB_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Nixie Clock</title>
<style>
  :root{--bg:#0d0d0d;--card:#1a1a1a;--accent:#ff8c00;--text:#f0e6d0;--dim:#888}
  *{box-sizing:border-box;margin:0;padding:0}
  body{background:var(--bg);color:var(--text);font-family:'Courier New',monospace;min-height:100vh;display:flex;flex-direction:column;align-items:center;padding:20px}
  h1{font-size:2em;color:var(--accent);letter-spacing:.3em;margin:20px 0;text-shadow:0 0 20px #ff8c0088}
  .clock{font-size:3em;letter-spacing:.2em;color:var(--accent);text-shadow:0 0 30px #ff8c00aa;margin:10px 0 30px}
  .card{background:var(--card);border:1px solid #333;border-radius:12px;padding:20px;width:100%;max-width:480px;margin-bottom:16px}
  .card h2{font-size:1em;color:var(--dim);letter-spacing:.2em;margin-bottom:16px;text-transform:uppercase}
  .row{display:flex;align-items:center;justify-content:space-between;margin-bottom:12px}
  label{color:var(--dim);font-size:.85em}
  input[type=number],input[type=text],select{background:#111;border:1px solid #444;color:var(--text);padding:6px 10px;border-radius:6px;font-family:inherit;font-size:.9em;width:100px}
  input[type=range]{width:140px;accent-color:var(--accent)}
  button{background:var(--accent);color:#000;border:none;border-radius:8px;padding:8px 20px;font-family:inherit;font-size:.9em;font-weight:bold;cursor:pointer;letter-spacing:.1em;transition:.2s}
  button:hover{filter:brightness(1.2)}
  button.sec{background:#333;color:var(--text)}
  .status{font-size:.75em;color:var(--dim);text-align:center;margin-top:8px}
  .badge{display:inline-block;background:#ff8c0022;border:1px solid var(--accent);border-radius:4px;padding:2px 8px;font-size:.75em;color:var(--accent)}
  @keyframes blink{0%,100%{background:var(--card)}50%{background:#3a2800}}
  .toggle{position:relative;display:inline-block;width:42px;height:24px}
  .toggle input{opacity:0;width:0;height:0}
  .slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#333;border-radius:24px;transition:.3s}
  .slider:before{content:'';position:absolute;height:18px;width:18px;left:3px;bottom:3px;background:#666;border-radius:50%;transition:.3s}
  .toggle input:checked+.slider{background:var(--accent)}
  .toggle input:checked+.slider:before{transform:translateX(18px);background:#000}
</style>
</head>
<body>
<h1>⬡ NIXIE CLOCK</h1>
<div class="clock" id="clkDisp">--:--:--</div>

<div class="card">
  <h2>⏱ Zeit stellen</h2>
  <div class="row"><label>Stunde</label><input type="number" id="ih" min="0" max="23" value="0"></div>
  <div class="row"><label>Minute</label><input type="number" id="im" min="0" max="59" value="0"></div>
  <div class="row"><label>Sekunde</label><input type="number" id="is" min="0" max="59" value="0"></div>
  <div class="row">
    <button onclick="setTime()">Übernehmen</button>
    <button class="sec" onclick="syncBrowser()">Browser-Zeit</button>
  </div>
</div>

<div class="card">
  <h2>💡 Helligkeit &amp; Animation</h2>
  <div class="row">
    <label>Nixie-Helligkeit</label>
    <select id="bright" onchange="setBright()">
      <option value="0">Sehr dim</option>
      <option value="1">Dim</option>
      <option value="2">Hell</option>
      <option value="3" selected>Voll</option>
    </select>
  </div>
  <div class="row">
    <label>NeoPixel-Helligkeit</label>
    <input type="range" id="neoBright" min="10" max="255" value="80" oninput="setNeoBright(this.value)">
    <span id="neoBrightVal">80</span>
  </div>
  <div class="row">
    <label>Animation</label>
    <select id="anim" onchange="setAnim()">
      <option value="0">Rainbow</option>
      <option value="1">Statisch Warmweiß</option>
      <option value="2">Puls</option>
      <option value="3">Slot-Machine</option>
    </select>
  </div>
  <div class="row">
    <label>NeoPixel-Farbe (Hue)</label>
    <input type="range" id="neoHue" min="0" max="255" value="0" oninput="setNeoHue(this.value)">
    <span id="neoHueVal">0</span>
  </div>
  <div class="row">
    <label>Power-Save (Auto-Dimmen)</label>
    <label class="toggle"><input type="checkbox" id="psEnabled" onchange="setPowerSave(this.checked)"><span class="slider"></span></label>
  </div>
</div>

<div class="card">
  <h2>🎰 Slot-Animation starten</h2>
  <div class="row">
    <button onclick="triggerSlot()">Slot-Machine!</button>
    <span class="badge" id="slotStatus">bereit</span>
  </div>
</div>

<div class="card">
  <h2>📡 IR-Fernbedienung</h2>
  <table style="width:100%;border-collapse:collapse">
    <thead><tr>
      <th style="text-align:left;color:var(--dim);font-size:.8em;padding:4px 0;width:45%">Funktion</th>
      <th style="text-align:left;color:var(--dim);font-size:.8em;padding:4px 0">Code</th>
      <th></th>
    </tr></thead>
    <tbody id="irRows"></tbody>
  </table>
  <p id="irMsg" style="font-size:.75em;color:var(--dim);margin-top:8px;min-height:1.2em"></p>
</div>

<div class="card">
  <h2>📶 WLAN-Einrichtung</h2>
  <div class="row"><label>Modus</label><span id="wifiMode" class="badge">--</span></div>
  <div id="wifiSta" style="display:none">
    <div class="row"><label>Heimnetz</label><span id="wifiStaSsid" class="badge">--</span></div>
    <div class="row"><label>IP-Adresse</label><span id="wifiStaIp" class="badge">--</span></div>
    <div class="row"><label>NTP-Sync</label><span id="wifiNtp" class="badge">nein</span></div>
  </div>
  <div class="row"><label>SSID</label><input type="text" id="wSsid" placeholder="Netzwerkname" style="width:160px"></div>
  <div class="row"><label>Passwort</label><input type="password" id="wPass" placeholder="Passwort" style="width:160px"></div>
  <div class="row">
    <button onclick="wSave()">Verbinden</button>
    <button class="sec" onclick="wScan()">Scannen</button>
  </div>
  <select id="wNets" size="4" style="display:none;width:100%;background:#111;border:1px solid #444;color:var(--text);border-radius:6px;margin-top:8px;font-family:inherit" onchange="document.getElementById('wSsid').value=this.value"></select>
  <p style="font-size:.75em;color:var(--dim);margin-top:8px">Leer lassen um WLAN zu entfernen. Neustart erfolgt automatisch.</p>
</div>

<div class="status" id="statusMsg">Verbunden ✓</div>

<script>
function api(path,body){
  let opts={method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)};
  return fetch(path,opts).then(r=>r.json()).catch(e=>({error:e.message}));
}
function get(path){return fetch(path).then(r=>r.json()).catch(()=>({}));}

async function refreshClock(){
  let d=await get('/api/status');
  if(d.time) document.getElementById('clkDisp').textContent=d.time;
  if(d.bright!==undefined) document.getElementById('bright').value=d.bright;
  if(d.neoBright!==undefined){document.getElementById('neoBright').value=d.neoBright;document.getElementById('neoBrightVal').textContent=d.neoBright;}
  if(d.anim!==undefined) document.getElementById('anim').value=d.anim;
  if(d.psEnabled!==undefined)document.getElementById('psEnabled').checked=d.psEnabled;
  if(d.slot!==undefined) document.getElementById('slotStatus').textContent=d.slot?'läuft…':'bereit';
}

async function setTime(){
  let h=parseInt(document.getElementById('ih').value);
  let m=parseInt(document.getElementById('im').value);
  let s=parseInt(document.getElementById('is').value);
  let r=await api('/api/settime',{h,m,s});
  document.getElementById('statusMsg').textContent=r.ok?'Zeit gesetzt ✓':'Fehler ✗';
}
function syncBrowser(){
  let now=new Date();
  document.getElementById('ih').value=now.getHours();
  document.getElementById('im').value=now.getMinutes();
  document.getElementById('is').value=now.getSeconds();
  setTime();
}
async function setBright(){
  let v=parseInt(document.getElementById('bright').value);
  await api('/api/bright',{level:v});
}
async function setNeoBright(v){
  document.getElementById('neoBrightVal').textContent=v;
  await api('/api/neobright',{val:parseInt(v)});
}
async function setAnim(){
  let v=parseInt(document.getElementById('anim').value);
  await api('/api/anim',{mode:v});
}
async function setNeoHue(v){
  document.getElementById('neoHueVal').textContent=v;
  await api('/api/neohue',{hue:parseInt(v)});
}
async function triggerSlot(){
  await api('/api/slot',{});
  document.getElementById('slotStatus').textContent='läuft…';
}

async function refreshWifi(){
  let d=await get('/api/wifi');
  if(!d.mode)return;
  document.getElementById('wifiMode').textContent=d.mode==='sta'?'STA+AP':'AP';
  let sta=document.getElementById('wifiSta');
  if(d.mode==='sta'){
    sta.style.display='';
    document.getElementById('wifiStaSsid').textContent=d.staSsid||'--';
    document.getElementById('wifiStaIp').textContent=d.staIp||'--';
    document.getElementById('wifiNtp').textContent=d.ntp?'ja ✓':'warte...';
  } else {
    sta.style.display='none';
  }
}
async function wSave(){
  let s=document.getElementById('wSsid').value.trim();
  let p=document.getElementById('wPass').value;
  document.getElementById('statusMsg').textContent='Gespeichert – Neustart läuft…';
  await api('/api/wifi',{ssid:s,pass:p});
}
async function wScan(){
  let sel=document.getElementById('wNets');
  sel.style.display='';
  sel.innerHTML='<option disabled>Suche Netzwerke…</option>';
  let nets=await get('/api/wifi/scan');
  sel.innerHTML='';
  if(!Array.isArray(nets)||!nets.length){sel.innerHTML='<option disabled>Keine Netzwerke gefunden</option>';return;}
  nets.sort((a,b)=>b.rssi-a.rssi).forEach(n=>{
    let o=document.createElement('option');
    o.value=n.ssid;
    o.textContent=n.ssid+' ('+n.rssi+' dBm)'+(n.enc?' 🔒':'');
    sel.appendChild(o);
  });
}

const IR_ACTIONS=['SET','UP','DOWN','BRIGHTNESS','ANIM_NEXT','SLOT','POWER_SAVE_TOGGLE'];
const IR_LABELS={'SET':'SET &#x2013; Einstellmodus','UP':'UP &#x2013; Erh&ouml;hen','DOWN':'DOWN &#x2013; Verringern',
  'BRIGHTNESS':'BRIGHTNESS &#x2013; Helligkeit','ANIM_NEXT':'ANIM &#x2013; n&auml;chste Animation',
  'SLOT':'SLOT &#x2013; Slot-Maschine','POWER_SAVE_TOGGLE':'POWER SAVE &#x2013; Toggle'};
let irPollTimer=null;

async function refreshIR(){
  let d=await get('/api/ir/status');
  if(!d.codes)return;
  let tbody=document.getElementById('irRows');
  tbody.innerHTML='';
  IR_ACTIONS.forEach(a=>{
    let code=d.codes[a]||'';
    let learning=(d.learning===a);
    let tr=document.createElement('tr');
    tr.id='irr-'+a;
    if(learning)tr.style.animation='blink 0.6s infinite';
    let td1=document.createElement('td');td1.style.cssText='padding:4px 8px 4px 0;font-size:.82em';td1.textContent=IR_LABELS[a];
    let td2=document.createElement('td');td2.style.cssText='padding:4px 8px;font-size:.8em;color:'+(code?'var(--accent)':'var(--dim)');td2.textContent=code||'—';
    let td3=document.createElement('td');td3.style.cssText='white-space:nowrap';
    td3.innerHTML='<button class="sec" style="padding:3px 9px;font-size:.78em" onclick="irLearn(\''+a+'\')">'+( learning?'warte&#x2026;':'Anlernen')+'</button> '+
      '<button class="sec" style="padding:3px 8px;font-size:.78em" onclick="irClear(\''+a+'\')">&#x2715;</button>';
    tr.appendChild(td1);tr.appendChild(td2);tr.appendChild(td3);
    tbody.appendChild(tr);
  });
  if(d.learning){
    document.getElementById('irMsg').textContent='Taste auf Fernbedienung drücken… (10 s Timeout)';
    if(!irPollTimer)irPollTimer=setInterval(async()=>{let s=await get('/api/ir/status');refreshIR();if(!s.learning){clearInterval(irPollTimer);irPollTimer=null;}},500);
  }else{
    document.getElementById('irMsg').textContent='';
    if(irPollTimer){clearInterval(irPollTimer);irPollTimer=null;}
  }
}

async function irLearn(action){
  await api('/api/ir/learn',{action});
  refreshIR();
}
async function irClear(action){
  await api('/api/ir/clear',{action});
  refreshIR();
}
async function setPowerSave(enabled){
  await api('/api/powersave',{enabled});
}

setInterval(refreshClock,1000);
refreshClock();
refreshWifi();
refreshIR();
</script>
</body>
</html>
)rawliteral";

// ═══════════════════════════════════════════════════════════
//  WIFI-SETUP (AP immer aktiv; STA wenn Zugangsdaten gespeichert)
// ═══════════════════════════════════════════════════════════
void setupWifi() {
  String savedSsid = prefs.getString("wifiSsid", "");
  String savedPass = prefs.getString("wifiPass", "");

  // Immer AP starten (Fallback-Hotspot)
  WiFi.mode(savedSsid.length() > 0 ? WIFI_AP_STA : WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WiFi] AP gestartet: %s  IP: %s\n", WIFI_SSID, WiFi.softAPIP().toString().c_str());

  if (savedSsid.length() > 0) {
    WiFi.begin(savedSsid.c_str(), savedPass.c_str());
    Serial.printf("[WiFi] Verbinde mit '%s'...\n", savedSsid.c_str());
    unsigned long t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < 10000) {
      delay(200);
    }
    if (WiFi.status() == WL_CONNECTED) {
      wifiStaConnected = true;
      Serial.printf("[WiFi] STA verbunden. IP: %s\n", WiFi.localIP().toString().c_str());
      configTzTime(NTP_TZ, NTP_SERVER);
      Serial.println("[NTP] Synchronisierung gestartet...");
    } else {
      Serial.println("[WiFi] STA-Verbindung fehlgeschlagen – nur AP aktiv.");
    }
  }
}

void setupWebServer() {

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
    req->send_P(200, "text/html", WEB_PAGE);
  });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *req) {
    StaticJsonDocument<256> doc;
    char buf[10];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", curHour, curMin, curSec);
    doc["time"]      = buf;
    doc["bright"]    = brightLevel;
    doc["neoBright"] = neoBright;
    doc["anim"]      = (int)animMode;
    doc["slot"]      = slotActive;
    doc["psEnabled"] = powerSaveEnabled;
    String out; serializeJson(doc, out);
    req->send(200, "application/json", out);
  });

  server.on("/api/settime", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      StaticJsonDocument<128> doc;
      if (!deserializeJson(doc, data, len)) {
        curHour = constrain((int)doc["h"], 0, 23);
        curMin  = constrain((int)doc["m"], 0, 59);
        curSec  = constrain((int)doc["s"], 0, 59);
        writeRTC();
        setDisplayTime(curHour, curMin, curSec);
      }
      req->send(200, "application/json", "{\"ok\":true}");
    }
  );

  server.on("/api/bright", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      StaticJsonDocument<64> doc;
      if (!deserializeJson(doc, data, len)) {
        brightLevel = constrain((int)doc["level"], 0, 3);
        neoBright   = BRIGHTNESS_LEVELS[brightLevel];
        prefs.putUChar("bright", brightLevel);
      }
      req->send(200, "application/json", "{\"ok\":true}");
    }
  );

  server.on("/api/neobright", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      StaticJsonDocument<64> doc;
      if (!deserializeJson(doc, data, len)) {
        neoBright = constrain((int)doc["val"], 10, 255);
        prefs.putUChar("neoBright", neoBright);
      }
      req->send(200, "application/json", "{\"ok\":true}");
    }
  );

  server.on("/api/anim", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      StaticJsonDocument<64> doc;
      if (!deserializeJson(doc, data, len)) {
        int m = constrain((int)doc["mode"], 0, (int)ANIM_COUNT - 1);
        animMode = (AnimMode)m;
        prefs.putUChar("animMode", m);
      }
      req->send(200, "application/json", "{\"ok\":true}");
    }
  );

  server.on("/api/neohue", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      StaticJsonDocument<64> doc;
      if (!deserializeJson(doc, data, len)) {
        neoHue = (uint8_t)doc["hue"];
      }
      req->send(200, "application/json", "{\"ok\":true}");
    }
  );

  server.on("/api/slot", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      startSlotAnimation(curHour, curMin, curSec);
      req->send(200, "application/json", "{\"ok\":true}");
    }
  );

  // --- WiFi Status ---
  server.on("/api/wifi", HTTP_GET, [](AsyncWebServerRequest *req) {
    StaticJsonDocument<256> doc;
    doc["mode"]   = wifiStaConnected ? "sta" : "ap";
    doc["apSsid"] = WIFI_SSID;
    doc["apIp"]   = WiFi.softAPIP().toString();
    if (wifiStaConnected) {
      doc["staSsid"] = WiFi.SSID();
      doc["staIp"]   = WiFi.localIP().toString();
      doc["ntp"]     = ntpSynced;
    }
    String out; serializeJson(doc, out);
    req->send(200, "application/json", out);
  });

  // --- WiFi Zugangsdaten speichern ---
  server.on("/api/wifi", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      StaticJsonDocument<256> doc;
      if (!deserializeJson(doc, data, len)) {
        String ssid = doc["ssid"].as<String>();
        String pass = doc["pass"].as<String>();
        if (ssid.length() > 0) {
          prefs.putString("wifiSsid", ssid);
          prefs.putString("wifiPass", pass);
        } else {
          prefs.remove("wifiSsid");
          prefs.remove("wifiPass");
        }
        req->send(200, "application/json", "{\"ok\":true}");
        pendingRestart = true;
        restartAt = millis() + 800;
      } else {
        req->send(400, "application/json", "{\"ok\":false}");
      }
    }
  );

  // --- WLAN-Scan ---
  server.on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *req) {
    int n = WiFi.scanNetworks();
    DynamicJsonDocument doc(2048);
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < n && i < 20; i++) {
      JsonObject net = arr.createNestedObject();
      net["ssid"] = WiFi.SSID(i);
      net["rssi"] = WiFi.RSSI(i);
      net["enc"]  = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }
    WiFi.scanDelete();
    String out; serializeJson(doc, out);
    req->send(200, "application/json", out);
  });

  // --- IR: Status ---
  server.on("/api/ir/status", HTTP_GET, [](AsyncWebServerRequest *req) {
    StaticJsonDocument<512> doc;
    if (irLearnTarget != IR_LEARN_NONE) {
      doc["learning"] = IR_ACTION_LABELS[(int)irLearnTarget];
    } else {
      doc["learning"] = "";
    }
    JsonObject codes = doc.createNestedObject("codes");
    for (int i = 0; i < IR_ACTION_COUNT; i++) {
      if (irCodes[i] != 0) {
        char buf[12];
        snprintf(buf, sizeof(buf), "0x%08X", (uint32_t)irCodes[i]);
        codes[IR_ACTION_LABELS[i]] = buf;
      } else {
        codes[IR_ACTION_LABELS[i]] = "";
      }
    }
    String out; serializeJson(doc, out);
    req->send(200, "application/json", out);
  });

  // --- IR: Anlernen starten ---
  server.on("/api/ir/learn", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      StaticJsonDocument<64> doc;
      if (!deserializeJson(doc, data, len)) {
        String action = doc["action"].as<String>();
        for (int i = 0; i < IR_ACTION_COUNT; i++) {
          if (action == IR_ACTION_LABELS[i]) {
            portENTER_CRITICAL(&irMux);
            irLearnTarget  = (IrAction)i;
            irLearnStartMs = millis();
            portEXIT_CRITICAL(&irMux);
            req->send(200, "application/json", "{\"ok\":true}");
            return;
          }
        }
      }
      req->send(400, "application/json", "{\"ok\":false}");
    }
  );

  // --- IR: Code löschen ---
  server.on("/api/ir/clear", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      StaticJsonDocument<64> doc;
      if (!deserializeJson(doc, data, len)) {
        String action = doc["action"].as<String>();
        for (int i = 0; i < IR_ACTION_COUNT; i++) {
          if (action == IR_ACTION_LABELS[i]) {
            portENTER_CRITICAL(&irMux);
            irCodes[i] = 0;
            portEXIT_CRITICAL(&irMux);
            prefs.putULong64(IR_ACTION_KEYS[i], 0);
            req->send(200, "application/json", "{\"ok\":true}");
            return;
          }
        }
      }
      req->send(400, "application/json", "{\"ok\":false}");
    }
  );

  // --- Power-Save Toggle ---
  server.on("/api/powersave", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      StaticJsonDocument<64> doc;
      if (!deserializeJson(doc, data, len)) {
        powerSaveEnabled = (bool)doc["enabled"];
        prefs.putBool("psEnabled", powerSaveEnabled);
        if (!powerSaveEnabled) powerSaveActive = false;
        req->send(200, "application/json", "{\"ok\":true}");
      } else {
        req->send(400, "application/json", "{\"ok\":false}");
      }
    }
  );

  server.begin();
}

// ═══════════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[NixieClock] Booting...");

  // --- Pins: Kathoden ---
  for (int i = 0; i < 10; i++) {
    pinMode(CATHODE_PIN[i], OUTPUT);
    digitalWrite(CATHODE_PIN[i], LOW);
  }
  // --- Pins: Anoden ---
  for (int i = 0; i < 6; i++) {
    pinMode(ANODE_PIN[i], OUTPUT);
    digitalWrite(ANODE_PIN[i], LOW);
  }

  // --- Taster ---
  pinMode(BTN_SET,   INPUT_PULLUP);
  pinMode(BTN_UP,    INPUT_PULLUP);
  pinMode(BTN_DOWN,  INPUT_PULLUP);
  pinMode(BTN_LIGHT, INPUT_PULLUP);

  // --- NeoPixel ---
  strip.begin();
  strip.setBrightness(80);
  strip.clear();
  strip.show();

  // --- Preferences laden ---
  prefs.begin("nixie", false);
  brightLevel = prefs.getUChar("bright",    3);
  neoBright   = prefs.getUChar("neoBright", 80);
  animMode    = (AnimMode)prefs.getUChar("animMode", 0);

  for (int i = 0; i < IR_ACTION_COUNT; i++) {
    irCodes[i] = prefs.getULong64(IR_ACTION_KEYS[i], 0);
  }
  powerSaveEnabled = prefs.getBool("psEnabled", true);

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

  // --- Multiplex-Timer (Timer 0, 1 MHz Basis) ---
  muxTimer = timerBegin(1000000);  // 1 MHz = 1 µs Auflösung
  timerAttachInterrupt(muxTimer, &onMuxTimer);
  timerAlarm(muxTimer, MUX_BLANK_US, false, 0);

  // --- WiFi + Web-Server ---
  setupWifi();
  setupWebServer();

  // --- IR-Empfänger ---
  irrecv.enableIRIn();
  Serial.printf("[IR] Empfänger gestartet auf Pin %d\n", IR_RECV_PIN);

  // --- Fade-In Vorbereitung ---
  startFadeIn   = true;
  startFadeStep = 0;

  lastInteractionMs = millis();
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

  // --- Power-Save ---
  handlePowerSave();

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

/*
 * ══════════════════════════════════════════════════════════
 *  HINWEISE ZUR HARDWARE / INBETRIEBNAHME
 * ══════════════════════════════════════════════════════════
 *
 *  1) Benötigte Bibliotheken im Arduino Library Manager:
 *     - "Adafruit NeoPixel" von Adafruit
 *     - "Rtc by Makuna" von Michael C. Miller
 *     - "AsyncTCP" von me-no-dev
 *     - "ESPAsyncWebServer" von me-no-dev
 *     - "ArduinoJson" von Benoit Blanchon
 *
 *  2) Board: "ESP32S3 Dev Module"
 *     Upload Speed: 921600, Flash: 4MB, Partition: Default
 *
 *  3) Web-Interface:
 *     - Mit WLAN "NixieClock" verbinden (Passwort: nixie1234)
 *     - Browser: http://192.168.4.1
 *
 *  4) Anpassen des WiFi-Passworts:
 *     #define WIFI_PASS "IhrPasswort"
 *
 *  5) Anti-Ghosting:
 *     MUX_BLANK_US kann bei sichtbarem Ghosting erhöht werden (z.B. 300).
 *     MUX_DIGIT_US entsprechend anpassen für optimale Helligkeit.
 *
 *  6) Kathoden-Wert 10:
 *     Wenn displayDigits[i] == 10, wird keine Kathode geschaltet
 *     (wird für Blink-Effekt im Einstellmodus genutzt).
 *     Sicherstellen, dass CATHODE_PIN[10] nicht existiert –
 *     der ISR prüft dies nicht (Array-Überläufe vermeiden):
 *     Im ISR ist uint8_t digit = displayDigits[muxIndex];
 *     und dann digitalWrite(CATHODE_PIN[digit], HIGH).
 *     Füge daher folgenden Guard in der ISR ein oder stelle
 *     sicher, dass displayDigits[i] nur 0–9 enthält im
 *     Normalbetrieb. Im Blink-Modus wird bereits die Anode
 *     nicht aktiviert, da handleEditMode() beide Digits auf
 *     10 setzt und die Anode trotzdem eingeschaltet wird –
 *     passe handleEditMode() an, indem du bei digit==10
 *     auch die Anode LOW lässt (ISR-Guard unten eingebaut).
 *
 *  SICHERHEIT: Die Nixie-Hochspannung (170–180V DC) ist
 *  lebensgefährlich! Kondensatoren entladen vor Arbeiten
 *  an der Schaltung!
 * ══════════════════════════════════════════════════════════

  ┌────────────────┬────────────────────────────────────────────────────────────────────────────────────────────┐  
  │    Feature     │                                          Details                                           │  
  ├────────────────┼────────────────────────────────────────────────────────────────────────────────────────────┤
  │                │ Runs before the web server. Always starts the AP (NixieClock/nixie1234). If home-WiFi      │
  │ setupWifi()    │ credentials are saved in flash, also connects in STA mode (WIFI_AP_STA). 10-second         │
  │                │ timeout, then continues with AP-only.                                                      │  
  ├────────────────┼────────────────────────────────────────────────────────────────────────────────────────────┤
  │ NTP sync       │ After STA connects, configTzTime() starts background NTP. In loop(), once getLocalTime()   │  
  │                │ succeeds, the RTC is updated once and ntpSynced is set.                                    │  
  ├────────────────┼────────────────────────────────────────────────────────────────────────────────────────────┤
  │ /api/wifi GET  │ Returns current mode (sta/ap), home network SSID, IP, and NTP status.                      │  
  ├────────────────┼────────────────────────────────────────────────────────────────────────────────────────────┤  
  │ /api/wifi POST │ Saves {ssid, pass} to NVS (Preferences). Empty SSID clears credentials. Reboots after 800  │
  │                │ ms.                                                                                        │  
  ├────────────────┼────────────────────────────────────────────────────────────────────────────────────────────┤  
  │ /api/wifi/scan │ Blocks ~2 s while scanning, returns sorted list with SSID, RSSI, encryption flag.          │
  ├────────────────┼────────────────────────────────────────────────────────────────────────────────────────────┤  
  │ Web UI card    │ Shows current mode/IP/NTP status. Scan populates a dropdown → click to fill the SSID       │  
  │                │ field. Save triggers restart.                                                              │  
  └────────────────┴────────────────────────────────────────────────────────────────────────────────────────────┘ 


  Beschreibung der Firmware – NixieClockUltra                                                                      
                                                                                                                   
  Überblick                                                                                                        
                                                                                                                   
  Die Firmware läuft auf einem ESP32-S3 und steuert eine Nixie-Röhren-Uhr mit 6 Nixie-Röhren, 10 NeoPixel-LEDs und 
  einer DS1302-Echtzeituhr (RTC). Die gesamte Logik ist in einer einzigen .ino-Datei organisiert und teilt sich in
  mehrere klar abgegrenzte Blöcke auf.                                                                             
                  
  ---
  1. Hardware-Ansteuerung der Nixie-Röhren (Multiplexing)
                                                         
  Da alle 6 Röhren dieselben 10 Kathoden-Pins (je eine pro Ziffer 0–9) teilen, wird ein Multiplex-Verfahren
  eingesetzt: Es ist immer nur eine Röhre gleichzeitig aktiv. Ein Hardware-Timer feuert als Interrupt Service      
  Routine (ISR) und schaltet in einem Zweiphasen-Rhythmus:
                                                                                                                   
  - Blank-Phase (200 µs): Alle Pins werden abgeschaltet – das verhindert sogenanntes "Ghosting", also schwaches    
  Nachleuchten falscher Ziffern.
  - Digit-Phase (1800 µs): Die richtige Anode (Röhre) und die richtige Kathode (Ziffer) werden eingeschaltet.      
                                                                                                                   
  Durch den schnellen Wechsel (ca. 925 Hz Gesamt-Zyklusrate) nimmt das menschliche Auge alle 6 Röhren als          
  gleichzeitig leuchtend wahr.                                                                                     
                                                                                                                   
  ---             
  2. Taster-Entprellung
                                                                                                                   
  Vier Taster (SET, UP, DOWN, LIGHT) werden in einer Button-Struktur verwaltet. Jeder Taster hat konfigurierbare
  Entprellzeit (30 ms) sowie eine Wiederholrate für Dauerdruck ("Auto-Repeat"). Die Logik erzeugt saubere          
  Einzel-Ereignisse (pressed) und Dauerhalte-Ereignisse (held).
                                                                                                                   
  ---             
  3. Echtzeituhr (DS1302 RTC)
                                                                                                                   
  Der DS1302 wird über das 3-Draht-Interface (ThreeWire) angesteuert. Beim Start wird geprüft, ob die RTC gültige
  Zeit enthält – falls nicht, wird eine Standardzeit (1. Januar 2024) gesetzt. Im Normalbetrieb liest die Firmware 
  die aktuelle Zeit alle 500 ms aus der RTC.
                                                                                                                   
  ---             
  4. Einstellmodus (endlicher Automat / FSM)
                                                                                                                   
  Durch Drücken des SET-Tasters durchläuft man eine Zustandsmaschine:
                                                                                                                   
  Kein Einstellmodus → Stunden einstellen → Minuten einstellen → Sekunden einstellen → Speichern                   
                                                                                                                   
  Die aktuell zu ändernde Stelle blinkt (250 ms-Intervall, Röhre wird einfach ausgeblendet). UP/DOWN ändern den    
  Wert mit Auto-Repeat. Nach 15 Sekunden ohne Eingabe wird der Modus automatisch beendet und die Zeit gespeichert.
                                                                                                                   
  ---                                                                                                              
  5. NeoPixel-Animationen
                                                                                                                   
  10 WS2812B-LEDs (6 Hintergrundlichter unter den Röhren, 4 für die Doppelpunkte) werden alle 20 ms aktualisiert.
  Es gibt vier Animationsmodi:                                                                                     
   
  ┌───────────────────┬─────────────────────────────────────────────────────────────────────────────────────────┐  
  │       Modus       │                                      Beschreibung                                       │
  ├───────────────────┼─────────────────────────────────────────────────────────────────────────────────────────┤
  │ Rainbow           │ Farbverlauf wandert über die 6 Hintergrund-LEDs, Doppelpunkte blinken sekundentaktgenau │
  ├───────────────────┼─────────────────────────────────────────────────────────────────────────────────────────┤
  │ Statisch Warmweiß │ Alle LEDs in warmweißem Orange, Doppelpunkte blinken                                    │  
  ├───────────────────┼─────────────────────────────────────────────────────────────────────────────────────────┤  
  │ Puls              │ Sinusförmiges Auf- und Abdimmen der gesamten Beleuchtung                                │  
  ├───────────────────┼─────────────────────────────────────────────────────────────────────────────────────────┤  
  │ Slot-Machine      │ Schneller Farbwechsel während der Slot-Animation, sonst Rainbow als Fallback            │
  └───────────────────┴─────────────────────────────────────────────────────────────────────────────────────────┘  
                  
  ---                                                                                                              
  6. Slot-Machine-Animation
                                                                                                                   
  Auf Knopfdruck oder per Web-Interface "rollen" alle 6 Röhren: Jede Röhre dreht ihre Ziffer schnell durch und
  stoppt nacheinander (mit 180 ms Versatz) bei der richtigen Zielziffer. Die Zielwerte sind die aktuelle Uhrzeit.  
                  
  ---                                                                                                              
  7. WiFi und Web-Interface
                                                                                                                   
  Beim Start (setupWifi()):
  - Der ESP32 startet immer als WLAN-Access-Point (NixieClock, Passwort nixie1234) – der Hotspot ist also immer    
  erreichbar unter 192.168.4.1.                                                                                    
  - Wenn zuvor Heimnetz-Zugangsdaten im Flash gespeichert wurden, verbindet sich der ESP32 zusätzlich als          
  WLAN-Client (STA-Modus) mit dem Heimnetz. Bei Erfolg startet automatisch die NTP-Zeitsynchronisierung.           
                                                                                                                   
  NTP-Sync: Sobald die Verbindung steht, ruft der ESP32 die genaue Zeit vom NTP-Server pool.ntp.org ab und schreibt
   sie einmalig in die RTC. Die Zeitzone ist auf Mitteleuropa (CET/CEST) eingestellt.                              
                  
  Web-Server (Port 80) bietet folgende API-Endpunkte:                                                              
                  
  ┌────────────────┬─────────┬────────────────────────────────────────────────────────────┐                        
  │    Endpunkt    │ Methode │                          Funktion                          │
  ├────────────────┼─────────┼────────────────────────────────────────────────────────────┤                        
  │ /              │ GET     │ Liefert die komplette Web-Oberfläche (HTML/CSS/JS)         │
  ├────────────────┼─────────┼────────────────────────────────────────────────────────────┤
  │ /api/status    │ GET     │ Aktuelle Uhrzeit, Helligkeit, Animationsmodus, Slot-Status │                        
  ├────────────────┼─────────┼────────────────────────────────────────────────────────────┤                        
  │ /api/settime   │ POST    │ Uhrzeit stellen ({h, m, s})                                │                        
  ├────────────────┼─────────┼────────────────────────────────────────────────────────────┤                        
  │ /api/bright    │ POST    │ Nixie-Helligkeit (4 Stufen)                                │
  ├────────────────┼─────────┼────────────────────────────────────────────────────────────┤                        
  │ /api/neobright │ POST    │ NeoPixel-Helligkeit (10–255)                               │
  ├────────────────┼─────────┼────────────────────────────────────────────────────────────┤                        
  │ /api/anim      │ POST    │ Animationsmodus wählen                                     │
  ├────────────────┼─────────┼────────────────────────────────────────────────────────────┤                        
  │ /api/neohue    │ POST    │ NeoPixel-Farbe (Hue 0–255)                                 │
  ├────────────────┼─────────┼────────────────────────────────────────────────────────────┤                        
  │ /api/slot      │ POST    │ Slot-Animation auslösen                                    │
  ├────────────────┼─────────┼────────────────────────────────────────────────────────────┤                        
  │ /api/wifi      │ GET     │ WiFi-Status (Modus, IP, NTP-Status)                        │
  ├────────────────┼─────────┼────────────────────────────────────────────────────────────┤                        
  │ /api/wifi      │ POST    │ Heimnetz-Zugangsdaten speichern → automatischer Neustart   │
  ├────────────────┼─────────┼────────────────────────────────────────────────────────────┤                        
  │ /api/wifi/scan │ GET     │ Verfügbare WLAN-Netzwerke scannen                          │
  └────────────────┴─────────┴────────────────────────────────────────────────────────────┘                        
                  
  Die Web-Oberfläche selbst ist als komprimierter HTML/CSS/JS-String im Programmspeicher (PROGMEM) abgelegt, um RAM
   zu sparen.     
                                                                                                                   
  ---             
  8. Einstellungen-Persistenz
                                                                                                                   
  Alle benutzerspezifischen Einstellungen (Helligkeit, NeoPixel-Helligkeit, Animationsmodus, WLAN-Zugangsdaten)
  werden im NVS-Flash des ESP32 gespeichert (Preferences-Bibliothek) und beim Neustart automatisch geladen.        
                  
  ---                                                                                                              
  9. Power-Save   
               
  Nach 120 Sekunden ohne Tastendruck wechselt der ESP32 in einen Stromspar-Modus: Die NeoPixel-Helligkeit wird auf
  ein Viertel gedimmt. Jeder Tastendruck weckt die Uhr wieder auf.                                                 
   
  ---                                                                                                              
  10. Start-Sequenz (setup())
                             
  1. Pins initialisieren (Kathoden, Anoden, Taster, NeoPixel)
  2. Einstellungen aus Flash laden                                                                                 
  3. RTC initialisieren und Zeit auslesen
  4. Multiplex-Timer starten                                                                                       
  5. WiFi starten (setupWifi())
  6. Web-Server starten (setupWebServer())                                                                         
  7. Sanftes Fade-In der NeoPixel-Beleuchtung                                                                      
                                                                                                                   
  ---                                                                                                              
  11. Hauptschleife (loop())                                                                                       
                            
  Pro Durchlauf (~1 ms) werden der Reihe nach abgearbeitet:
                                                                                                                   
  - Fade-In beim Start (blockiert bis abgeschlossen)                                                               
  - Taster auslesen                                                                                                
  - Helligkeitsstufe verwalten                                                                                     
  - Einstellmodus-Logik                                                                                            
  - RTC alle 500 ms lesen und Display aktualisieren
  - Slot-Animation aktualisieren                                                                                   
  - NeoPixel-Animation rendern
  - Power-Save prüfen                                                                                              
  - NTP→RTC-Sync prüfen (einmalig nach Verbindungsaufbau)
  - Ausstehenden Neustart nach WLAN-Änderung ausführen          

  ┌───────────────────┬──────────────────────────────┬────────────────────────────────┐                            
  │    Bibliothek     │            Autor             │ Suchbegriff im Library Manager │                            
  ├───────────────────┼──────────────────────────────┼────────────────────────────────┤                            
  │ Adafruit NeoPixel │ Adafruit                     │ Adafruit NeoPixel              │                            
  ├───────────────────┼──────────────────────────────┼────────────────────────────────┤                            
  │ Rtc by Makuna     │ Michael C. Miller            │ Rtc by Makuna                  │                            
  ├───────────────────┼──────────────────────────────┼────────────────────────────────┤
  │ ArduinoJson       │ Benoit Blanchon              │ ArduinoJson                    │                            
  ├───────────────────┼──────────────────────────────┼────────────────────────────────┤                            
  │ AsyncTCP          │ ESP32Async (ehem. me-no-dev) │ AsyncTCP                       │
  ├───────────────────┼──────────────────────────────┼────────────────────────────────┤                            
  │ ESPAsyncWebServer │ ESP32Async (ehem. me-no-dev) │ ESPAsyncWebServer              │
  └───────────────────┴──────────────────────────────┴────────────────────────────────┘                            
                  
  ▎ Hinweis zu AsyncTCP / ESPAsyncWebServer: Diese beiden werden manchmal nicht im offiziellen Library Manager     
  gefunden oder sind dort veraltet. Zuverlässiger ist die Installation direkt von GitHub:
  ▎ - https://github.com/ESP32Async/AsyncTCP                                                                       
  ▎ - https://github.com/ESP32Async/ESPAsyncWebServer                                                              
  ▎ ZIP herunterladen → Arduino IDE: Sketch → Bibliothek einbinden → .ZIP-Bibliothek hinzufügen
                                                                                                                   
  ---             
  Bereits im ESP32-Arduino-Core enthalten (keine Installation nötig)                                               
                                                                                                                   
  ┌──────────────────────────────────────┬────────────────────────────────────┐
  │              Bibliothek              │            Beschreibung            │                                    
  ├──────────────────────────────────────┼────────────────────────────────────┤
  │ WiFi                                 │ WLAN-Stack des ESP32               │
  ├──────────────────────────────────────┼────────────────────────────────────┤
  │ Preferences                          │ NVS-Flash-Speicher (Einstellungen) │                                    
  ├──────────────────────────────────────┼────────────────────────────────────┤                                    
  │ time.h / configTzTime / getLocalTime │ NTP- und Zeitzonen-Funktionen      │                                    
  └──────────────────────────────────────┴────────────────────────────────────┘                                    
                  
                                                   
                                       
 */
