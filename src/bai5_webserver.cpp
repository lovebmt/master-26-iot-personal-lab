#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <SparkFun_SCD30_Arduino_Library.h>

#define LED_PIN 4 // Onboard RGB Red LED (Active-LOW)
#define I2C_SDA_PIN 27
#define I2C_SCL_PIN 22

const char* STA_SSID = "YOUR_WIFI_SSID";
const char* STA_PASS = "YOUR_WIFI_PASSWORD";
const char* AP_SSID  = "ESP32-Lab-AP";
const char* AP_PASS  = "12345678";

WebServer server(80);
SCD30 airSensor;
TFT_eSPI tft = TFT_eSPI();

bool ledState = false; // false = OFF (HIGH), true = ON (LOW)
float currentCO2 = 1140.0;
float currentTemp = 33.2;
float currentHum = 39.8;
unsigned long requestCount = 0;
unsigned long lastSensorRead = 0;

void setLed(bool state) {
    ledState = state;
    digitalWrite(LED_PIN, ledState ? LOW : HIGH); // Active-LOW
}

void initLCD() {
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    // Header
    tft.fillRoundRect(5, 5, 310, 42, 6, 0x18E3);
    tft.setTextColor(TFT_CYAN, 0x18E3);
    tft.drawString("BAI 5: ESP32 WEBSERVER", 14, 10, 4);
    tft.setTextColor(TFT_WHITE, 0x18E3);
    tft.drawString("Hoc vien: Le Duc Phuong - ESP32 CYD", 14, 30, 2);

    // IP Card
    tft.fillRoundRect(5, 52, 310, 36, 6, 0x10A2);
    tft.setTextColor(TFT_LIGHTGREY, 0x10A2);
    tft.drawString("STA IP:", 12, 56, 2);

    // Sensor SCD30 Card
    tft.fillRoundRect(5, 93, 200, 80, 6, 0x10A2);
    tft.drawString("SCD30 AIR SENSOR:", 12, 97, 2);

    // LED Card
    tft.fillRoundRect(210, 93, 105, 80, 6, 0x10A2);
    tft.drawString("LED GPIO 4:", 216, 97, 2);

    // Footer
    tft.fillRoundRect(5, 178, 310, 56, 6, 0x2124);
    tft.setTextColor(TFT_GREEN, 0x2124);
    tft.drawString("RESTful Webserver: Active (Port 80)", 12, 184, 2);
    tft.setTextColor(TFT_WHITE, 0x2124);
    tft.drawString("SoftAP: ESP32-Lab-AP (192.168.4.1)", 12, 206, 2);
}

void updateLCD() {
    // 1. IP
    tft.fillRoundRect(65, 54, 240, 30, 4, 0x10A2);
    tft.setTextColor(TFT_YELLOW, 0x10A2);
    String ipStr = "http://" + WiFi.localIP().toString();
    tft.drawString(ipStr, 70, 58, 2);

    // 2. SCD30 Sensor Info
    tft.fillRoundRect(12, 115, 186, 52, 4, 0x10A2);
    tft.setTextColor(TFT_GREENYELLOW, 0x10A2);
    char buf[64];
    sprintf(buf, "CO2: %.0f ppm", currentCO2);
    tft.drawString(buf, 14, 118, 2);

    tft.setTextColor(TFT_ORANGE, 0x10A2);
    sprintf(buf, "T: %.1f C | H: %.1f %%", currentTemp, currentHum);
    tft.drawString(buf, 14, 142, 2);

    // 3. LED State
    tft.fillRoundRect(216, 115, 92, 52, 4, 0x10A2);
    tft.setTextColor(ledState ? TFT_RED : TFT_DARKGREY, 0x10A2);
    tft.drawString(ledState ? "BAT (ON)" : "TAT (OFF)", 220, 130, 2);

    // 4. Requests count
    tft.fillRoundRect(12, 184, 296, 18, 2, 0x2124);
    tft.setTextColor(TFT_GREEN, 0x2124);
    sprintf(buf, "HTTP Requests: %lu | Mode: STA+AP", requestCount);
    tft.drawString(buf, 12, 184, 2);
}

// HTML Web UI Dark Mode
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 CYD - He Thong Giam Sat SCD30</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
        body { background: #0d1117; color: #c9d1d9; display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 20px; }
        .card { background: #161b22; border: 1px solid #30363d; border-radius: 16px; padding: 28px; width: 100%; max-width: 440px; box-shadow: 0 10px 30px rgba(0,0,0,0.5); }
        .header { text-align: center; margin-bottom: 24px; border-bottom: 1px solid #21262d; padding-bottom: 16px; }
        .header h1 { color: #58a6ff; font-size: 22px; margin-bottom: 6px; }
        .header p { color: #8b949e; font-size: 13px; }
        .sensor-card { background: #0d1117; border: 1px solid #30363d; border-radius: 12px; padding: 18px; margin-bottom: 18px; }
        .sensor-title { color: #8b949e; font-size: 11px; text-transform: uppercase; font-weight: 700; letter-spacing: 0.5px; margin-bottom: 10px; display: flex; align-items: center; gap: 8px; }
        .status-dot { width: 8px; height: 8px; border-radius: 50%; background: #3fb950; display: inline-block; }
        .co2-val { font-size: 38px; font-weight: 800; color: #7ee787; line-height: 1; }
        .sub-metrics { display: flex; gap: 12px; margin-top: 14px; padding-top: 12px; border-top: 1px solid #21262d; }
        .sub-box { flex: 1; }
        .sub-title { font-size: 11px; color: #8b949e; }
        .sub-val { font-size: 20px; font-weight: 700; }
        .temp-color { color: #ffa657; }
        .hum-color { color: #79c0ff; }
        .ctrl-card { background: #0d1117; border: 1px solid #30363d; border-radius: 12px; padding: 18px; margin-bottom: 18px; }
        .ctrl-row { display: flex; justify-content: space-between; align-items: center; margin-top: 10px; }
        .badge { padding: 6px 14px; border-radius: 20px; font-weight: 700; font-size: 12px; }
        .badge-off { background: rgba(248,81,73,0.15); color: #f85149; border: 1px solid rgba(248,81,73,0.4); }
        .badge-on { background: rgba(63,185,80,0.15); color: #3fb950; border: 1px solid rgba(63,185,80,0.4); }
        .btn-toggle { background: #1f6feb; color: white; border: none; padding: 10px 20px; border-radius: 8px; font-weight: 600; font-size: 14px; cursor: pointer; transition: 0.2s; }
        .btn-toggle:hover { background: #388bfd; }
        .footer { text-align: center; color: #8b949e; font-size: 11px; line-height: 1.5; }
    </style>
</head>
<body>
    <div class="card">
        <div class="header">
            <h1>ESP32 CYD WEBSERVER</h1>
            <p>Báo cáo IoT Thạc sĩ - Học viên: <strong>Lê Đức Phương</strong></p>
        </div>

        <div class="sensor-card">
            <div class="sensor-title">
                <span class="status-dot"></span>
                CẢM BIẾN SENSIRION SCD30 (I2C 0X61)
            </div>
            <div>
                <span id="co2" class="co2-val">---</span> <span style="color:#8b949e; font-size: 16px;">ppm</span>
            </div>
            <div class="sub-metrics">
                <div class="sub-box">
                    <div class="sub-title">NHIỆT ĐỘ</div>
                    <div id="temp" class="sub-val temp-color">--.- °C</div>
                </div>
                <div class="sub-box">
                    <div class="sub-title">ĐỘ ẨM</div>
                    <div id="humidity" class="sub-val hum-color">--.- %</div>
                </div>
            </div>
            <div style="font-size: 11px; color: #8b949e; margin-top: 8px;">Cập nhật tự động qua Fetch API (mỗi 1.5s)</div>
        </div>

        <div class="ctrl-card">
            <div class="sensor-title">ĐIỀU KHIỂN THIẾT BỊ (LED ONBOARD GPIO 4)</div>
            <div class="ctrl-row">
                <div>
                    <span style="font-size: 13px; color: #8b949e;">Trạng thái: </span>
                    <span id="led-badge" class="badge badge-off">ĐANG TẮT</span>
                </div>
                <button class="btn-toggle" onclick="toggleLed()">BẬT / TẮT LED</button>
            </div>
        </div>

        <div class="footer">
            Webserver IP: <strong id="ip-text">192.168.1.4</strong> | Cổng: 80<br>
            Cơ chế: AJAX/Fetch (phi khóa, không tải lại trang)
        </div>
    </div>

    <script>
        async function updateData() {
            try {
                const res = await fetch('/api/data');
                const data = await res.json();
                document.getElementById('co2').innerText = Math.round(data.co2);
                document.getElementById('temp').innerText = data.temp.toFixed(1) + ' °C';
                document.getElementById('humidity').innerText = data.humidity.toFixed(1) + ' %';
                document.getElementById('ip-text').innerText = data.ip;

                const badge = document.getElementById('led-badge');
                if (data.led === 'ON') {
                    badge.innerText = 'ĐANG BẬT';
                    badge.className = 'badge badge-on';
                } else {
                    badge.innerText = 'ĐANG TẮT';
                    badge.className = 'badge badge-off';
                }
            } catch (err) {
                console.error('Fetch error:', err);
            }
        }

        async function toggleLed() {
            try {
                const res = await fetch('/api/led/toggle');
                const data = await res.json();
                const badge = document.getElementById('led-badge');
                if (data.led === 'ON') {
                    badge.innerText = 'ĐANG BẬT';
                    badge.className = 'badge badge-on';
                } else {
                    badge.innerText = 'ĐANG TẮT';
                    badge.className = 'badge badge-off';
                }
            } catch (err) {
                console.error('Toggle error:', err);
            }
        }

        setInterval(updateData, 1500);
        updateData();
    </script>
</body>
</html>
)rawliteral";

void handleRoot() {
    requestCount++;
    server.send(200, "text/html", INDEX_HTML);
    updateLCD();
}

void handleData() {
    requestCount++;
    JsonDocument doc;
    doc["co2"] = round(currentCO2 * 10) / 10.0;
    doc["temp"] = round(currentTemp * 10) / 10.0;
    doc["humidity"] = round(currentHum * 10) / 10.0;
    doc["led"] = ledState ? "ON" : "OFF";
    doc["ip"] = WiFi.localIP().toString();
    doc["uptime"] = millis() / 1000;

    String jsonStr;
    serializeJson(doc, jsonStr);
    server.send(200, "application/json", jsonStr);
    updateLCD();
}

void handleLedToggle() {
    requestCount++;
    setLed(!ledState);

    Serial.printf("[%lu ms] [WEB API] Toggle LED -> %s\n",
                  millis(), ledState ? "BAT (ON)" : "TAT (OFF)");

    JsonDocument doc;
    doc["status"] = "success";
    doc["led"] = ledState ? "ON" : "OFF";
    doc["co2"] = round(currentCO2 * 10) / 10.0;
    doc["temp"] = round(currentTemp * 10) / 10.0;
    doc["humidity"] = round(currentHum * 10) / 10.0;

    String jsonStr;
    serializeJson(doc, jsonStr);
    server.send(200, "application/json", jsonStr);
    updateLCD();
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LED_PIN, OUTPUT);
    setLed(false); // Tat LED ban dau

    Serial.println("\n========================================================");
    Serial.println("  BAI 5: ESP32 RESTFUL WEBSERVER & AJAX SENSIRION SCD30");
    Serial.println("  Hoc vien: Le Duc Phuong");
    Serial.println("  Board   : ESP32-2432S028R (CYD)");
    Serial.println("  Sensor  : Sensirion SCD30 (I2C 0x61)");
    Serial.println("========================================================");

    initLCD();

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(50000);

    if (!airSensor.begin(Wire)) {
        Serial.println("[ERROR] Khong tim thay SCD30!");
    } else {
        Serial.println("[SUCCESS] Khoi tao Sensirion SCD30 thanh cong tai 0x61!");
        airSensor.setMeasurementInterval(2);
    }

    // 1. Ket noi Wi-Fi STA
    WiFi.mode(WIFI_AP_STA);
    Serial.printf("[WIFI] Dang ket noi toi: %s ...\n", STA_SSID);
    WiFi.begin(STA_SSID, STA_PASS);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 25) {
        delay(400);
        Serial.print(".");
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WIFI STA SUCCESS] IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\n[WIFI STA TIMEOUT] Dang chuyen sang fallback AP mode.");
    }

    // 2. Khoi tao SoftAP fallback
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.printf("[WIFI AP SUCCESS] SoftAP SSID: %s | IP: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());

    // 3. Dang ky cac REST API routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/data", HTTP_GET, handleData);
    server.on("/api/led/toggle", HTTP_GET, handleLedToggle);
    server.on("/api/led/toggle", HTTP_POST, handleLedToggle);

    server.begin();
    Serial.println("[HTTP SERVER] Webserver da khoi chay tren cong 80!");
    Serial.println("--------------------------------------------------------");
    Serial.printf("  Truy cap Web Dashboard: http://%s/\n", WiFi.localIP().toString().c_str());
    Serial.println("  API Telemetry:         http://<IP>/api/data");
    Serial.println("  API Toggle LED:        http://<IP>/api/led/toggle");
    Serial.println("--------------------------------------------------------\n");

    updateLCD();
}

void loop() {
    server.handleClient();

    if (millis() - lastSensorRead >= 2000) {
        lastSensorRead = millis();
        if (airSensor.dataAvailable()) {
            currentCO2 = airSensor.getCO2();
            currentTemp = airSensor.getTemperature();
            currentHum = airSensor.getHumidity();
            updateLCD();
        }
    }
}
