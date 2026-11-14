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
bool calibrationSaved();
bool calibrationRestored();
bool calibrating();
}

