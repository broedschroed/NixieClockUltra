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
<div style="font-size:1.2em;color:var(--dim);letter-spacing:.1em;margin-bottom:20px" id="dateDisp">--.--.--</div>

<div class="card">
  <h2>⏱ Zeit & Datum stellen</h2>
  <div class="row"><label>Stunde</label><input type="number" id="ih" min="0" max="23" value="0"></div>
  <div class="row"><label>Minute</label><input type="number" id="im" min="0" max="59" value="0"></div>
  <div class="row"><label>Sekunde</label><input type="number" id="is" min="0" max="59" value="0"></div>
  <div class="row"><label>Tag</label><input type="number" id="id" min="1" max="31" value="1"></div>
  <div class="row"><label>Monat</label><input type="number" id="imo" min="1" max="12" value="1"></div>
  <div class="row"><label>Jahr (2-stellig)</label><input type="number" id="iy" min="0" max="99" value="24"></div>
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
      <option value="0">Sehr dunkel</option>
      <option value="1">Dunkel</option>
      <option value="2">Hell</option>
      <option value="3" selected>Sehr hell</option>
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
    </select>
  </div>
  <div class="row">
    <label>Trennpunkt-Helligkeit</label>
    <input type="range" id="colonBright" min="1" max="100" value="80" oninput="setColonBright(this.value)">
    <span id="colonBrightVal">80</span>
  </div>
  <div class="row">
    <label>Trennpunkte dauerhaft an</label>
    <label class="toggle"><input type="checkbox" id="colonOn" onchange="setColonOn(this.checked)"><span class="slider"></span></label>
  </div>
  <div class="row">
    <label>Trennpunkte statisch (Warmweiß)</label>
    <label class="toggle"><input type="checkbox" id="colonStatic" onchange="setColonStatic(this.checked)"><span class="slider"></span></label>
  </div>

</div>

<div class="card">
  <h2>🎰 Slot-Animation</h2>
  <div class="row">
    <label>Automatisch</label>
    <select id="slotIval" onchange="setSlotInterval()">
      <option value="0">Aus</option>
      <option value="1">10 Sek</option>
      <option value="2">1 Min</option>
      <option value="3">15 Min</option>
      <option value="4">1 Std</option>
    </select>
  </div>
  <div class="row">
    <button onclick="triggerSlot()">Slot-Machine!</button>
    <span class="badge" id="slotStatus">bereit</span>
  </div>
</div>

<div class="card">
  <h2>🌙 Nacht-Modus</h2>
  <div class="row"><label>Aktuell</label><span class="badge" id="nightStateBadge">Normal</span></div>
  <div class="row"><label>Zeitbereich aktiv</label>
    <label class="toggle"><input type="checkbox" id="ntEn"><span class="slider"></span></label></div>
  <div class="row"><label>Von (Stunde 0–23)</label>
    <input type="number" id="ntFrom" min="0" max="23" value="23"></div>
  <div class="row"><label>Bis (Stunde 0–23)</label>
    <input type="number" id="ntTo" min="0" max="23" value="7"></div>
  <div class="row"><label>Modus</label>
    <select id="ntMode">
      <option value="0">Gedimmt</option>
      <option value="1">Dunkel</option>
    </select></div>
  <div class="row"><label>LDR-Sensor aktiv</label>
    <label class="toggle"><input type="checkbox" id="ldrEn"><span class="slider"></span></label></div>
  <div class="row"><label>Schwellwert (0–4095)</label>
    <input type="range" id="ldrThr" min="0" max="4095" value="512"
      oninput="document.getElementById('ldrThrVal').textContent=this.value">
    <span id="ldrThrVal">512</span></div>
  <div class="row"><label>LDR-Rohwert</label><span class="badge" id="ldrVal">--</span></div>
  <div class="row"><button onclick="saveNightMode()">Übernehmen</button></div>
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
  </div>
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
  if(d.slotIval!==undefined) document.getElementById('slotIval').value=d.slotIval;
  if(d.colonOn!==undefined)document.getElementById('colonOn').checked=d.colonOn;
  if(d.colonStatic!==undefined)document.getElementById('colonStatic').checked=d.colonStatic;
  if(d.colonBright!==undefined){document.getElementById('colonBright').value=d.colonBright;document.getElementById('colonBrightVal').textContent=d.colonBright;}
  if(d.slot!==undefined) document.getElementById('slotStatus').textContent=d.slot?'läuft…':'bereit';
  if(d.date){
    let parts=d.date.split('.');
    if(parts.length===3){
      document.getElementById('id').value  = parseInt(parts[0]);
      document.getElementById('imo').value = parseInt(parts[1]);
      document.getElementById('iy').value  = parseInt(parts[2]);
      document.getElementById('dateDisp').textContent = d.date;
    }
  }
  if(d.nightState!==undefined) document.getElementById('nightStateBadge').textContent=NS_LABELS[d.nightState]||'Normal';
}

async function setTime(){
  let h  = parseInt(document.getElementById('ih').value);
  let m  = parseInt(document.getElementById('im').value);
  let s  = parseInt(document.getElementById('is').value);
  let d  = parseInt(document.getElementById('id').value);
  let mo = parseInt(document.getElementById('imo').value);
  let y  = parseInt(document.getElementById('iy').value);
  let r  = await api('/api/settime',{h,m,s,d,mo,y});
  document.getElementById('statusMsg').textContent=r.ok?'Zeit & Datum gesetzt ✓':'Fehler ✗';
}
function syncBrowser(){
  let now=new Date();
  document.getElementById('ih').value  = now.getHours();
  document.getElementById('im').value  = now.getMinutes();
  document.getElementById('is').value  = now.getSeconds();
  document.getElementById('id').value  = now.getDate();
  document.getElementById('imo').value = now.getMonth()+1;
  document.getElementById('iy').value  = now.getFullYear()%100;
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
async function setSlotInterval(){
  let v=parseInt(document.getElementById('slotIval').value);
  await api('/api/slotinterval',{interval:v});
}
async function setColonBright(v){
  document.getElementById('colonBrightVal').textContent=v;
  await api('/api/colonbright',{val:parseInt(v)});
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

const IR_ACTIONS=['SET','UP','DOWN','BRIGHTNESS','ANIM_NEXT','SLOT','COLON_TOGGLE'];
const IR_LABELS={'SET':'SET &#x2013; Einstellmodus','UP':'UP &#x2013; Erh&ouml;hen','DOWN':'DOWN &#x2013; Verringern',
  'BRIGHTNESS':'BRIGHTNESS &#x2013; Helligkeit','ANIM_NEXT':'ANIM &#x2013; n&auml;chste Animation',
  'SLOT':'SLOT &#x2013; Slot-Maschine','COLON_TOGGLE':'COLON &#x2013; Trennpunkte an/aus'};
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
async function setColonOn(enabled){
  await api('/api/colonon',{enabled});
}
async function setColonStatic(enabled){
  await api('/api/colonstatic',{enabled});
}

const NS_LABELS=['Normal','Gedimmt','Dunkel'];
async function refreshNightMode(){
  let d=await get('/api/nightmode');
  if(d.ntEn===undefined)return;
  document.getElementById('ntEn').checked  = d.ntEn;
  document.getElementById('ntFrom').value  = d.ntFrom;
  document.getElementById('ntTo').value    = d.ntTo;
  document.getElementById('ntMode').value  = d.ntMode;
  document.getElementById('ldrEn').checked = d.ldrEn;
  document.getElementById('ldrThr').value  = d.ldrThr;
  document.getElementById('ldrThrVal').textContent = d.ldrThr;
  document.getElementById('ldrVal').textContent    = d.ldrVal!==undefined?d.ldrVal:'--';
}
async function saveNightMode(){
  let r=await api('/api/nightmode',{
    ntEn:  document.getElementById('ntEn').checked,
    ntFrom:parseInt(document.getElementById('ntFrom').value),
    ntTo:  parseInt(document.getElementById('ntTo').value),
    ntMode:parseInt(document.getElementById('ntMode').value),
    ldrEn: document.getElementById('ldrEn').checked,
    ldrThr:parseInt(document.getElementById('ldrThr').value)
  });
  document.getElementById('statusMsg').textContent=r.ok?'Nacht-Modus gespeichert ✓':'Fehler ✗';
}

setInterval(refreshClock,1000);
refreshClock();
refreshWifi();
refreshIR();
refreshNightMode();
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

  // DHCP-Hostname setzen bevor WiFi-Stack konfiguriert wird
  WiFi.setHostname(WIFI_HOSTNAME);

  // WIFI_AP_STA immer setzen: STA-Interface muss aktiv sein damit Scan funktioniert
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WiFi] AP gestartet: %s  IP: %s\n", WIFI_SSID, WiFi.softAPIP().toString().c_str());

  if (savedSsid.length() > 0) {
    WiFi.begin(savedSsid.c_str(), savedPass.c_str());
    Serial.printf("[WiFi] Verbinde mit '%s'...\n", savedSsid.c_str());
    unsigned long t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < 20000) {
      delay(200);
    }
    if (WiFi.status() == WL_CONNECTED) {
      wifiStaConnected = true;
      Serial.printf("[WiFi] STA verbunden. IP: %s\n", WiFi.localIP().toString().c_str());
      if (MDNS.begin(WIFI_HOSTNAME)) {
        Serial.printf("[mDNS] Erreichbar als http://%s.local\n", WIFI_HOSTNAME);
      }
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
    StaticJsonDocument<384> doc;
    char buf[10];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", curHour, curMin, curSec);
    doc["time"]       = buf;
    doc["bright"]     = brightLevel;
    doc["neoBright"]  = neoBright;
    doc["anim"]       = (int)animMode;
    doc["slot"]       = slotActive;
    doc["slotIval"]   = (int)slotInterval;
    doc["colonOn"]     = colonAlwaysOn;
    doc["colonStatic"] = colonStatic;
    doc["colonBright"] = colonBright;
    char dateBuf[9];
    snprintf(dateBuf, sizeof(dateBuf), "%02d.%02d.%02d", curDay, curMonth, curYear);
    doc["date"]       = dateBuf;
    doc["nightState"] = (int)nightState;
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
        if (doc.containsKey("d"))  curDay   = constrain((int)doc["d"],  1, 31);
        if (doc.containsKey("mo")) curMonth = constrain((int)doc["mo"], 1, 12);
        if (doc.containsKey("y"))  curYear  = constrain((int)doc["y"],  0, 99);
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

  server.on("/api/colonbright", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      StaticJsonDocument<64> doc;
      if (!deserializeJson(doc, data, len)) {
        colonBright = constrain((int)doc["val"], 1, 100);
        prefs.putUChar("colonBright", colonBright);
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

  server.on("/api/slotinterval", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      StaticJsonDocument<64> doc;
      if (!deserializeJson(doc, data, len)) {
        int v = constrain((int)doc["interval"], 0, 4);
        slotInterval = (SlotInterval)v;
        prefs.putUChar("slotIval", v);
        req->send(200, "application/json", "{\"ok\":true}");
      } else {
        req->send(400, "application/json", "{\"ok\":false}");
      }
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

  // --- Trennpunkte dauerhaft an ---
  server.on("/api/colonon", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      StaticJsonDocument<64> doc;
      if (!deserializeJson(doc, data, len)) {
        colonAlwaysOn = (bool)doc["enabled"];
        prefs.putBool("colonOn", colonAlwaysOn);
        req->send(200, "application/json", "{\"ok\":true}");
      } else {
        req->send(400, "application/json", "{\"ok\":false}");
      }
    }
  );

  server.on("/api/colonstatic", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      StaticJsonDocument<64> doc;
      if (!deserializeJson(doc, data, len)) {
        colonStatic = (bool)doc["enabled"];
        prefs.putBool("colonStatic", colonStatic);
        req->send(200, "application/json", "{\"ok\":true}");
      } else {
        req->send(400, "application/json", "{\"ok\":false}");
      }
    }
  );

  // --- Nacht-Modus: Status lesen ---
  server.on("/api/nightmode", HTTP_GET, [](AsyncWebServerRequest *req) {
    StaticJsonDocument<256> doc;
    doc["ntEn"]   = nightTimeEnabled;
    doc["ntFrom"] = nightStart;
    doc["ntTo"]   = nightEnd;
    doc["ntMode"] = nightTimeMode;
    doc["ldrEn"]  = ldrEnabled;
    doc["ldrThr"] = ldrThreshold;
    doc["ldrVal"] = ldrReading;
    doc["state"]  = (int)nightState;
    String out; serializeJson(doc, out);
    req->send(200, "application/json", out);
  });

  // --- Nacht-Modus: Einstellungen speichern ---
  server.on("/api/nightmode", HTTP_POST, [](AsyncWebServerRequest *req){},
    nullptr,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
      StaticJsonDocument<256> doc;
      if (!deserializeJson(doc, data, len)) {
        nightTimeEnabled = doc["ntEn"].as<bool>();
        nightStart       = constrain((int)doc["ntFrom"], 0, 23);
        nightEnd         = constrain((int)doc["ntTo"],   0, 23);
        nightTimeMode    = constrain((int)doc["ntMode"], 0, 1);
        ldrEnabled       = doc["ldrEn"].as<bool>();
        ldrThreshold     = (uint16_t)constrain((int)doc["ldrThr"], 0, 4095);
        prefs.putBool("ntEn",     nightTimeEnabled);
        prefs.putUChar("ntFrom",  nightStart);
        prefs.putUChar("ntTo",    nightEnd);
        prefs.putUChar("ntMode",  nightTimeMode);
        prefs.putBool("ldrEn",    ldrEnabled);
        prefs.putUShort("ldrThr", ldrThreshold);
        req->send(200, "application/json", "{\"ok\":true}");
      } else {
        req->send(400, "application/json", "{\"ok\":false}");
      }
    }
  );

  server.begin();
}
