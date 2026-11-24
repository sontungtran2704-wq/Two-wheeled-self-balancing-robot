// Gom sơ đồ chân và quy ước lắp đặt mà HAL IMU, HAL động cơ và tác vụ Wi-Fi dùng.
#pragma once
// User identified: ESP32-S3 44-pin, MPU6050, A4988, 12 V motor supply,
// regulated 5 V board supply, no encoder. Wiring confirmed by user 2026-11-11.
// Module flash/PSRAM suffix, motor direction and IMU orientation still unverified.
#ifndef ROBOT_REFERENCE_PINMAP
#define ROBOT_REFERENCE_PINMAP 1
#endif
#ifndef ROBOT_HARDWARE_CONFIRMED
#define ROBOT_HARDWARE_CONFIRMED 1
#endif
#ifndef ROBOT_WIFI_SSID
#define ROBOT_WIFI_SSID ""
#endif
#ifndef ROBOT_WIFI_PASSWORD
#define ROBOT_WIFI_PASSWORD ""
#endif
namespace hardware {
constexpr bool confirmed=ROBOT_HARDWARE_CONFIRMED!=0;
constexpr bool pinsConfigured=ROBOT_REFERENCE_PINMAP!=0;
// A4988 VDD/MS1/MS2/MS3 and SLP/RST: ESP32 3V3; microstep 1/16.
// GPIO 4/5 là STEP/DIR trái, GPIO 6/7 là STEP/DIR phải. STEP đếm cạnh lên, DIR chọn chiều.
constexpr int stepL=ROBOT_REFERENCE_PINMAP?4:-1,dirL=ROBOT_REFERENCE_PINMAP?5:-1;
constexpr int stepR=ROBOT_REFERENCE_PINMAP?6:-1,dirR=ROBOT_REFERENCE_PINMAP?7:-1;
// GPIO 15 điều khiển EN chung của A4988, mức thấp cho phép chạy. GPIO 8/9 là SDA/SCL của IMU.
constexpr int enable=ROBOT_REFERENCE_PINMAP?15:-1,sda=ROBOT_REFERENCE_PINMAP?8:-1,scl=ROBOT_REFERENCE_PINMAP?9:-1;
// Đảo mức DIR bên phải để ánh xạ dấu tốc độ logic sang cách lắp hai motor.
constexpr bool invertL=false,invertR=true;
// Trim góc lắp IMU tính bằng rad, được trừ khỏi góc tính từ gia tốc. Zero điều khiển được lưu riêng.
constexpr double pitchTrim=0.;
constexpr const char* ssid=ROBOT_WIFI_SSID;
constexpr const char* password=ROBOT_WIFI_PASSWORD;
constexpr uint8_t imuAddress=0x68;
static_assert(!confirmed || (stepL>=0 && stepR>=0 && enable>=0 && sda>=0 && scl>=0),"Confirm physical pin map before enabling hardware");
static_assert(!confirmed || (stepL<32 && stepR<32 && dirL<32 && dirR<32 && enable<32),"ISR GPIO register bank requires pins below32");
}
