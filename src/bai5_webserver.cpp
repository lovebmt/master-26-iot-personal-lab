/**
 * BÀI 5. THIẾT KẾ WEBSERVER TRÊN ESP32 CYD
 * Môn học: Internet of Things - Chương trình Thạc sĩ
 * Học viên: Lê Đức Phương
 *
 * Chế độ Wi-Fi: Dual Mode (STA kết nối Router + SoftAP dự phòng)
 * Địa chỉ IP:
 *   - SoftAP : http://192.168.4.1 (SSID: ESP32-Lab-AP, Pass: 12345678)
 *   - Station: Cấp phát từ DHCP Router Wi-Fi
 * Các route RESTful:
 *   - GET  /               : Trang chủ Dashboard HTML/CSS/JS responsive
 *   - GET  /api/data       : Trả về JSON cường độ sáng và trạng thái thiết bị
 *   - POST /api/led/toggle : Bật/Tắt LED onboard GPIO 4 qua Web
 * Mở rộng: Hiển thị địa chỉ IP, trạng thái LED và số request trên LCD 2.8"
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <BH1750.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>

#define I2C_SDA_PIN 27
#define I2C_SCL_PIN 22
#define BH1750_I2C_ADDR 0x23
#define LED_PIN 4

// Cấu hình Wi-Fi
const char* STA_SSID     = "YOUR_WIFI_SSID";
const char* STA_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* AP_SSID      = "ESP32-Lab-AP";
const char* AP_PASSWORD  = "12345678";

WebServer server(80);
TFT_eSPI tft = TFT_eSPI();
BH1750 lightMeter;

bool ledState = false; // false = OFF (HIGH), true = ON (LOW)
float currentLux = 0.0;
unsigned long requestCounter = 0;
String activeIP = "";

// -------------------------------------------------------------
// GIAO DIỆN HTML/CSS/JS (RESPONSIVE DASHBOARD)
// -------------------------------------------------------------
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 IoT Dashboard - Le Duc Phuong</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body { background: #0f172a; color: #f8fafc; display: flex; flex-direction: column; align-items: center; min-height: 100vh; padding: 20px; }
    .container { width: 100%; max-width: 520px; background: #1e293b; border-radius: 16px; box-shadow: 0 10px 25px rgba(0,0,0,0.5); padding: 24px; border: 1px solid #334155; }
    header { text-align: center; border-bottom: 1px solid #334155; padding-bottom: 16px; margin-bottom: 20px; }
    h1 { font-size: 20px; color: #38bdf8; margin-bottom: 6px; }
    .author { font-size: 13px; color: #94a3b8; }
    .card { background: #0f172a; border-radius: 12px; padding: 18px; margin-bottom: 16px; border: 1px solid #1e293b; }
    .card-title { font-size: 12px; text-transform: uppercase; color: #94a3b8; letter-spacing: 1px; margin-bottom: 8px; }
    .sensor-val { font-size: 40px; font-weight: 700; color: #4ade80; display: flex; align-items: baseline; gap: 8px; }
    .unit { font-size: 18px; color: #94a3b8; font-weight: 400; }
    .ctrl-section { display: flex; align-items: center; justify-content: space-between; margin-top: 10px; }
    .status-badge { display: inline-block; padding: 6px 14px; border-radius: 20px; font-size: 13px; font-weight: 600; }
    .badge-on { background: #22c55e22; color: #4ade80; border: 1px solid #22c55e; }
    .badge-off { background: #ef444422; color: #f87171; border: 1px solid #ef4444; }
    button { background: #0284c7; color: white; border: none; padding: 12px 24px; border-radius: 10px; font-size: 15px; font-weight: 600; cursor: pointer; transition: 0.2s; }
    button:hover { background: #0369a1; }
    button:active { transform: scale(0.97); }
    footer { margin-top: 20px; font-size: 12px; color: #64748b; text-align: center; line-height: 1.6; }
    .live-dot { width: 8px; height: 8px; background: #22c55e; border-radius: 50%; display: inline-block; animation: pulse 1.5s infinite; margin-right: 6px; }
    @keyframes pulse { 0% { opacity: 1; transform: scale(1); } 50% { opacity: 0.4; transform: scale(1.3); } 100% { opacity: 1; transform: scale(1); } }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>ESP32 CYD WEBSERVER</h1>
      <p class="author">Báo cáo IoT Thạc sĩ - Học viên: <strong>Lê Đức Phương</strong></p>
    </header>

    <div class="card">
      <div class="card-title"><span class="live-dot"></span>Cảm biến ánh sáng BH1750 (I2C 0x23)</div>
      <div class="sensor-val"><span id="luxVal">--.-</span> <span class="unit">Lux</span></div>
      <small style="color:#64748b; font-size:11px;">Cập nhật tự động qua Fetch API (mỗi 1.5s)</small>
    </div>

    <div class="card">
      <div class="card-title">Điều khiển thiết bị (LED Onboard GPIO 4)</div>
      <div class="ctrl-section">
        <div>Trạng thái: <span id="ledBadge" class="status-badge badge-off">ĐANG TẮT</span></div>
        <button id="toggleBtn" onclick="toggleLED()">BẬT / TẮT LED</button>
      </div>
    </div>

    <footer>
      <div>Webserver IP: <strong id="ipDisplay">...</strong> | Cổng: 80</div>
      <div>Cơ chế: AJAX/Fetch (phi khóa, không tải lại trang)</div>
    </footer>
  </div>

  <script>
    function updateData() {
      fetch('/api/data')
        .then(res => res.json())
        .then(data => {
          document.getElementById('luxVal').innerText = data.lux.toFixed(1);
          const badge = document.getElementById('ledBadge');
          if (data.led === 'ON') {
            badge.innerText = 'ĐANG BẬT';
            badge.className = 'status-badge badge-on';
          } else {
            badge.innerText = 'ĐANG TẮT';
            badge.className = 'status-badge badge-off';
          }
          document.getElementById('ipDisplay').innerText = data.ip;
        })
        .catch(err => console.log('Fetch error:', err));
    }

    function toggleLED() {
      fetch('/api/led/toggle', { method: 'POST' })
        .then(res => res.json())
        .then(data => { updateData(); });
    }

    // Tự động cập nhật mỗi 1500ms
    setInterval(updateData, 1500);
    updateData();
  </script>
</body>
</html>
)rawliteral";

void drawStaticUI() {
    tft.fillScreen(TFT_BLACK);

    // Header Bar
    tft.fillRect(0, 0, 320, 34, 0x03E0); // Deep Forest Green
    tft.setTextColor(TFT_WHITE, 0x03E0);
    tft.drawString("BAI 5: ESP32 WEBSERVER", 12, 8, 4);

    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("Hoc vien: Le Duc Phuong", 12, 42, 2);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("Architecture: RESTful Server (STA/AP)", 12, 60, 2);

    tft.drawFastHLine(10, 80, 300, TFT_DARKGREY);

    // IP Address Banner
    tft.fillRect(12, 88, 296, 32, 0x18E3);
    tft.drawRect(12, 88, 296, 32, TFT_SKYBLUE);
    tft.setTextColor(TFT_WHITE, 0x18E3);
    tft.drawString("URL:", 20, 96, 2);
    tft.setTextColor(TFT_YELLOW, 0x18E3);
    tft.drawString("http://" + activeIP, 60, 96, 2);
}

void updateWebserverUI(float lux, bool state, unsigned long reqs) {
    // Card 1: Sensor Lux
    tft.fillRoundRect(12, 128, 142, 70, 6, 0x01E4);
    tft.drawRoundRect(12, 128, 142, 70, 6, TFT_CYAN);
    tft.setTextColor(TFT_WHITE, 0x01E4);
    tft.drawString("BH1750 LUX", 20, 136, 2);
    char bufLux[16];
    sprintf(bufLux, "%.1f", lux);
    tft.setTextColor(TFT_GREENYELLOW, 0x01E4);
    tft.drawString(bufLux, 25, 156, 4);

    // Card 2: LED State
    uint16_t ledBg = state ? 0xB800 : 0x2124;
    tft.fillRoundRect(166, 128, 142, 70, 6, ledBg);
    tft.drawRoundRect(166, 128, 142, 70, 6, state ? TFT_RED : TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, ledBg);
    tft.drawString("LED GPIO 4", 175, 136, 2);
    tft.setTextColor(state ? TFT_WHITE : TFT_SILVER, ledBg);
    tft.drawString(state ? "BAT (ON)" : "TAT (OFF)", 175, 156, 4);

    // Bottom Stats
    char bufStat[64];
    sprintf(bufStat, "HTTP Requests: %lu | Mode: STA+AP", reqs);
    tft.fillRect(12, 206, 296, 24, TFT_BLACK);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString(bufStat, 12, 210, 2);
}

// -------------------------------------------------------------
// HTTP ROUTE HANDLERS
// -------------------------------------------------------------
void handleRoot() {
    requestCounter++;
    server.send(200, "text/html", INDEX_HTML);
}

void handleData() {
    requestCounter++;
    JsonDocument doc;
    doc["lux"] = currentLux;
    doc["led"] = ledState ? "ON" : "OFF";
    doc["ip"]  = activeIP;
    doc["uptime"] = millis() / 1000;
    
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

void handleToggleLED() {
    requestCounter++;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? LOW : HIGH); // Active-LOW

    Serial.printf("[HTTP POST /api/led/toggle] LED chuyen thanh: %s\n", ledState ? "ON" : "OFF");
    handleData();
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // Tắt LED ban đầu (Active-LOW)

    // Khởi tạo I2C & BH1750
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, BH1750_I2C_ADDR, &Wire);

    // Khởi tạo Wi-Fi DUAL MODE (AP + STA)
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    IPAddress apIP = WiFi.softAPIP();

    Serial.println("\n========================================================");
    Serial.println("  BAI 5: THIET KE WEBSERVER ESP32 CYD (STA + SOFTAP)");
    Serial.println("  Hoc vien: Le Duc Phuong");
    Serial.println("  Board   : ESP32-2432S028R (CYD) + LCD ILI9341");
    Serial.printf ("  SoftAP  : SSID='%s', Pass='%s'\n", AP_SSID, AP_PASSWORD);
    Serial.printf ("  SoftAP IP: http://%s\n", apIP.toString().c_str());

    // Thử kết nối STA
    WiFi.begin(STA_SSID, STA_PASSWORD);
    unsigned long startTry = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTry < 4000) {
        delay(200);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        activeIP = WiFi.localIP().toString();
        Serial.printf("\n  Station Connected! IP: http://%s\n", activeIP.c_str());
    } else {
        activeIP = apIP.toString();
        Serial.printf("\n  Dung IP SoftAP: http://%s\n", activeIP.c_str());
    }
    Serial.println("========================================================\n");

    // Khởi tạo LCD
    tft.init();
    tft.setRotation(1);
    drawStaticUI();
    updateWebserverUI(currentLux, ledState, requestCounter);

    // Đăng ký RESTful Routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/data", HTTP_GET, handleData);
    server.on("/api/led/toggle", HTTP_POST, handleToggleLED);
    server.on("/api/led/toggle", HTTP_GET, handleToggleLED); // Hỗ trợ cả GET để test dễ bằng browser

    server.begin();
    Serial.println("[WEBSERVER] HTTP Server khoi dong thanh cong tren cong 80!");
}

void loop() {
    server.handleClient();

    static unsigned long lastSensorRead = 0;
    if (millis() - lastSensorRead >= 1000) {
        lastSensorRead = millis();
        float lux = lightMeter.readLightLevel();
        if (lux >= 0.0) {
            currentLux = lux;
        }
        updateWebserverUI(currentLux, ledState, requestCounter);
    }
}
