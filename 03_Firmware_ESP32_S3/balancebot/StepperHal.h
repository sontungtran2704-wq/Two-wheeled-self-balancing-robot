// HAL nhận vận tốc hai bánh theo m/s và cờ bật motor, trả bộ đếm xung có dấu cùng cờ watchdog.
#pragma once
#include <Arduino.h>
#include "StepNco.h"
namespace stepper {
bool begin();
void apply(double left_m_s,double right_m_s,bool enabled);
void snapshot(int64_t& left,int64_t& right,bool& watchdog_fault);
}
