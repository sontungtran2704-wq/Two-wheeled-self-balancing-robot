#line 1 "D:\\blancing\\Robot_Collab\\03_Firmware_ESP32S3\\balancebot_lqr_60_test\\ImuHal.h"
#pragma once
#include <Arduino.h>
#include "ControlCore.h"
namespace imu {
void task(void*);
robot::Sensors snapshot();
void requestCalibration();
bool calibrated();
unsigned diagnostic();
unsigned identity();
unsigned calibrationAccelMilliG();
unsigned calibrationAccelStdMg();
unsigned calibrationGyroStdMrad();
bool sampleFresh();
}

