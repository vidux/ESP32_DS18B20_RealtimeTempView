/*
  ESP32 + DS18B20 realtime WebSocket temperature dashboard

  - Reads one or more DS18B20 sensors on a single OneWire pin
  - Serves a minimal HTML page
  - Pushes temperature updates over WebSocket (/ws) ~1 Hz


  Set your Wi-Fi credentials below.
*/

#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// ===================== User config =====================
const char *WIFI_SSID = "ur ssid";
const char *WIFI_PASSWORD = "ur password";

#define ONE_WIRE_BUS 15        // GPIO for DS18B20 data
#define READ_INTERVAL_MS 1000  // How often to read and broadcast
// =======================================================

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Store discovered sensor addresses
struct DeviceInfo {
  DeviceAddress addr;  // 8 bytes
  float lastC = NAN;
};

std::vector<DeviceInfo> devices;
unsigned long lastRead = 0;

// Convert DeviceAddress to hex string
String addrToHex(const DeviceAddress &addr) {
  char buf[17];  // 16 hex chars + null
  for (uint8_t i = 0; i < 8; i++) {
    sprintf(&buf[i * 2], "%02X", addr[i]);
  }
  buf[16] = '\0';
  return String(buf);
}

// Build JSON string of all sensors
String buildJsonPayload() {
  String json = "{\"type\":\"temps\",\"ts\":" + String(millis()) + ",\"sensors\":[";
  for (size_t i = 0; i < devices.size(); i++) {
    if (i) json += ",";
    json += "{\"addr\":\"" + addrToHex(devices[i].addr) + "\",\"c\":";
    if (isnan(devices[i].lastC)) json += "null";
    else json += String(devices[i].lastC, 2);
    json += "}";
  }
  json += "]}";
  return json;
}

void broadcastTemps() {
  if (ws.count() == 0) return;  // no clients
  String payload = buildJsonPayload();
  ws.textAll(payload);
}

// Minimal responsive HTML page with client-side WebSocket
const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>ESP32 DS18B20 Realtime</title>
  <style>
    :root{font-family:system-ui,-apple-system,Segoe UI,Roboto,Ubuntu,Arial,sans-serif}
    body{margin:0;display:grid;min-height:100svh;place-items:center;background:#0f172a;color:#e2e8f0}
    .card{width:min(680px,92vw);background:#111827;border-radius:16px;padding:20px;box-shadow:0 10px 30px rgb(0 0 0 / .35)}
    h1{font-size:clamp(1.2rem,2.5vw,1.6rem);margin:0 0 8px}
    .meta{opacity:.8;font-size:.9rem;margin-bottom:14px}
    .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:12px}
    .tile{background:#0b1220;border:1px solid #1f2937;border-radius:14px;padding:14px}
    .addr{font-family:ui-monospace,Consolas,monospace;font-size:.85rem;opacity:.9}
    .val{font-size:2.2rem;line-height:1.1}
    .ok{color:#86efac} .warn{color:#fbbf24} .bad{color:#fca5a5}
    footer{margin-top:10px;opacity:.7;font-size:.8rem}
    code{background:#111827;border:1px solid #1f2937;border-radius:8px;padding:2px 6px}
  </style>
</head>
<body>
  <div class="card">
    <h1>ESP32 → DS18B20 Realtime</h1>
    <div class="meta">Status: <span id="status">connecting…</span> · Interval: <span id="ivl">—</span> ms</div>
    <div id="grid" class="grid"></div>
    <footer>WebSocket <code id="wsurl"></code></footer>
  </div>
<script>
(() => {
  const $ = (id)=>document.getElementById(id);
  const grid = $("grid");
  const statusEl = $("status");
  const ivlEl = $("ivl");
  const wsUrl = `ws://${location.host}/ws`;
  $("wsurl").textContent = wsUrl;

  let lastTs = 0;
  const tiles = new Map();

  function ensureTile(addr){
    if(tiles.has(addr)) return tiles.get(addr);
    const d = document.createElement('div');
    d.className = 'tile';
    d.innerHTML = `<div class="addr">${addr}</div><div class="val">—</div>`;
    grid.appendChild(d);
    tiles.set(addr, d);
    return d;
  }

  function clsForTemp(c){
    if(c==null) return '';
    if(c < 0 || c > 60) return 'bad';
    if(c >= 40) return 'warn';
    return 'ok';
  }

  let ws;
  function connect(){
    ws = new WebSocket(wsUrl);
    ws.onopen = () => statusEl.textContent = 'connected';
    ws.onclose = () => { statusEl.textContent = 'disconnected'; setTimeout(connect, 1000); };
    ws.onerror = () => { statusEl.textContent = 'error'; };
    ws.onmessage = (ev) => {
      try {
        const msg = JSON.parse(ev.data);
        if(msg.type === 'temps'){
          if(lastTs) ivlEl.textContent = (msg.ts - lastTs);
          lastTs = msg.ts;
          msg.sensors.forEach(s => {
            const d = ensureTile(s.addr);
            const v = d.querySelector('.val');
            const c = s.c == null ? '—' : s.c.toFixed(2) + ' °C';
            v.textContent = c;
            v.className = 'val ' + clsForTemp(s.c);
          });
        }
      } catch(e) { console.error(e); }
    };
  }

  connect();
})();
</script>
</body>
</html>
)HTML";

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
               void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    // Send immediate snapshot on connect
    client->text(buildJsonPayload());
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nBooting…");

  // Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Connecting to %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.printf("\nWiFi OK  IP: %s\n", WiFi.localIP().toString().c_str());

  // Sensors
  sensors.begin();
  sensors.setWaitForConversion(true);  // synchronous reads
  sensors.setResolution(9);           // 9–12 bits (12 = ~750ms)

  int count = sensors.getDeviceCount();
  Serial.printf("Found %d DS18B20 device(s)\n", count);

  DeviceAddress addr;
  devices.clear();
  for (int i = 0; i < count; i++) {
    if (sensors.getAddress(addr, i)) {
      DeviceInfo di{};
      memcpy(di.addr, addr, 8);
      devices.push_back(di);
      Serial.printf("  %d: %s\n", i, addrToHex(addr).c_str());
    }
  }
  if (devices.empty()) {
    Serial.println("WARNING: No DS18B20 sensors detected.");
  }

  // HTTP + WS
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
    AsyncWebServerResponse *res = req->beginResponse_P(200, "text/html", INDEX_HTML);
    res->addHeader("Cache-Control", "no-store");
    req->send(res);
  });

  server.onNotFound([](AsyncWebServerRequest *req) {
    req->send(404, "text/plain", "Not found");
  });
  server.begin();
  Serial.println("HTTP server started");
}

void runBroadCast() {
  unsigned long now = millis();
  if (now - lastRead >= READ_INTERVAL_MS) {
    lastRead = now;
    sensors.requestTemperatures();

    for (auto &d : devices) {
      float c = sensors.getTempC(d.addr);
      if (c <= -127.0f) c = NAN;  // sensor error
      d.lastC = c;
    }
    broadcastTemps();
  }
}

void loop() {
  // Read & broadcast at interval
  runBroadCast();
}
