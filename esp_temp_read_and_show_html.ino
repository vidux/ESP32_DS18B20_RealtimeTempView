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

#define ONE_WIRE_BUS 15       // GPIO for DS18B20 data
#define READ_INTERVAL_MS 1000 // How often to read and broadcast
// =======================================================

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Store discovered sensor addresses
struct DeviceInfo
{
  DeviceAddress addr; // 8 bytes
  float lastC = NAN;
};

std::vector<DeviceInfo> devices;
unsigned long lastRead = 0;

// Convert DeviceAddress to hex string
String addrToHex(const DeviceAddress &addr)
{
  char buf[17]; // 16 hex chars + null
  for (uint8_t i = 0; i < 8; i++)
  {
    sprintf(&buf[i * 2], "%02X", addr[i]);
  }
  buf[16] = '\0';
  return String(buf);
}

// Build JSON string of all sensors
String buildJsonPayload()
{
  String json = "{\"type\":\"temps\",\"ts\":" + String(millis()) + ",\"sensors\":[";
  for (size_t i = 0; i < devices.size(); i++)
  {
    if (i)
      json += ",";
    json += "{\"addr\":\"" + addrToHex(devices[i].addr) + "\",\"c\":";
    if (isnan(devices[i].lastC))
      json += "null";
    else
      json += String(devices[i].lastC, 2);
    json += "}";
  }
  json += "]}";
  return json;
}

void broadcastTemps()
{
  if (ws.count() == 0)
    return; // no clients
  String payload = buildJsonPayload();
  ws.textAll(payload);
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
               void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    // Send immediate snapshot on connect
    client->text(buildJsonPayload());
  }
}

void setup()
{
  Serial.begin(115200);
  delay(300);
  Serial.println("\nBooting…");

  // Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Connecting to %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(400);
    Serial.print(".");
  }
  Serial.printf("\nWiFi OK  IP: %s\n", WiFi.localIP().toString().c_str());

  // Sensors
  sensors.begin();
  sensors.setWaitForConversion(true); // synchronous reads
  sensors.setResolution(9);           // 9–12 bits (12 = ~750ms)

  int count = sensors.getDeviceCount();
  Serial.printf("Found %d DS18B20 device(s)\n", count);

  DeviceAddress addr;
  devices.clear();
  for (int i = 0; i < count; i++)
  {
    if (sensors.getAddress(addr, i))
    {
      DeviceInfo di{};
      memcpy(di.addr, addr, 8);
      devices.push_back(di);
      Serial.printf("  %d: %s\n", i, addrToHex(addr).c_str());
    }
  }
  if (devices.empty())
  {
    Serial.println("WARNING: No DS18B20 sensors detected.");
  }

  // HTTP + WS

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();
  Serial.println("HTTP server started");
}

void runBroadCast()
{
  unsigned long now = millis();
  if (now - lastRead >= READ_INTERVAL_MS)
  {
    lastRead = now;
    sensors.requestTemperatures();

    for (auto &d : devices)
    {
      float c = sensors.getTempC(d.addr);
      if (c <= -127.0f)
        c = NAN; // sensor error
      d.lastC = c;
    }
    broadcastTemps();
  }
}

void loop()
{
  // Read & broadcast at interval
  runBroadCast();
}
