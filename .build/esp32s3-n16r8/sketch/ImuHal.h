#line 1 "D:\\TLHT\\Two-wheeled-self-balancing-robot\\03_Firmware_ESP32S3\\balancebot\\ImuHal.h"
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

