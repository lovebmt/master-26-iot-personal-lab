/**
 * BÀI 2. LED BLINKY TRÊN ESP32 CYD (ESP32-2432S028R)
 * Môn học: Internet of Things - Chương trình Thạc sĩ
 * Học viên: Lê Đức Phương
 *
 * Chân LED: GPIO 4 (LED Đỏ onboard, Active-LOW)
 * Chu kỳ: 1000 ms sáng / 1000 ms tắt (Tổng chu kỳ: 2000 ms)
 * Phương pháp: Non-blocking millis()
 * Mở rộng: Hiển thị trạng thái LED và chu kỳ trực tiếp lên màn hình LCD ILI9341
 */

#include <Arduino.h>
#include <TFT_eSPI.h>

#define LED_PIN 4               // GPIO 4: LED Đỏ onboard (Active-LOW: 0=BẬT, 1=TẮT)
#define BLINK_INTERVAL_MS 1000  // Chu kỳ đổi trạng thái: 1000ms

TFT_eSPI tft = TFT_eSPI();

bool ledState = false;
unsigned long previousMillis = 0;
unsigned long toggleCount = 0;

void drawStaticUI() {
    tft.fillScreen(TFT_BLACK);
    
    // Header Bar
    tft.fillRect(0, 0, 320, 34, 0x0011); // Dark Navy Blue
    tft.setTextColor(TFT_WHITE, 0x0011);
    tft.drawString("BAI 2: LED BLINKY CONTROL", 12, 8, 4);

    // Sub header info
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("Hoc vien: Le Duc Phuong", 12, 42, 2);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("Hardware: ESP32 CYD - GPIO 4 (Red LED)", 12, 60, 2);
    tft.drawString("Cycle   : 1000ms ON / 1000ms OFF (millis)", 12, 78, 2);

    // Divider line
    tft.drawFastHLine(10, 100, 300, TFT_DARKGREY);

    // Label for status card
    tft.setTextColor(TFT_GOLD, TFT_BLACK);
    tft.drawString("TRANG THAI LED:", 12, 112, 2);
}

void updateDisplay(bool state, unsigned long count, unsigned long currentSec) {
    // Card background
    uint16_t cardColor = state ? 0xB800 : 0x18E3; // Dark Red vs Dark Slate
    tft.fillRoundRect(12, 134, 296, 60, 8, cardColor);
    tft.drawRoundRect(12, 134, 296, 60, 8, state ? TFT_RED : TFT_DARKGREY);

    tft.setTextColor(state ? TFT_WHITE : TFT_SILVER, cardColor);
    if (state) {
        tft.drawString("  LED: DANG SANG (ON)  ", 30, 148, 4);
    } else {
        tft.drawString("  LED: DANG TAT  (OFF) ", 30, 148, 4);
    }

    // Bottom statistics bar
    char buf[64];
    sprintf(buf, "So lan dao: %lu | Thoi gian: %02lu:%02lu", count, (currentSec / 60) % 60, currentSec % 60);
    tft.fillRect(12, 206, 296, 24, TFT_BLACK);
    tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
    tft.drawString(buf, 12, 208, 2);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // 1. Khởi tạo GPIO LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // Mức HIGH: LED tắt ban đầu (Active-LOW)

    // 2. Khởi tạo Màn hình LCD
    tft.init();
    tft.setRotation(1); // 320x240 Landscape
    drawStaticUI();
    updateDisplay(ledState, toggleCount, 0);

    // 3. In log Serial khởi động
    Serial.println("\n========================================================");
    Serial.println("  BAI 2: DIEU KHIEN LED BLINKY TREN ESP32 CYD");
    Serial.println("  Hoc vien: Le Duc Phuong");
    Serial.println("  Board   : ESP32-2432S028R (CYD) + LCD ILI9341");
    Serial.println("  Hardware: GPIO 4 (Onboard Red LED - Active LOW)");
    Serial.println("  Interval: 1000 ms (Non-blocking millis)");
    Serial.println("========================================================\n");
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= BLINK_INTERVAL_MS) {
        previousMillis = currentMillis;
        ledState = !ledState;
        toggleCount++;

        // Điều khiển chân GPIO (Active-LOW: ledState=true -> LOW để sáng)
        digitalWrite(LED_PIN, ledState ? LOW : HIGH);

        // Cập nhật LCD
        unsigned long sec = currentMillis / 1000;
        updateDisplay(ledState, toggleCount, sec);

        // In log Serial Monitor
        Serial.printf("[%02lu:%02lu] [LED #%lu] Trang thai: %s (GPIO 4: %s)\n",
                      (sec / 60) % 60,
                      sec % 60,
                      toggleCount,
                      ledState ? "BAT (ON) " : "TAT (OFF)",
                      ledState ? "LOW " : "HIGH");
    }
}
