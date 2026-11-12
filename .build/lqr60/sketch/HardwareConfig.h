#line 1 "D:\\blancing\\Robot_Collab\\03_Firmware_ESP32S3\\balancebot_lqr_60_test\\HardwareConfig.h"
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
constexpr int stepL=ROBOT_REFERENCE_PINMAP?4:-1,dirL=ROBOT_REFERENCE_PINMAP?5:-1;
constexpr int stepR=ROBOT_REFERENCE_PINMAP?6:-1,dirR=ROBOT_REFERENCE_PINMAP?7:-1;
constexpr int enable=ROBOT_REFERENCE_PINMAP?15:-1,sda=ROBOT_REFERENCE_PINMAP?8:-1,scl=ROBOT_REFERENCE_PINMAP?9:-1;
constexpr bool invertL=false,invertR=true;
constexpr double pitchTrim=0.;
constexpr const char* ssid=ROBOT_WIFI_SSID;
constexpr const char* password=ROBOT_WIFI_PASSWORD;
constexpr uint8_t imuAddress=0x68;
static_assert(!confirmed || (stepL>=0 && stepR>=0 && enable>=0 && sda>=0 && scl>=0),"Confirm physical pin map before enabling hardware");
static_assert(!confirmed || (stepL<32 && stepR<32 && dirL<32 && dirR<32 && enable<32),"ISR GPIO register bank requires pins below32");
}
