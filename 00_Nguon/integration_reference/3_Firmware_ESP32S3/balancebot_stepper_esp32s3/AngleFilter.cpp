#include "AngleFilter.h"
#include "Config.h"

AngleFilter filt;

void AngleFilter::update(const Imu& s, float dt) {
  float thdMeas = SIGN_THETA * GY_PITCH(s.gx, s.gy, s.gz);
  float thAcc   = SIGN_THETA * atan2f(-AX_FWD(s.ax, s.ay, s.az), AZ_UP(s.ax, s.ay, s.az));
  float norm    = sqrtf(s.ax * s.ax + s.ay * s.ay + s.az * s.az);
  float pred    = theta + thdMeas * dt;
  if (fabsf(norm - 1.0f) < 0.25f) {              // chỉ tin accel khi gần 1 g
    float alpha = FILTER_TAU_S / (FILTER_TAU_S + dt);
    theta = alpha * pred + (1.0f - alpha) * thAcc;
  } else {
    theta = pred;
  }
  thetaDot = thdMeas;
}

void AngleFilter::initFromAccel(Imu& s) {
  double sum = 0; int n = 0;
  for (int i = 0; i < 100; i++) {
    if (s.update()) {
      sum += SIGN_THETA * atan2f(-AX_FWD(s.ax, s.ay, s.az), AZ_UP(s.ax, s.ay, s.az));
      n++;
    }
    delay(2);
  }
  theta = n ? (float)(sum / n) : 0.0f;
}
