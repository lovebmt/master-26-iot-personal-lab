/**
 * BÀI 4. LẬP TRÌNH SEMAPHORE TRÊN ESP32 CYD (FREERTOS)
 * Môn học: Internet of Things - Chương trình Thạc sĩ
 * Học viên: Lê Đức Phương
 *
 * Loại Semaphore: Binary Semaphore (xSemaphoreCreateBinary)
 * Các tác vụ:
 *   - TaskSensor (Độ ưu tiên 2): Đọc cảm biến BH1750 định kỳ, gửi tín hiệu (Give)
 *   - TaskLogger (Độ ưu tiên 1): Chờ nhận tín hiệu (Take), in Serial & cập nhật LCD
 * Mở rộng: Trực quan hóa tiến trình đồng bộ FreeRTOS trên màn hình LCD 2.8"
 */

#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <TFT_eSPI.h>

#define I2C_SDA_PIN 27
#define I2C_SCL_PIN 22
#define BH1750_I2C_ADDR 0x23
#define LED_PIN 4

// Khởi tạo ngoại vi
TFT_eSPI tft = TFT_eSPI();
BH1750 lightMeter;

// Khai báo Semaphore và biến dữ liệu dùng chung (Shared Resource)
SemaphoreHandle_t xBinarySemaphore = NULL;
volatile float sharedSensorValue = 0.0;
volatile unsigned long giveCount = 0;
volatile unsigned long takeCount = 0;

void drawStaticUI() {
    tft.fillScreen(TFT_BLACK);

    // Header Bar
    tft.fillRect(0, 0, 320, 34, 0x500A); // Deep Maroon / Violet
    tft.setTextColor(TFT_WHITE, 0x500A);
    tft.drawString("BAI 4: FREERTOS SEMAPHORE", 12, 8, 4);

    // Sub header info
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("Hoc vien: Le Duc Phuong", 12, 42, 2);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("Sync Type: Binary Semaphore (Signaling)", 12, 60, 2);
    tft.drawString("Tasks    : TaskSensor (Prio 2) -> TaskLogger (Prio 1)", 12, 78, 2);

    tft.drawFastHLine(10, 96, 300, TFT_DARKGREY);
}

void updateSemaphoreUI(float val, unsigned long gCount, unsigned long tCount) {
    // Card Task 1: TaskSensor (GIVE)
    tft.fillRoundRect(12, 104, 296, 38, 6, 0x01E4);
    tft.drawRoundRect(12, 104, 296, 38, 6, TFT_CYAN);
    tft.setTextColor(TFT_WHITE, 0x01E4);
    tft.drawString("TaskSensor (Prio 2):", 20, 114, 2);
    char buf1[32];
    sprintf(buf1, "GIVE #%lu", gCount);
    tft.setTextColor(TFT_GREENYELLOW, 0x01E4);
    tft.drawString(buf1, 200, 114, 2);

    // Card Task 2: TaskLogger (TAKE)
    tft.fillRoundRect(12, 148, 296, 38, 6, 0x30A2);
    tft.drawRoundRect(12, 148, 296, 38, 6, TFT_GREEN);
    tft.setTextColor(TFT_WHITE, 0x30A2);
    tft.drawString("TaskLogger (Prio 1):", 20, 158, 2);
    char buf2[32];
    sprintf(buf2, "TAKE #%lu", tCount);
    tft.setTextColor(TFT_YELLOW, 0x30A2);
    tft.drawString(buf2, 200, 158, 2);

    // Bottom Value Bar
    char bufVal[48];
    sprintf(bufVal, "Shared Value: %.2f Lux", val);
    tft.fillRect(12, 196, 296, 36, TFT_BLACK);
    tft.drawRect(12, 196, 296, 36, TFT_DARKGREY);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(bufVal, 40, 204, 4);
}

// -------------------------------------------------------------
// TASK 1: Đọc cảm biến BH1750 và Give Semaphore
// -------------------------------------------------------------
void TaskSensor(void *pvParameters) {
    (void) pvParameters;
    for (;;) {
        float lux = lightMeter.readLightLevel();
        if (lux < 0.0) lux = 0.0;

        // Cập nhật tài nguyên dùng chung
        sharedSensorValue = lux;
        giveCount++;

        Serial.printf("[Task 1 - Sensor] (Prio 2) Doc Cam Bien: %.2f Lux -> [xSemaphoreGive]\n", lux);

        // Báo hiệu cho TaskLogger qua Binary Semaphore
        xSemaphoreGive(xBinarySemaphore);

        // Chu kỳ lấy mẫu 2000ms (dùng vTaskDelay nhường CPU)
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// -------------------------------------------------------------
// TASK 2: Chờ Semaphore, In Log và Cập nhật Hiển thị
// -------------------------------------------------------------
void TaskLogger(void *pvParameters) {
    (void) pvParameters;
    for (;;) {
        // Chờ nhận Semaphore (trạng thái Blocked không tốn CPU)
        if (xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE) {
            takeCount++;
            float receivedValue = sharedSensorValue;

            // Nháy nhanh LED để biểu thị nhận sự kiện
            digitalWrite(LED_PIN, LOW); // LED ON
            
            Serial.printf("  ==> [Task 2 - Logger] (Prio 1) [xSemaphoreTake] THANH CONG! Nhan gia tri: %.2f Lux\n", receivedValue);

            // Cập nhật LCD
            updateSemaphoreUI(receivedValue, giveCount, takeCount);

            vTaskDelay(pdMS_TO_TICKS(100));
            digitalWrite(LED_PIN, HIGH); // LED OFF
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // Tắt LED ban đầu (Active-LOW)

    // Khởi tạo LCD
    tft.init();
    tft.setRotation(1);
    drawStaticUI();
    updateSemaphoreUI(0.0, 0, 0);

    Serial.println("\n========================================================");
    Serial.println("  BAI 4: DONG BO HOA TASK VOI BINARY SEMAPHORE (FREERTOS)");
    Serial.println("  Hoc vien: Le Duc Phuong");
    Serial.println("  Board   : ESP32-2432S028R (CYD) + LCD ILI9341");
    Serial.println("  Semaphore: Binary Semaphore (Signaling mechanism)");
    Serial.println("  Task 1  : TaskSensor (Priority 2) -> Gives Semaphore");
    Serial.println("  Task 2  : TaskLogger (Priority 1) -> Takes Semaphore");
    Serial.println("========================================================\n");

    // Khởi tạo I2C và BH1750
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, BH1750_I2C_ADDR, &Wire);

    // 1. Tạo Binary Semaphore
    xBinarySemaphore = xSemaphoreCreateBinary();

    if (xBinarySemaphore != NULL) {
        Serial.println("[FREERTOS] Khoi tao Binary Semaphore thanh cong!");

        // 2. Khởi tạo 2 Task FreeRTOS chạy đồng thời trên 2 mức ưu tiên
        xTaskCreate(
            TaskSensor,
            "TaskSensor",
            4096,
            NULL,
            2,          // Priority 2 (Cao hơn)
            NULL
        );

        xTaskCreate(
            TaskLogger,
            "TaskLogger",
            4096,
            NULL,
            1,          // Priority 1 (Thấp hơn)
            NULL
        );
    } else {
        Serial.println("[FREERTOS ERROR] Khong the tao Binary Semaphore!");
    }
}

void loop() {
    // Vòng lặp loop của Arduino để trống, nhường quyền hoàn toàn cho FreeRTOS scheduler
    vTaskDelay(pdMS_TO_TICKS(1000));
}
