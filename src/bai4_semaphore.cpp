#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <SparkFun_SCD30_Arduino_Library.h>

#define I2C_SDA_PIN 27
#define I2C_SCL_PIN 22

struct SensorData {
    float co2;
    float temp;
    float hum;
    unsigned long timestamp;
};

SensorData sharedData;
SemaphoreHandle_t xSemaphore = NULL;
SCD30 airSensor;
TFT_eSPI tft = TFT_eSPI();

unsigned long syncCount = 0;

void initLCD() {
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    // Header
    tft.fillRoundRect(5, 5, 310, 42, 6, 0x18E3);
    tft.setTextColor(TFT_CYAN, 0x18E3);
    tft.drawString("BAI 4: FREERTOS SEMAPHORE", 14, 10, 4);
    tft.setTextColor(TFT_WHITE, 0x18E3);
    tft.drawString("Hoc vien: Le Duc Phuong - ESP32 CYD", 14, 30, 2);

    // Task Sensor Card
    tft.fillRoundRect(5, 52, 310, 56, 6, 0x10A2);
    tft.setTextColor(TFT_YELLOW, 0x10A2);
    tft.drawString("TASK 1: vTaskSensor (Priority 2)", 12, 56, 2);

    // Task Logger Card
    tft.fillRoundRect(5, 113, 310, 56, 6, 0x10A2);
    tft.setTextColor(TFT_GREEN, 0x10A2);
    tft.drawString("TASK 2: vTaskLogger (Priority 1)", 12, 117, 2);

    // Sync Status Card
    tft.fillRoundRect(5, 175, 310, 60, 6, 0x2124);
    tft.setTextColor(TFT_CYAN, 0x2124);
    tft.drawString("SEMAPHORE STATUS:", 12, 180, 2);
}

void updateLCD(float co2, float temp, float hum, unsigned long count) {
    // Task 1 info
    tft.fillRoundRect(12, 75, 296, 28, 4, 0x10A2);
    tft.setTextColor(TFT_WHITE, 0x10A2);
    char s1[64];
    sprintf(s1, "Doc SCD30 (2s) -> Give() Sem");
    tft.drawString(s1, 14, 80, 2);

    // Task 2 info
    tft.fillRoundRect(12, 136, 296, 28, 4, 0x10A2);
    tft.setTextColor(TFT_GREENYELLOW, 0x10A2);
    char s2[64];
    sprintf(s2, "CO2:%.0f | T:%.1fC | H:%.1f%%", co2, temp, hum);
    tft.drawString(s2, 14, 140, 2);

    // Sync status
    tft.fillRoundRect(12, 202, 296, 28, 4, 0x2124);
    tft.setTextColor(TFT_GREEN, 0x2124);
    char s3[64];
    sprintf(s3, "SYNC #%lu OK (Take portMAX_DELAY)", count);
    tft.drawString(s3, 14, 204, 2);
}

// Task 1: vTaskSensor (Priority 2 - Cao hon)
void vTaskSensor(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(2000);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (airSensor.dataAvailable()) {
            sharedData.co2 = airSensor.getCO2();
            sharedData.temp = airSensor.getTemperature();
            sharedData.hum = airSensor.getHumidity();
            sharedData.timestamp = millis();

            Serial.printf("[%lu ms] [TASK SENSOR (Prio 2)] Doc SCD30: CO2=%.1f ppm, T=%.2f C, H=%.2f %%\n",
                          sharedData.timestamp, sharedData.co2, sharedData.temp, sharedData.hum);
            Serial.printf("[%lu ms] [TASK SENSOR] Phat Semaphore: xSemaphoreGive()...\n", millis());

            xSemaphoreGive(xSemaphore);
        }
    }
}

// Task 2: vTaskLogger (Priority 1 - Thap hon)
void vTaskLogger(void *pvParameters) {
    for (;;) {
        // Cho vo han cho den khi TaskSensor phat Semaphore
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
            syncCount++;
            Serial.printf("[%lu ms] [TASK LOGGER (Prio 1)] >>> Nhan duoc Semaphore #%lu <<<\n",
                          millis(), syncCount);
            Serial.printf("    [DONG BO THANH CONG] Shared Data: CO2=%.1f ppm | Temp=%.2f C | Hum=%.2f %% (Ghi nhan luc %lu ms)\n\n",
                          sharedData.co2, sharedData.temp, sharedData.hum, sharedData.timestamp);

            updateLCD(sharedData.co2, sharedData.temp, sharedData.hum, syncCount);
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n========================================================");
    Serial.println("  BAI 4: DONG BO HOA TASK VOI FREERTOS BINARY SEMAPHORE");
    Serial.println("  Hoc vien: Le Duc Phuong");
    Serial.println("  Board   : ESP32-2432S028R (CYD)");
    Serial.println("  Sensor  : Sensirion SCD30 (I2C 0x61)");
    Serial.println("  Co che  : Binary Semaphore (Producer - Consumer)");
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

    // Tao Binary Semaphore
    xSemaphore = xSemaphoreCreateBinary();
    if (xSemaphore == NULL) {
        Serial.println("[ERROR] Khong the tao Semaphore!");
        while (1);
    }
    Serial.println("[FREERTOS] Da tao thanh cong Binary Semaphore.");

    // Tao Task 1: vTaskSensor (Priority 2 - Producer)
    xTaskCreatePinnedToCore(
        vTaskSensor,
        "TaskSensor",
        4096,
        NULL,
        2,
        NULL,
        1
    );
    Serial.println("[FREERTOS] Da tao TaskSensor (Priority 2) tren Core 1.");

    // Tao Task 2: vTaskLogger (Priority 1 - Consumer)
    xTaskCreatePinnedToCore(
        vTaskLogger,
        "TaskLogger",
        4096,
        NULL,
        1,
        NULL,
        1
    );
    Serial.println("[FREERTOS] Da tao TaskLogger (Priority 1) tren Core 1.");
}

void loop() {
    vTaskDelete(NULL); // Huy loop() de danh toan bo CPU cho FreeRTOS tasks
}
