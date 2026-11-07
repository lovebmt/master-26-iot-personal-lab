#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <SparkFun_SCD30_Arduino_Library.h>

// Pin I2C tren jack CN1 cua CYD
#define I2C_SDA_PIN 27
#define I2C_SCL_PIN 22

// Thong tin Wi-Fi & CoreIOT (Bảo mật: tự động nạp credentials nội bộ nếu có, mặc định placeholder khi push Git)
#if __has_include("credentials.h")
#include "credentials.h"
const char* WIFI_SSID    = LOCAL_WIFI_SSID;
const char* WIFI_PASS    = LOCAL_WIFI_PASS;
const char* ACCESS_TOKEN = LOCAL_COREIOT_TOKEN;
#else
const char* WIFI_SSID    = "YOUR_WIFI_SSID";
const char* WIFI_PASS    = "YOUR_WIFI_PASSWORD";
const char* ACCESS_TOKEN = "YOUR_COREIOT_TOKEN";
#endif
const char* MQTT_SERVER  = "app.coreiot.io";
const int   MQTT_PORT    = 1883;

SCD30 airSensor;
TFT_eSPI tft = TFT_eSPI();
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 2000;

void scanI2C() {
    Serial.println("\n------------------------------------------------");
    Serial.printf("[I2C SCANNER] Quet tren bus SDA=%d, SCL=%d...\n", I2C_SDA_PIN, I2C_SCL_PIN);
    byte count = 0;
    for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        byte error = Wire.endTransmission();
        if (error == 0) {
            Serial.printf("  -> Tim thay thiet bi I2C tai: 0x%02X", addr);
            if (addr == 0x61) {
                Serial.print(" (Cam bien Sensirion SCD30 - NDIR CO2/Temp/Hum)");
            }
            Serial.println();
            count++;
        }
    }
    Serial.printf("[I2C SCANNER] Tim thay tong cong %d thiet bi.\n", count);
    Serial.println("------------------------------------------------\n");
}

void initLCD() {
    tft.init();
    tft.setRotation(1); // Landscape 320x240
    tft.fillScreen(TFT_BLACK);

    // Header Card
    tft.fillRoundRect(5, 5, 310, 42, 6, 0x18E3); // Dark Navy Blue
    tft.setTextColor(TFT_CYAN, 0x18E3);
    tft.drawString("BAI 3: SENSIRION SCD30 (I2C)", 14, 10, 4);
    tft.setTextColor(TFT_WHITE, 0x18E3);
    tft.drawString("Hoc vien: Le Duc Phuong - ESP32 CYD", 14, 30, 2);

    // Frame Card CO2
    tft.fillRoundRect(5, 52, 310, 56, 6, 0x10A2);
    tft.setTextColor(TFT_LIGHTGREY, 0x10A2);
    tft.drawString("CO2 CONCENTRATION (NDIR):", 12, 56, 2);

    // Frame Card Temp & Hum
    tft.fillRoundRect(5, 113, 152, 60, 6, 0x10A2);
    tft.drawString("TEMPERATURE:", 12, 117, 2);

    tft.fillRoundRect(163, 113, 152, 60, 6, 0x10A2);
    tft.drawString("HUMIDITY:", 170, 117, 2);

    // Footer Card
    tft.fillRoundRect(5, 178, 310, 56, 6, 0x2124);
    tft.setTextColor(TFT_GREEN, 0x2124);
    tft.drawString("Status: Dang khoi tao SCD30 (0x61)...", 12, 185, 2);
    tft.setTextColor(TFT_WHITE, 0x2124);
    tft.drawString("CoreIOT MQTT: app.coreiot.io", 12, 208, 2);
}

void updateLCD(float co2, float temp, float hum, bool mqttOk) {
    // 1. CO2
    tft.fillRoundRect(12, 72, 296, 32, 4, 0x10A2);
    tft.setTextColor(TFT_GREENYELLOW, 0x10A2);
    char co2Str[32];
    sprintf(co2Str, "%.1f ppm", co2);
    tft.drawString(co2Str, 14, 73, 4);

    // 2. Temp
    tft.fillRoundRect(12, 134, 138, 34, 4, 0x10A2);
    tft.setTextColor(TFT_ORANGE, 0x10A2);
    char tempStr[20];
    sprintf(tempStr, "%.1f C", temp);
    tft.drawString(tempStr, 14, 136, 4);

    // 3. Humidity
    tft.fillRoundRect(170, 134, 138, 34, 4, 0x10A2);
    tft.setTextColor(TFT_CYAN, 0x10A2);
    char humStr[20];
    sprintf(humStr, "%.1f %%", hum);
    tft.drawString(humStr, 172, 136, 4);

    // 4. Footer
    tft.fillRoundRect(12, 185, 296, 20, 2, 0x2124);
    tft.setTextColor(mqttOk ? TFT_GREEN : TFT_YELLOW, 0x2124);
    if (mqttOk) {
        tft.drawString("SCD30 OK (0x61) | MQTT: Connected", 12, 185, 2);
    } else {
        tft.drawString("SCD30 OK (0x61) | WiFi: Connecting...", 12, 185, 2);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n========================================================");
    Serial.println("  BAI 3: GIAO TIEP I2C CAM BIEN SENSIRION SCD30 + LCD");
    Serial.println("  Hoc vien: Le Duc Phuong");
    Serial.println("  Board   : ESP32-2432S028R (Cheap Yellow Display)");
    Serial.println("  Sensor  : Sensirion SCD30 (NDIR CO2 + Temp + Humidity)");
    Serial.printf ("  I2C Pins: SDA = GPIO %d, SCL = GPIO %d\n", I2C_SDA_PIN, I2C_SCL_PIN);
    Serial.println("  Address : 0x61");
    Serial.println("========================================================");

    initLCD();

    // Khoi tao I2C tren GPIO 27 (SDA) va GPIO 22 (SCL)
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(50000);

    scanI2C();

    Serial.println("[SCD30] Dang ket noi cam bien Sensirion SCD30...");
    if (!airSensor.begin(Wire)) {
        Serial.println("[ERROR] Khong the ket noi SCD30! Kiem tra day noi SDA/SCL.");
    } else {
        Serial.println("[SUCCESS] Ket noi Sensirion SCD30 thanh cong tai dia chi 0x61!");
        airSensor.setMeasurementInterval(2);
    }

    // Ket noi Wi-Fi
    WiFi.mode(WIFI_STA);
    Serial.printf("[WIFI] Dang ket noi toi SSID: %s ...\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    client.setServer(MQTT_SERVER, MQTT_PORT);
}

void loop() {
    // Reconnect WiFi neu can
    if (WiFi.status() == WL_CONNECTED) {
        if (!client.connected()) {
            if (client.connect("ESP32_SCD30_Device", ACCESS_TOKEN, NULL)) {
                Serial.println("[COREIOT MQTT] Ket noi thanh cong toi CoreIOT Broker!");
            }
        }
        client.loop();
    }

    if (millis() - lastReadTime >= READ_INTERVAL) {
        lastReadTime = millis();

        if (airSensor.dataAvailable()) {
            float co2 = airSensor.getCO2();
            float temp = airSensor.getTemperature();
            float hum = airSensor.getHumidity();

            Serial.printf("[%lu ms] [SCD30 SENSOR] CO2: %.1f ppm | Temp: %.2f C | Hum: %.2f %%\n",
                          millis(), co2, temp, hum);

            bool mqttOk = false;
            if (client.connected()) {
                JsonDocument doc;
                doc["co2"] = round(co2 * 10) / 10.0;
                doc["temperature"] = round(temp * 100) / 100.0;
                doc["humidity"] = round(hum * 100) / 100.0;

                char jsonBuf[128];
                serializeJson(doc, jsonBuf);
                client.publish("v1/devices/me/telemetry", jsonBuf);
                Serial.printf("[COREIOT MQTT] Published: %s\n", jsonBuf);
                mqttOk = true;
            }

            updateLCD(co2, temp, hum, mqttOk);
        } else {
            Serial.printf("[%lu ms] [SCD30 SENSOR] Dang cho du lieu san sang...\n", millis());
        }
    }
}
