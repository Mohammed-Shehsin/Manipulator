#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <ESPmDNS.h>

// ---------------- Wi-Fi ----------------
const char* WIFI_SSID = "Orange_Swiatlowod_B9F0";
const char* WIFI_PASS = "YxGi5hSMNv2nj6W9za";
const char* AP_SSID   = "ESP32-Servo";
const char* AP_PASS   = "12345678";

// ---------------- Servos ----------------
const int NUM_SERVOS = 5;
// D5, D18, D19, D14, D27
const int SERVO_PINS[NUM_SERVOS] = {5, 18, 19, 14, 27};

int usMin = 500, usMax = 2500;
Servo servos[NUM_SERVOS];
volatile int angles[NUM_SERVOS] = {90, 90, 90, 90, 90};

WebServer server(80);
const int LED_PIN = 2;

// ---------------- Helpers ----------------
static inline int clamp(int x, int a, int b) { return x < a ? a : (x > b ? b : x); }
static inline int angleToUs(int deg) { return map(clamp(deg, 0, 180), 0, 180, usMin, usMax); }
static inline void blink() { pinMode(LED_PIN, OUTPUT); digitalWrite(LED_PIN, !digitalRead(LED_PIN)); }

// ---------------- HTML PAGE ----------------
const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 — Five Servo Sliders</title>
<style>
 body{font-family:system-ui,Segoe UI,Roboto,Arial;margin:24px;max-width:720px}
 .card{border:1px solid #ddd;border-radius:8px;padding:16px;margin:12px 0}
 .row{display:flex;align-items:center;gap:12px;flex-wrap:wrap}
 input[type=range]{width:100%}
 .val{min-width:60px;text-align:right}
 button{padding:10px 16px;border:1px solid #0a7c2f;background:#0db24b;color:#fff;border-radius:8px;font-weight:600;cursor:pointer}
 a.btn{display:inline-block;margin-right:8px;margin-top:8px;padding:8px 12px;background:#eee;border-radius:6px;text-decoration:none;color:#000;border:1px solid #ccc}
</style></head><body>
<h2>ESP32 – Five Servo Sliders</h2>

<div class="card">
  <b>Quick test links (no JavaScript):</b><br>
  <a class="btn" href="/set?s1=0&s2=0&s3=0&s4=0&s5=0">All 0°</a>
  <a class="btn" href="/set?s1=90&s2=90&s3=90&s4=90&s5=90">All 90°</a>
  <a class="btn" href="/set?s1=180&s2=180&s3=180&s4=180&s5=180">All 180°</a>
</div>

<form class="card" action="/set" method="get">
  <div class="row">
    <label for="s1"><b>Servo 1</b> (GPIO5)</label>
    <input id="s1" name="s1" type="range" min="0" max="180" value="90" step="1"
           oninput="v1.textContent=this.value">
    <div class="val"><span id="v1">90</span>°</div>
  </div>

  <div class="row" style="margin-top:12px">
    <label for="s2"><b>Servo 2</b> (GPIO18)</label>
    <input id="s2" name="s2" type="range" min="0" max="180" value="90" step="1"
           oninput="v2.textContent=this.value">
    <div class="val"><span id="v2">90</span>°</div>
  </div>

  <div class="row" style="margin-top:12px">
    <label for="s3"><b>Servo 3</b> (GPIO19)</label>
    <input id="s3" name="s3" type="range" min="0" max="180" value="90" step="1"
           oninput="v3.textContent=this.value">
    <div class="val"><span id="v3">90</span>°</div>
  </div>

  <div class="row" style="margin-top:12px">
    <label for="s4"><b>Servo 4</b> (GPIO14)</label>
    <input id="s4" name="s4" type="range" min="0" max="180" value="90" step="1"
           oninput="v4.textContent=this.value">
    <div class="val"><span id="v4">90</span>°</div>
  </div>

  <div class="row" style="margin-top:12px">
    <label for="s5"><b>Servo 5</b> (GPIO27)</label>
    <input id="s5" name="s5" type="range" min="0" max="180" value="90" step="1"
           oninput="v5.textContent=this.value">
    <div class="val"><span id="v5">90</span>°</div>
  </div>

  <div style="margin-top:16px">
    <button type="submit">Send</button>
  </div>
</form>

<div class="card">
  <b>Status:</b> <a class="btn" href="/status">/status</a> (JSON)
</div>
</body></html>
)HTML";

// ---------------- Handlers ----------------
void handleRoot() {
  Serial.println("[HTTP] GET / (root)");
  server.send_P(200, "text/html", PAGE);
}

void handleSet() {
  Serial.printf("[HTTP] GET /set  args=%d\n", server.args());
  for (uint8_t i = 0; i < server.args(); ++i) {
    Serial.printf("  %s=%s\n",
                  server.argName(i).c_str(),
                  server.arg(i).c_str());
  }

  // Update only servos that appear as query parameters
  for (int i = 0; i < NUM_SERVOS; ++i) {
    String param = "s" + String(i + 1);  // s1..s5
    if (server.hasArg(param)) {
      int a = clamp(server.arg(param).toInt(), 0, 180);
      angles[i] = a;
      servos[i].write(angles[i]);
    }
  }

  // Debug print
  Serial.print("[SET]");
  for (int i = 0; i < NUM_SERVOS; ++i) {
    Serial.printf(" S%d=%3d° (%4dus)",
                  i + 1, angles[i], angleToUs(angles[i]));
  }
  Serial.println();

  blink();

  // Simple response page
  String resp = "OK<br>";
  for (int i = 0; i < NUM_SERVOS; ++i) {
    resp += "S" + String(i + 1) + "=" + String(angles[i]) +
            " (us~" + String(angleToUs(angles[i])) + ")<br>";
  }
  resp += "Back: <a href='/'>home</a>";
  server.send(200, "text/html", resp);
}

void handleStatus() {
  // JSON: {"s1":90,"s2":90,"s3":90,"s4":90,"s5":90}
  String json = "{";
  for (int i = 0; i < NUM_SERVOS; ++i) {
    json += "\"s" + String(i + 1) + "\":" + String(angles[i]);
    if (i < NUM_SERVOS - 1) json += ",";
  }
  json += "}";
  server.send(200, "application/json", json);
}

void handleNotFound() {
  Serial.printf("[HTTP] 404 %s\n", server.uri().c_str());
  server.send(404, "text/plain", "Not found");
}

void startHTTP() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/set", HTTP_GET, handleSet);
  server.on("/status", HTTP_GET, handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

// ---------------- Setup / Loop ----------------
void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  delay(500);
  Serial.println("\nBooting…");

  // Allocate PWM timers (enough for many servos)
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Attach all servos
  for (int i = 0; i < NUM_SERVOS; ++i) {
    servos[i].setPeriodHertz(50); // standard 50 Hz servo
    servos[i].attach(SERVO_PINS[i], usMin, usMax);
    servos[i].write(angles[i]);   // go to initial 90°
  }

  // Wi-Fi STA first
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
    Serial.print("Wi-Fi IP: ");
    Serial.println(WiFi.localIP());
    if (MDNS.begin("esp32")) {
      Serial.println("mDNS: http://esp32.local");
    }
    startHTTP();
  } else {
    Serial.println("STA failed → AP mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    startHTTP();
  }
}

void loop() {
  server.handleClient();
}
