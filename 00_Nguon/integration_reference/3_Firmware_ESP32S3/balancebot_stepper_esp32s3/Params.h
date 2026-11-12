//==============================================================================
//  Params - tham số LQR (K, A41, B4) + zeropoint, lưu/đọc NVS (namespace "bb")
//==============================================================================
#pragma once
#include <Arduino.h>
#include "Config.h"

struct Params {
  float K[3];          // k1..k3: [θ-θ_ref, θ̇, v_x]
  float A41, B4, FrefGain; // ẍ=A41·θ+B4·F; Fff=FrefGain·theta_ref
  float posKp, posKi, posKd;  // PID vị trí vòng ngoài (chỉnh bằng $P, lưu NVS)
  float zeropoint;     // rad

  Params();
  void setDefaults();              // chỉ trong RAM, zeropoint = 0
  void load();                     // đọc NVS, thiếu thì dùng mặc định
  void save() const;               // ghi NVS
  static void clearStored();       // xoá NVS (cả WiFi - Net phải ghi lại nếu cần giữ)
  void print() const;
};

extern Params params;
