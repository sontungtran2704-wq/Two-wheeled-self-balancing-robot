//==============================================================================
//  Status - telemetry (Serial + TCP) và LED trạng thái
//==============================================================================
#pragma once
#include <Arduino.h>

struct StatusLed {
  int8_t last;

  StatusLed() : last(-1) {}
  void begin();                    // cấu hình chân LED thường (nếu có)
  void update(uint8_t state);      // nháy theo trạng thái xe
};

extern StatusLed statusLed;

void sendTelemetry();              // t, theta, theta_dot, x, x_dot, u, st
