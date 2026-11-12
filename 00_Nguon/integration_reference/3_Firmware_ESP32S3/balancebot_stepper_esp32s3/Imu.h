//==============================================================================
//  Imu - cảm biến MPU6050 qua I2C: đọc accel (g) và gyro (rad/s, đã trừ bias)
//==============================================================================
#pragma once
#include <Arduino.h>
#include "Config.h"

struct Imu {
  float   ax, ay, az;          // g
  float   gx, gy, gz;          // rad/s (đã trừ bias)
  float   gyroBias[3];         // rad/s
  uint8_t failCount;           // số lần đọc lỗi liên tiếp (chặn ở 255)

  Imu();
  bool begin();                // Wire.begin + cấu hình MPU6050
  bool update();               // đọc mới, cập nhật ax..gz và failCount
  bool calibrateGyro();        // xe đặt yên; true nếu đủ ổn định
  bool healthy() const { return failCount < IMU_FAIL_LIMIT; }

private:
  bool writeReg(uint8_t reg, uint8_t val);
  bool readRaw(float* a, float* g);   // a: g, g: rad/s (chưa trừ bias)
};

extern Imu imu;
