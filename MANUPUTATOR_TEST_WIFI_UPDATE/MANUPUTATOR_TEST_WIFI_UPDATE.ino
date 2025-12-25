#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <ESPmDNS.h>

// ---------------- Wi-Fi ----------------
const char* WIFI_SSID = "WIFI_ADDR";
const char* WIFI_PASS = "WIFI_PASS";
const char* AP_SSID   = "ESP32-Servo";
const char* AP_PASS   = "12345678";

WebServer server(80);
const int LED_PIN = 2;

// ---------------- Joint mapping ----------------
enum JointId { BASE=0, SHOULDER=1, ELBOW=2, WRIST=3, GRIPPER=4, N=5 };

struct Joint {
  const char* key;
  int pin;
  int minDeg;
  int maxDeg;
  int homeDeg;

  int cur;      // applied angle
  int target;   // requested angle
  Servo servo;
};

// Your mapping (A):
Joint J[N] = {
  {"base",     27, 0, 180,  30,  30,  30},
  {"shoulder",  5, 0, 180, 170, 170, 170},
  {"elbow",    19, 0, 180,   0,   0,   0},
  {"wrist",    18, 0, 180,  66,  66,  66},
  {"gripper",  14, 0,  90,  90,  90,  90}
};

// Servo pulse limits
int usMin = 500, usMax = 2500;

// ---------------- Speed control (smooth motion) ----------------
volatile int spd = 60;         // 1..100
volatile int stepDeg = 2;      // deg per update
volatile int intervalMs = 25;  // ms per update
uint32_t lastMoveMs = 0;

static inline int clampi(int x, int a, int b) { return x < a ? a : (x > b ? b : x); }
static inline int signi(int x) { return (x > 0) - (x < 0); }

void updateSpeedDerived() {
  stepDeg    = map(clampi(spd, 1, 100), 1, 100, 1, 6);       // 1..6 deg
  intervalMs = map(clampi(spd, 1, 100), 1, 100, 140, 10);    // 140..10 ms
}

static inline void blink() { digitalWrite(LED_PIN, !digitalRead(LED_PIN)); }

void attachAndHome() {
  for (int i = 0; i < N; ++i) {
    J[i].servo.setPeriodHertz(50);
    J[i].servo.attach(J[i].pin, usMin, usMax);

    J[i].cur = clampi(J[i].homeDeg, J[i].minDeg, J[i].maxDeg);
    J[i].target = J[i].cur;
    J[i].servo.write(J[i].cur);
  }
}

void updateServosSmooth() {
  uint32_t now = millis();
  if (now - lastMoveMs < (uint32_t)intervalMs) return;
  lastMoveMs = now;

  for (int i = 0; i < N; ++i) {
    int c = J[i].cur;
    int t = J[i].target;
    if (c == t) continue;

    int d = t - c;
    int step = signi(d) * stepDeg;
    int next = c + step;

    if ((step > 0 && next > t) || (step < 0 && next < t)) next = t;

    J[i].cur = next;
    J[i].servo.write(J[i].cur);
  }
}

// ---------------- HTML ----------------
const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 — Robot Arm Control (2D)</title>
<style>
  body{font-family:system-ui,Segoe UI,Roboto,Arial;margin:16px;max-width:860px}
  .grid{display:grid;grid-template-columns:1fr;gap:12px}
  .card{border:1px solid #ddd;border-radius:12px;padding:14px}
  .row{display:flex;align-items:center;gap:10px;margin:10px 0}
  label{min-width:140px;font-weight:700}
  input[type=range]{flex:1}
  .val{min-width:72px;text-align:right;font-variant-numeric:tabular-nums}
  button{padding:10px 14px;border-radius:10px;border:1px solid #0a7c2f;background:#0db24b;color:#fff;font-weight:700}
  .btn2{background:#f3f3f3;border:1px solid #ccc;color:#000}
  .small{color:#666;font-size:13px}
  .hdr{display:flex;justify-content:space-between;gap:10px;flex-wrap:wrap;align-items:center}
  .pill{padding:6px 10px;border:1px solid #ddd;border-radius:999px;background:#fff}

  /* ✅ Canvas centered and truly sized for mobile */
  .canvasWrap{display:flex;justify-content:center}
  canvas{
    display:block;
    width:360px;     /* fixed displayed size */
    height:260px;
    max-width:100%;
    border:1px solid #eee;
    border-radius:12px;
    background:#fafafa
  }

  /* Readout outside canvas (no “corner” feeling) */
  .readout{
    margin-top:10px;
    padding:10px 12px;
    border:1px solid #eee;
    border-radius:10px;
    background:#fff;
    font-size:14px;
    line-height:1.4;
  }
  .readout b{display:inline-block;min-width:120px}
</style></head><body>

<div class="hdr">
  <h2 style="margin:0">ESP32 – Robot Arm Control (2D Preview)</h2>
  <div class="pill small" id="net">Connecting…</div>
</div>

<div class="grid">
  <div class="card">
    <div class="canvasWrap">
      <!-- ✅ internal resolution matches displayed resolution -->
      <canvas id="cv" width="360" height="260"></canvas>
    </div>

    <div class="readout" id="readout">—</div>

    <div class="small" style="margin-top:8px">
      Base (Yaw) + Wrist (Roll) are rotation joints. Shoulder drawing direction matches your real robot.
    </div>
  </div>

  <div class="card">
    <div class="row">
      <label>Speed</label>
      <input id="spd" type="range" min="1" max="100" value="60">
      <div class="val"><span id="v_spd">60</span></div>
    </div>

    <div class="row">
      <label>Base (Yaw)</label>
      <input id="base" type="range" min="0" max="180" value="30">
      <div class="val"><span id="v_base">30</span>°</div>
    </div>

    <div class="row">
      <label>Shoulder (Pitch)</label>
      <input id="shoulder" type="range" min="0" max="180" value="170">
      <div class="val"><span id="v_shoulder">170</span>°</div>
    </div>

    <div class="row">
      <label>Elbow (Pitch)</label>
      <input id="elbow" type="range" min="0" max="180" value="0">
      <div class="val"><span id="v_elbow">0</span>°</div>
    </div>

    <div class="row">
      <label>Wrist (Roll)</label>
      <input id="wrist" type="range" min="0" max="180" value="66">
      <div class="val"><span id="v_wrist">66</span>°</div>
    </div>

    <div class="row">
      <label>Gripper (Open)</label>
      <input id="gripper" type="range" min="0" max="90" value="90">
      <div class="val"><span id="v_gripper">90</span>°</div>
    </div>

    <div style="display:flex;gap:10px;flex-wrap:wrap;margin-top:10px">
      <button id="homeBtn" type="button">Home</button>
      <button id="syncBtn" type="button" class="btn2">Sync from ESP32</button>
      <a class="btn2" style="padding:10px 14px;border-radius:10px;text-decoration:none;display:inline-block"
         href="/status">/status</a>
    </div>

    <div class="small" style="margin-top:10px">
      Auto-send enabled (throttled). Speed controls smooth motion on ESP32.
    </div>
  </div>
</div>

<script>
  // ✅ Shorter links (shoulder + elbow shorter)
  const L1 = 70;   // shoulder link (shorter)
  const L2 = 65;   // elbow link (shorter)
  const L3 = 40;   // wrist/tool link

  const HOME = { spd:60, base:30, shoulder:170, elbow:0, wrist:66, gripper:90 };

  const ids = ["spd","base","shoulder","elbow","wrist","gripper"];
  const el = Object.fromEntries(ids.map(id => [id, document.getElementById(id)]));
  const v  = Object.fromEntries(ids.map(id => [id, document.getElementById("v_"+id)]));

  function getUI(){
    const a = {};
    for(const id of ids) a[id] = parseInt(el[id].value,10);
    return a;
  }
  function setUI(a){
    for(const id of ids){
      if(a[id] !== undefined){
        el[id].value = a[id];
        v[id].textContent = a[id];
      }
    }
    draw();
  }

  let pending=false, lastSent=0;
  const THROTTLE_MS = 120;

  async function sendNow(){
    const a = getUI();
    const qs = new URLSearchParams(a).toString();
    try{
      const resp = await fetch("/set?"+qs, {cache:"no-store"});
      if(!resp.ok) throw new Error("HTTP "+resp.status);
      document.getElementById("net").textContent = "OK (sent)";
      document.getElementById("net").style.color = "#0a7c2f";
    }catch(e){
      document.getElementById("net").textContent = "Send failed";
      document.getElementById("net").style.color = "#b00020";
    }
  }

  function scheduleSend(){
    const now = Date.now();
    if(now - lastSent > THROTTLE_MS){
      lastSent = now;
      sendNow();
      return;
    }
    if(pending) return;
    pending = true;
    setTimeout(() => {
      pending=false;
      lastSent = Date.now();
      sendNow();
    }, THROTTLE_MS);
  }

  for(const id of ids){
    el[id].addEventListener("input", () => {
      v[id].textContent = el[id].value;
      draw();
      scheduleSend();
    });
  }

  document.getElementById("homeBtn").addEventListener("click", () => {
    setUI(HOME);
    scheduleSend();
  });

  document.getElementById("syncBtn").addEventListener("click", async () => {
    await syncFromESP();
  });

  async function syncFromESP(){
    try{
      const r = await fetch("/status", {cache:"no-store"});
      if(!r.ok) throw new Error("HTTP "+r.status);
      const data = await r.json();
      setUI(data);
      document.getElementById("net").textContent = "Synced";
      document.getElementById("net").style.color = "#0a7c2f";
    }catch(e){
      document.getElementById("net").textContent = "Sync failed";
      document.getElementById("net").style.color = "#b00020";
    }
  }

  const cv = document.getElementById("cv");
  const ctx = cv.getContext("2d");
  const toRad = d => (d*Math.PI)/180;

  function drawBaseYawIndicator(x,y,deg){
    const r=16;
    ctx.lineWidth=3; ctx.strokeStyle="#666";
    ctx.beginPath(); ctx.arc(x,y,r,0,Math.PI*2); ctx.stroke();
    const a = toRad(deg - 90);
    const ax=x + r*Math.cos(a), ay=y + r*Math.sin(a);
    ctx.strokeStyle="#111"; ctx.lineWidth=4;
    ctx.beginPath(); ctx.moveTo(x,y); ctx.lineTo(ax,ay); ctx.stroke();
  }

  function drawWristRollMarker(x,y,rollRad){
    const s=9;
    ctx.save();
    ctx.translate(x,y);
    ctx.rotate(rollRad);
    ctx.strokeStyle="#000"; ctx.lineWidth=3;
    ctx.beginPath(); ctx.moveTo(-s,-s); ctx.lineTo(s,s); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(-s,s); ctx.lineTo(s,-s); ctx.stroke();
    ctx.restore();
  }

  function updateReadout(a){
    document.getElementById("readout").innerHTML =
      `<b>Speed:</b> ${a.spd}<br>` +
      `<b>Base (Yaw):</b> ${a.base}°<br>` +
      `<b>Shoulder:</b> ${a.shoulder}°<br>` +
      `<b>Elbow:</b> ${a.elbow}°<br>` +
      `<b>Wrist (Roll):</b> ${a.wrist}°<br>` +
      `<b>Gripper:</b> ${a.gripper}°`;
  }

  function draw(){
    const a = getUI();
    updateReadout(a);

    ctx.clearRect(0,0,cv.width,cv.height);

    // ✅ TRUE center in canvas
    const ox = cv.width / 2;
    const oy = cv.height - 28;

    // Calibration constants (keep your working ones)
    const SHOULDER_OFFSET = 260;
    const ELBOW_OFFSET = 0;
    const WRIST_ROLL_OFFSET = 66;

    // Shoulder direction corrected (bend right)
    const th1 = toRad((SHOULDER_OFFSET - a.shoulder) - 180);
    const th2 = th1 + toRad(a.elbow - ELBOW_OFFSET);
    const roll = toRad(a.wrist - WRIST_ROLL_OFFSET);

    const x1 = ox + L1*Math.cos(th1), y1 = oy + L1*Math.sin(th1);
    const x2 = x1 + L2*Math.cos(th2), y2 = y1 + L2*Math.sin(th2);
    const x3 = x2 + L3*Math.cos(th2), y3 = y2 + L3*Math.sin(th2);

    // base stand
    ctx.lineWidth=8; ctx.strokeStyle="#333";
    ctx.beginPath(); ctx.moveTo(ox-30,oy+22); ctx.lineTo(ox+30,oy+22); ctx.stroke();

    drawBaseYawIndicator(ox, oy+4, a.base);

    // links
    ctx.lineWidth=10; ctx.strokeStyle="#222"; ctx.lineCap="round";
    ctx.beginPath(); ctx.moveTo(ox,oy); ctx.lineTo(x1,y1); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(x1,y1); ctx.lineTo(x2,y2); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(x2,y2); ctx.lineTo(x3,y3); ctx.stroke();

    // joints
    function joint(x,y){
      ctx.fillStyle="#fff"; ctx.strokeStyle="#111"; ctx.lineWidth=3;
      ctx.beginPath(); ctx.arc(x,y,8,0,Math.PI*2); ctx.fill(); ctx.stroke();
    }
    joint(ox,oy); joint(x1,y1); joint(x2,y2); joint(x3,y3);

    drawWristRollMarker(x3,y3,roll);

    // gripper
    const grip = a.gripper;
    const gOpen = (grip/90)*0.8;
    const gLen = 18;
    const toolDir = th2 + roll;

    const gAng1 = toolDir + gOpen;
    const gAng2 = toolDir - gOpen;

    ctx.lineWidth=5; ctx.strokeStyle="#444";
    ctx.beginPath(); ctx.moveTo(x3,y3); ctx.lineTo(x3+gLen*Math.cos(gAng1), y3+gLen*Math.sin(gAng1)); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(x3,y3); ctx.lineTo(x3+gLen*Math.cos(gAng2), y3+gLen*Math.sin(gAng2)); ctx.stroke();
  }

  (async function init(){
    draw();
    await syncFromESP();
  })();
</script>

</body></html>
)HTML";

// ---------------- HTTP handlers ----------------
void handleRoot() { server.send_P(200, "text/html", PAGE); }

// /set?spd=..&base=..&shoulder=..&elbow=..&wrist=..&gripper=..
void handleSet() {
  if (server.hasArg("spd")) {
    spd = clampi(server.arg("spd").toInt(), 1, 100);
    updateSpeedDerived();
  }

  for (int i = 0; i < N; ++i) {
    if (server.hasArg(J[i].key)) {
      int raw = server.arg(J[i].key).toInt();
      raw = clampi(raw, J[i].minDeg, J[i].maxDeg);
      J[i].target = raw;
    }
  }

  blink();
  server.send(200, "text/plain", "OK");
}

// /status -> current applied angles + speed
void handleStatus() {
  String json = "{";
  json += "\"spd\":" + String(spd) + ",";
  for (int i = 0; i < N; ++i) {
    json += "\"";
    json += J[i].key;
    json += "\":";
    json += J[i].cur;
    if (i < N - 1) json += ",";
  }
  json += "}";
  server.send(200, "application/json", json);
}

void handleNotFound() { server.send(404, "text/plain", "Not found"); }

void startHTTP() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/set", HTTP_GET, handleSet);
  server.on("/status", HTTP_GET, handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();
}

// ---------------- Setup / Loop ----------------
void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  delay(300);
  Serial.println("\nBooting…");

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  updateSpeedDerived();
  attachAndHome();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to '%s'…\n", WIFI_SSID);

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(400);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi IP: "); Serial.println(WiFi.localIP());
    if (MDNS.begin("esp32")) Serial.println("mDNS: http://esp32.local");
  } else {
    Serial.println("STA failed → AP mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
  }

  startHTTP();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  updateServosSmooth();
}
