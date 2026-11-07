/**
 * BÀI 3. KẾT NỐI I2C CẢM BIẾN ÁNH SÁNG BH1750 TRÊN ESP32 CYD
 * Môn học: Internet of Things - Chương trình Thạc sĩ
 * Học viên: Lê Đức Phương
 *
 * SDA: GPIO 27   SCL: GPIO 22   Địa chỉ I2C: 0x23
 * Mở rộng: Hiển thị giá trị Lux thời gian thực trực tiếp lên màn hình LCD 2.8"
 * Nền tảng Cloud: CoreIOT (app.coreiot.io:1883)
 */

#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define I2C_SDA_PIN 27
#define I2C_SCL_PIN 22
#define BH1750_I2C_ADDR 0x23

// Cấu hình Wi-Fi & CoreIOT
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* COREIOT_HOST  = "app.coreiot.io";
const int   COREIOT_PORT  = 1883;
const char* ACCESS_TOKEN  = "YOUR_COREIOT_TOKEN"; // Token thiết bị ESP32_Lab_Device

TFT_eSPI tft = TFT_eSPI();
BH1750 lightMeter;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL_MS = 1500;
bool sensorReady = false;

void drawStaticUI() {
    tft.fillScreen(TFT_BLACK);

    // Header Bar
    tft.fillRect(0, 0, 320, 34, 0x0A82); // Dark Teal
    tft.setTextColor(TFT_WHITE, 0x0A82);
    tft.drawString("BAI 3: I2C SENSOR BH1750", 12, 8, 4);

    // Sub header info
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("Hoc vien: Le Duc Phuong", 12, 42, 2);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("Bus I2C : SDA=GPIO 27, SCL=GPIO 22", 12, 60, 2);
    tft.drawString("Address : 0x23 (Continuous High-Res)", 12, 78, 2);

    // Divider
    tft.drawFastHLine(10, 98, 300, TFT_DARKGREY);

    // Section title
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("CUONG DO ANH SANG (LUX):", 12, 108, 2);
}

void updateLuxOnDisplay(float lux, bool mqttOk) {
    // Card container
    uint16_t cardBg = 0x01E4; // Deep Navy/Slate
    tft.fillRoundRect(12, 128, 296, 68, 8, cardBg);
    tft.drawRoundRect(12, 128, 296, 68, 8, TFT_CYAN);

    char valStr[32];
    sprintf(valStr, "%.1f Lux", lux);

    tft.setTextColor(TFT_GREENYELLOW, cardBg);
    tft.drawString(valStr, 35, 142, 6); // Font size 6: big numbers!

    // Bottom status bar
    tft.fillRect(12, 204, 296, 28, TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("I2C: OK (0x23) | ", 12, 208, 2);

    if (mqttOk) {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("CoreIOT: CONNECTED", 130, 208, 2);
    } else {
        tft.setTextColor(TFT_ORANGE, TFT_BLACK);
        tft.drawString("CoreIOT: LOCAL ONLY", 130, 208, 2);
    }
}

void scanI2CBus() {
    Serial.println("\n------------------------------------------------");
    Serial.printf("[I2C SCANNER] Quet tren bus SDA=%d, SCL=%d...\n", I2C_SDA_PIN, I2C_SCL_PIN);
    byte count = 0;
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
        if (error == 0) {
            Serial.printf("  -> Tim thay thiet bi I2C tai: 0x%02X", address);
            if (address == 0x23) Serial.print(" (Cam bien quang BH1750)");
            Serial.println();
            count++;
        }
    }
    Serial.printf("[I2C SCANNER] Tim thay tong cong %d thiet bi.\n", count);
    Serial.println("------------------------------------------------\n");
}

bool publishTelemetry(float lux) {
    if (WiFi.status() != WL_CONNECTED) return false;

    if (!mqttClient.connected()) {
        if (!mqttClient.connect("ESP32_CYD_B3", ACCESS_TOKEN, NULL)) {
            return false;
        }
    }

    JsonDocument doc;
    doc["light_lux"] = lux;
    char payload[128];
    serializeJson(doc, payload);

    bool res = mqttClient.publish("v1/devices/me/telemetry", payload);
    if (res) {
        Serial.printf("[COREIOT MQTT] Published: %s\n", payload);
    }
    return res;
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // 1. Khởi tạo LCD
    tft.init();
    tft.setRotation(1); // 320x240
    drawStaticUI();

    Serial.println("\n========================================================");
    Serial.println("  BAI 3: GIAO TIEP I2C CAM BIEN BH1750 + MAN HINH LCD");
    Serial.println("  Hoc vien: Le Duc Phuong");
    Serial.println("  Board   : ESP32-2432S028R (CYD)");
    Serial.println("  I2C Pins: SDA = GPIO 27, SCL = GPIO 22");
    Serial.println("  Address : 0x23 (Continuous High-Res)");
    Serial.println("========================================================");

    // 2. Khởi tạo I2C
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    scanI2CBus();

    // 3. Khởi tạo BH1750
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, BH1750_I2C_ADDR, &Wire)) {
        Serial.println("[BH1750] Khoi tao cam bien thanh cong!");
        sensorReady = true;
    } else {
        Serial.println("[BH1750 ERROR] Khoi tao that bai!");
    }

    // 4. Kết nối Wi-Fi thử nghiệm (timeout ngắn 3s để không chặn luồng)
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    mqttClient.setServer(COREIOT_HOST, COREIOT_PORT);
}

void loop() {
    if (mqttClient.connected()) {
        mqttClient.loop();
    }

    unsigned long currentMillis = millis();
    if (currentMillis - lastReadTime >= READ_INTERVAL_MS) {
        lastReadTime = currentMillis;

        if (sensorReady) {
            float lux = lightMeter.readLightLevel();
            if (lux >= 0.0) {
                bool pubOk = publishTelemetry(lux);
                updateLuxOnDisplay(lux, pubOk);
                Serial.printf("[%lu ms] [BH1750 SENSOR] Cuong do sang: %.2f Lux\n", currentMillis, lux);
            } else {
                Serial.println("[BH1750] Loi doc du lieu I2C!");
            }
        }
    }
}
