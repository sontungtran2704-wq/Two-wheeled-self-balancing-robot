// Giao diện giữa tác vụ đọc IMU và vòng cân bằng. snapshot trả về bản sao mẫu cùng cờ hợp lệ.
#pragma once
#include <Arduino.h>
#include "ControlCore.h"
namespace imu {
void task(void*);
robot::Sensors snapshot();
// Yêu cầu đo và lưu bias gyro ở tác vụ IMU, đồng thời vô hiệu mẫu cũ cho điều khiển.
void requestCalibration();
bool calibrated();
unsigned diagnostic();
unsigned identity();
unsigned calibrationAccelMilliG();
unsigned calibrationAccelStdMg();
unsigned calibrationGyroStdMrad();
bool sampleFresh();
bool calibrationSaved();
bool calibrationRestored();
bool calibrating();
}

