//==============================================================================
//  StepperDrive - phát xung STEP cho 2 động cơ bước bằng timer (DDS 32 bit,
//  ISR 40 kHz) + đếm bước (odometry) + chân EN chung.
//==============================================================================
#pragma once
#include <Arduino.h>
#include "Config.h"

// Một kênh động cơ (1 driver A4988 / DRV8825 / TMC2209)
struct StepChannel {
  uint8_t  stepPin, dirPin;
  bool     invert;
  volatile uint32_t inc;      // bước pha mỗi tick (tỉ lệ tần số)
  volatile int8_t   dirReq;   // +1 tiến, -1 lùi
  int8_t   dirCur;
  uint32_t acc;
  bool     pulse;
  volatile int32_t steps;     // đếm bước, dương = tiến

  void init(uint8_t stepP, uint8_t dirP, bool inv);
  void tick();                // gọi trong ISR mỗi 25 us
};

struct StepperDrive {
  StepChannel ch[2];          // 0 = trái, 1 = phải
  volatile uint32_t wd;       // watchdog: số tick ISR kể từ lần setVelocity cuối; quá STEP_WD_TICKS -> ngắt xung

  void  begin();              // cấu hình chân, TẮT driver ngay (gọi đầu setup)
  void  startTimer();         // bật ISR phát xung
  void  enable(bool on);      // bật/tắt driver (chân EN)
  void  setVelocity(int i, float v);   // v: vận tốc bánh (m/s), dương = tiến
  void  stop();               // vận tốc 2 bánh = 0
  float positionMeters() const;        // x trung bình 2 bánh
};

extern StepperDrive stepper;
