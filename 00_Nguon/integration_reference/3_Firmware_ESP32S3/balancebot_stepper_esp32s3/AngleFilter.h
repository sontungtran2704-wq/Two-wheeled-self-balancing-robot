//==============================================================================
//  AngleFilter - lọc bù (gyro + accel) cho góc nghiêng θ và tốc độ θ̇
//  Quy ước: θ > 0 = thân nghiêng ra TRƯỚC.
//==============================================================================
#pragma once
#include "Imu.h"

struct AngleFilter {
  float theta;       // rad (thô, chưa trừ zeropoint)
  float thetaDot;    // rad/s

  AngleFilter() : theta(0), thetaDot(0) {}
  void initFromAccel(Imu& sensor);               // khởi tạo θ từ accel (xe đứng yên)
  void update(const Imu& sensor, float dt);      // 1 bước lọc
};

extern AngleFilter filt;
