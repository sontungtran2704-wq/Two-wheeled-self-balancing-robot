#line 1 "D:\\TLHT\\Two-wheeled-self-balancing-robot\\03_Firmware_ESP32S3\\balancebot\\StepNco.h"
#pragma once
#include <cstdint>
#include <cmath>
#ifndef IRAM_ATTR
#define ROBOT_IRAM
#else
#define ROBOT_IRAM IRAM_ATTR
#endif
namespace robot {
struct StepEvent {bool rise=false,fall=false,direction_changed=false;};
class StepNco {
 public:
  void setRate(double rate) {
    if(!std::isfinite(rate))rate=0;
    if(rate>15000)rate=15000;if(rate< -15000)rate=-15000;
    requested_=rate<0?-1:1;
    increment_=uint32_t(std::abs(rate)*(4294967296.0/40000.));
  }
  ROBOT_IRAM StepEvent tick() {
    StepEvent e;
    if(high_){high_=false;e.fall=true;}
    if(requested_!=direction_) {direction_=requested_;phase_=0;e.direction_changed=true;return e;}
    if(!increment_)return e;
    const uint32_t old=phase_;phase_+=increment_;
    if(phase_<old){e.rise=true;high_=true;steps_+=direction_;}
    return e;
  }
  ROBOT_IRAM void disable() {increment_=0;phase_=0;}
  int direction() const {return direction_;}
  int64_t steps() const {return steps_;}
 private:
  uint32_t phase_=0,increment_=0;
  int direction_=1,requested_=1;bool high_=false;int64_t steps_=0;
};
}
