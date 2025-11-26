#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <BH1750.h>

BH1750 lightMeter;

// ======================= CONFIG =======================
const char* ssid     = "VIETTEL TANG 3";
const char* password = "0966536583";

// Relay ESP32 pins
const int relayPins[] = {16, 17, 5, 18, 19, 23};
const int soLuongDen = 6;

// trạng thái đèn (0 = tắt, 1 = bật)
int relayState[6] = {0,0,0,0,0,0};

// 0 = manual, 1 = auto
int autoMode = 0;

// WebServer port 80
WebServer server(80);

// helper: gửi header CORS
void sendCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}


void setRelay(int id, int state) {
  if (id < 0 || id >= soLuongDen) return;
  relayState[id] = state;
  // Nếu module active-LOW: LOW = Bật, HIGH = Tắt
  digitalWrite(relayPins[id], state ? LOW : HIGH);
}

void setAll(int state) {
  for (int i = 0; i< soLuongDen; i++) {
    setRelay(i, state);
  }
}

// -----------------------------------------------------
void handleOptions() {
  // Trả về OK cho preflight
  sendCORSHeaders();
  server.send(204, "text/plain", "");
}

void handleStatus() {
  float lux = lightMeter.readLightLevel();
  // Build JSON manually (nhỏ gọn)
  String json = "{";
  json += "\"lux\":" + String(lux, 2) + ",";
  json += "\"auto\":" + String(autoMode) + ",";
  json += "\"relays\":[";
  for (int i = 0; i < soLuongDen; i++) {
    json += String(relayState[i]);
    if (i < soLuongDen - 1) json += ",";
  }
  json += "]}";

  sendCORSHeaders();
  server.send(200, "application/json", json);
}

void handleToggle() {
  if (!server.hasArg("id") || !server.hasArg("state")) {
    sendCORSHeaders();
    server.send(400, "text/plain", "Missing params");
    return;
  }

  int id = server.arg("id").toInt();
  int st = server.arg("state").toInt();

  if (autoMode == 1) {
    sendCORSHeaders();
    server.send(403, "text/plain", "Auto mode active");
    return;
  }

  if (id >= 0 && id < soLuongDen) {
    setRelay(id, st);
    sendCORSHeaders();
    server.send(200, "text/plain", "OK");
  } 
  else {
    sendCORSHeaders();
    server.send(400, "text/plain", "Invalid ID");
  }
}

void handleSetMode() {
  if (!server.hasArg("auto")) {
    sendCORSHeaders();
    server.send(400, "text/plain", "Missing mode");
    return;
  }

  autoMode = server.arg("auto").toInt();

  // Nếu chuyển sang Auto → để hệ thống tự điều chỉnh
  if (autoMode == 1) {
    Serial.println("Chuyen sang che do TU DONG");
  } 
  else {
    Serial.println("Chuyen sang che do THU CONG");
  }

  sendCORSHeaders();
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  // BH1750
  Wire.begin(21, 22);
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("BH1750 ready!");
  } 
  else {
    Serial.println("BH1750 error!");
  }

  // Relay pins
  for (int i = 0; i < soLuongDen; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);  // active-LOW => HIGH = OFF
  }

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP = ");
  Serial.println(WiFi.localIP());

  // API routes
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/toggle", HTTP_GET, handleToggle);
  server.on("/setMode", HTTP_GET, handleSetMode);

  // OPTIONS (preflight)
  server.on("/status", HTTP_OPTIONS, handleOptions);
  server.on("/toggle", HTTP_OPTIONS, handleOptions);
  server.on("/setMode", HTTP_OPTIONS, handleOptions);

  server.begin();
}

void loop() {
  server.handleClient();

  float lux = lightMeter.readLightLevel();

  if (autoMode == 1) {
    if (lux < 300) {
      setAll(1);   // bật hết
    } 
    else if (lux > 500) {
      setAll(0);   // tắt hết
    }
  }

  delay(300);
}