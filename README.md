# BÁO CÁO THỰC HÀNH MÔN HỌC INTERNET OF THINGS (IoT)
### CHƯƠNG TRÌNH ĐÀO TẠO THẠC SĨ – KHOA KHOA HỌC VÀ KỸ THUẬT MÁY TÍNH
**TRƯỜNG ĐẠI HỌC BÁCH KHOA – ĐẠI HỌC QUỐC GIA TP. HỒ CHÍ MINH**

---

<p align="center">
  <img src="https://img.shields.io/badge/PlatformIO-Core%206.8-orange?logo=platformio" alt="PlatformIO" />
  <img src="https://img.shields.io/badge/Board-ESP32--2432S028R%20(CYD)-blue?logo=espressif" alt="ESP32 CYD" />
  <img src="https://img.shields.io/badge/Sensor-Sensirion%20SCD30%20(NDIR%20CO2)-green" alt="SCD30" />
  <img src="https://img.shields.io/badge/OS-FreeRTOS%20SMP-blueviolet" alt="FreeRTOS" />
  <img src="https://img.shields.io/badge/Cloud-CoreIOT%20Platform-teal" alt="CoreIOT" />
  <img src="https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B" alt="C++" />
</p>

---

## 👨‍🎓 THÔNG TIN HỌC VIÊN
* **Họ và tên**: **Lê Đức Phương**
* **Mã số học viên (MSHV)**: **2570480**
* **Chương trình đào tạo**: Thạc sĩ Khoa học & Kỹ thuật Máy tính
* **Email**: `leducphuong.hcm@gmail.com`
* **Kho mã nguồn GitHub**: [https://github.com/lovebmt/master-26-iot-personal-lab](https://github.com/lovebmt/master-26-iot-personal-lab)
* **File báo cáo Word hoàn chỉnh**: [`BaoCaoThucHanh_LeDucPhuong.docx`](BaoCaoThucHanh_LeDucPhuong.docx)

---

## 🎬 VIDEO MINH CHỨNG THỰC TẾ (YOUTUBE)

Theo yêu cầu của đề cương thực hành, học viên thực hiện ghi hình minh chứng thực tế trên phần cứng và tải lên YouTube:

| Bài thực hành | Nội dung video minh chứng | Đường dẫn YouTube |
| :--- | :--- | :--- |
| **BÀI 2** | **Điều khiển LED Blinky chu kỳ 1000ms non-blocking**<br>Quay thực tế phần cứng board ESP32 CYD với LED Đỏ onboard (GPIO 4) nhấp nháy đều đặn chu kỳ 1s sáng / 1s tắt kết hợp hiển thị thông số trên LCD ILI9341. | 🔗 [https://youtu.be/Upg9MxHF-D4](https://youtu.be/Upg9MxHF-D4) |
| **BÀI 5** | **Web Server điều khiển LED và giám sát SCD30**<br>Quay song song (Side-by-side): Trình duyệt Google Chrome mở Web UI `192.168.1.4` hiển thị telemetry CO2, nhiệt độ, độ ẩm và camera quay thực tế board phần cứng với đèn LED Đỏ bật/tắt đồng bộ tức thì khi điều khiển qua Web. | 🔗 [https://youtu.be/rHOaKIJMmGk](https://youtu.be/rHOaKIJMmGk) |

---

## 🛠️ CẤU HÌNH PHẦN CỨNG & CẢM BIẾN

Dự án được triển khai trên phần cứng thực tế:

```
                  +-------------------------------------------------+
                  |         ESP32-2432S028R ("Cheap Yellow Display")|
                  |                                                 |
                  |  +-------------------------------------------+  |
                  |  |  Màn hình LCD 2.8" SPI (ILI9341 320x240)   |  |
                  |  |  Hiển thị Dashboard UI, Telemetry, Status |  |
                  |  +-------------------------------------------+  |
                  |                                                 |
                  |  [GPIO 4]  ---> Đèn LED Đỏ onboard (Active-LOW) |
                  |  [GPIO 27] ---> I2C SDA (CN1 Connector)        |
                  |  [GPIO 22] ---> I2C SCL (CN1 Connector)        |
                  |  [3V3 / GND] -> Cấp nguồn cảm biến             |
                  +-------------------------------------------------+
                                      |   |
                                 SDA  |   |  SCL
                                      v   v
                  +-------------------------------------------------+
                  |      Sensirion SCD30 (NDIR Optical Sensor)      |
                  |   • Nồng độ Khí CO2: 400 - 10,000 ppm (±30 ppm) |
                  |   • Nhiệt độ        : -40°C đến +70°C            |
                  |   • Độ ẩm tương đối : 0% đến 100% RH            |
                  |   • Địa chỉ I2C     : 0x61                      |
                  +-------------------------------------------------+
```

1. **Vi điều khiển (MCU)**:
   * **ESP32-D0WD-V3**: Dual-Core Xtensa 32-bit LX6 @ 240MHz, 520 KB SRAM, 4 MB Flash.
   * **Màn hình tích hợp**: 2.8 inch Color TFT LCD ILI9341 (320x240 landscape), giao tiếp qua SPI bus (MOSI: 13, MISO: 12, SCLK: 14, CS: 15, DC: 2, BL: 21).
   * **LED onboard**: GPIO 4 (LED Đỏ mặt sau board, Active-LOW: `LOW` = BẬT, `HIGH` = TẮT).
2. **Cảm biến Môi trường NDIR**:
   * **Sensirion SCD30**: Cảm biến đo trực tiếp nồng độ khí CO2 bằng công nghệ quang học hồng ngoại không phân tán (NDIR), tích hợp cảm biến nhiệt độ và độ ẩm kỹ thuật số Sensirion.
   * Giao diện: I2C (địa chỉ phần cứng `0x61`).
   * Kết nối chân: `SDA -> GPIO 27`, `SCL -> GPIO 22`.

---

## 📑 TỔNG HỢP NỘI DUNG 5 BÀI THỰC HÀNH

### BÀI 1: Tạo tài khoản, thiết bị và Dashboard trên CoreIOT
* **Mục tiêu**: Làm quen với nền tảng CoreIOT Cloud (`https://app.coreiot.io`).
* **Triển khai**:
  * Đăng ký tài khoản Tenant Admin (`Justin Lê` / `leducphuong.hcm@gmail.com`).
  * Tạo thiết bị `ESP32_Lab_Device` (Device ID: `885e62d0-da72-11f1-9298-f795494fee72`).
  * Gửi dữ liệu telemetry qua giao thức MQTT (`app.coreiot.io:1883`) bằng Access Token.
  * Thiết kế Dashboard hoàn chỉnh với đầy đủ các widget: Bảng hiển thị telemetry (CO2, Temp, Hum), Trạng thái thiết bị, Cảnh báo (Alarms) và Bản đồ định vị OpenStreetMap.

### BÀI 2: Lập trình Điều khiển LED Blinky
* **Mục tiêu**: Điều khiển LED onboard nhấp nháy chu kỳ 1000ms sáng / 1000ms tắt không chặn CPU.
* **Mã nguồn**: [`src/bai2_blink.cpp`](src/bai2_blink.cpp)
* **Kỹ thuật**: Sử dụng biến thời gian `millis()` (non-blocking) thay vì `delay()`, cho phép CPU xử lý đồng thời việc vẽ giao diện LCD và in log Serial.
* **Minh chứng**: Video [https://youtu.be/Upg9MxHF-D4](https://youtu.be/Upg9MxHF-D4).

### BÀI 3: Giao tiếp I2C Cảm biến SCD30 và gửi Telemetry lên CoreIOT
* **Mục tiêu**: Đọc dữ liệu môi trường từ cảm biến quang học SCD30 qua bus I2C và đẩy lên Cloud.
* **Mã nguồn**: [`src/bai3_scd30.cpp`](src/bai3_scd30.cpp)
* **Quy trình**:
  * Chạy thuật toán I2C Scanner tự động rà quét bus tại `GPIO 27 (SDA)` và `GPIO 22 (SCL)`, nhận diện chính xác địa chỉ `0x61 (Sensirion SCD30)`.
  * Khởi tạo thư viện `SparkFun SCD30`, đọc chu kỳ 2s/lần: Nồng độ $CO_2$ (ppm), Nhiệt độ (°C), Độ ẩm tương đối (%RH).
  * Hiển thị trực quan dưới dạng thẻ đo thông số (Digital Cards) trên màn hình LCD ILI9341.
  * Đóng gói JSON payload và publish lên MQTT broker CoreIOT topic `v1/devices/me/telemetry`.

### BÀI 4: Lập trình Semaphore đồng bộ Task trên FreeRTOS
* **Mục tiêu**: Đồng bộ hóa dữ liệu giữa các tác vụ độc lập bằng cơ chế Binary Semaphore của FreeRTOS.
* **Mã nguồn**: [`src/bai4_semaphore.cpp`](src/bai4_semaphore.cpp)
* **Kiến trúc luồng**:
  * `TaskSensor` (Priority 2, định kỳ 2000ms): Đọc cảm biến SCD30, cập nhật vào biến toàn cục và phát tín hiệu báo hiệu `xSemaphoreGive(xBinarySemaphore)`.
  * `TaskLogger` (Priority 1): Bị chặn (blocked) chờ cờ hiệu tại `xSemaphoreTake(xBinarySemaphore, portMAX_DELAY)`. Ngay khi nhận được semaphore, task thức dậy in log Serial và làm mới thẻ đồng bộ trên LCD.
* **Lý thuyết**: Phân tích chi tiết sự khác nhau giữa Binary Semaphore (đồng bộ hóa) và Mutex (bảo vệ tài nguyên, có cơ chế Priority Inheritance chống đảo độ ưu tiên).

### BÀI 5: Xây dựng RESTful Web Server Giám sát và Điều khiển
* **Mục tiêu**: Xây dựng máy chủ Web Server trên ESP32 theo kiến trúc RESTful để giám sát telemetry SCD30 và điều khiển LED qua mạng.
* **Mã nguồn**: [`src/bai5_webserver.cpp`](src/bai5_webserver.cpp)
* **Tính năng**:
  * **Wi-Fi Dual Mode**: Chạy song song chế độ Station (kết nối Wi-Fi phòng lab `192.168.1.4`) và chế độ Access Point (`ESP32-Lab-AP` - `192.168.4.1`).
  * **RESTful Endpoints**:
    * `GET /`: Trả về giao diện Web Dark Mode Responsive chuẩn Mobile/Desktop.
    * `GET /api/data`: Trả về payload JSON nhẹ (`co2`, `temp`, `humidity`, `led`, `uptime`).
    * `POST /api/led/toggle`: Đổi trạng thái đèn LED Đỏ GPIO 4 và phản hồi trạng thái mới.
  * **Cơ chế cập nhật**: Sử dụng **AJAX / Fetch API Polling** định kỳ 1.5s, không reload trang, tối ưu bộ nhớ heap của vi điều khiển.
* **Minh chứng**: Video [https://youtu.be/rHOaKIJMmGk](https://youtu.be/rHOaKIJMmGk).

---

## 🗂️ CẤU TRÚC THƯ MỤC DỰ ÁN

```
master-26-iot-personal-lab/
├── BaoCaoThucHanh_LeDucPhuong.docx     # Báo cáo Word hoàn chỉnh (18 ảnh minh chứng + link video)
├── BaoCaoThucHanh_ThacSi (1).docx      # Đề cương thực hành gốc của Giảng viên
├── platformio.ini                      # Cấu hình PlatformIO cho 4 môi trường firmware
├── README.md                           # Tài liệu tổng quan dự án (file này)
├── src/                                # Mã nguồn C++ PlatformIO
│   ├── bai2_blink.cpp                  # Firmware Bài 2: Non-blocking LED Blinky + LCD UI
│   ├── bai3_scd30.cpp                  # Firmware Bài 3: I2C Scanner + SCD30 + MQTT CoreIOT
│   ├── bai4_semaphore.cpp              # Firmware Bài 4: FreeRTOS Binary Semaphore
│   └── bai5_webserver.cpp              # Firmware Bài 5: Dual Wi-Fi RESTful Webserver + Dark UI
├── evidence/                           # 18 hình ảnh minh chứng thực tế
│   ├── bai1/                           # 4 ảnh chụp giao diện CoreIOT (Account, Device, Telemetry, Dashboard)
│   ├── bai2/                           # 4 ảnh minh chứng LED Blinky (Hardware ON/OFF, Serial log, Build)
│   ├── bai3/                           # 5 ảnh I2C SCD30 (Sơ đồ, I2C scan log, Telemetry, LCD, CoreIOT)
│   ├── bai4/                           # 2 ảnh Semaphore (Sơ đồ luồng, Serial sync log)
│   └── bai5/                           # 4 ảnh Web Server (Giao diện desktop, mobile, serial IP, LED ON)
└── videos/                             # Video minh chứng thực tế lưu cục bộ
    ├── Video_Demo_Bai2_LED_Blinky.mp4  # Video Bài 2 (74 MB)
    └── Video_Demo_Bai5_WebServer_IoT.mp4 # Video Bài 5 (79.7 MB)
```

---

## 🚀 HƯỚNG DẪN BIÊN DỊCH VÀ NẠP FIRMWARE

### 1. Chuẩn bị môi trường
* Cài đặt [PlatformIO Core (CLI)](https://platformio.org/install/cli) hoặc tiện ích mở rộng PlatformIO trên VS Code.
* Kết nối bo mạch ESP32 CYD vào máy tính qua cổng micro-USB (Port: `/dev/cu.usbserial-130` trên macOS hoặc `COMx` trên Windows).

### 2. Biên dịch và nạp cho từng bài thực hành

```bash
# Nạp firmware Bài 2 (LED Blinky):
pio run -e bai2_blink -t upload

# Nạp firmware Bài 3 (Cảm biến SCD30 + CoreIOT MQTT):
pio run -e bai3_scd30 -t upload

# Nạp firmware Bài 4 (FreeRTOS Binary Semaphore):
pio run -e bai4_semaphore -t upload

# Nạp firmware Bài 5 (RESTful Web Server):
pio run -e bai5_webserver -t upload
```

### 3. Mở Serial Monitor để theo dõi log
```bash
pio device monitor -b 115200
```

---

## 📜 CAM KẾT HỌC VIÊN
Tôi xin cam kết toàn bộ mã nguồn, cấu hình phần cứng, dữ liệu đo đạc thực tế từ cảm biến Sensirion SCD30, hình ảnh và video minh chứng trong báo cáo này do chính tôi thực hiện trên thiết bị thật.

**Học viên:** Lê Đức Phương (MSHV: 2570480)

