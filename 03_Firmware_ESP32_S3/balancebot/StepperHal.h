#pragma once
#include <Arduino.h>
#include "StepNco.h"
namespace stepper {
bool begin();
void apply(double left_m_s,double right_m_s,bool enabled);
void snapshot(int64_t& left,int64_t& right,bool& watchdog_fault);
}
