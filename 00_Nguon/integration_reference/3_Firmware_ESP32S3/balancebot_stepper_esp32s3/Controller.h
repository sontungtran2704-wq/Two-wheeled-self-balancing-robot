//==============================================================================
// Controller - PID vi tri vong ngoai + LQR can bang vong trong + failsafe
// PID: x_ref - x_hat -> theta_ref
// LQR: [theta-theta_ref, theta_dot, v_hat] -> F
// Odometry: vi tri uoc luong tu so xung STEP, khong phai encoder phan hoi.
//==============================================================================
#pragma once
#include <Arduino.h>
#include "Config.h"

enum BotState : uint8_t { ST_IDLE = 0, ST_ARMING = 1, ST_RUN = 2, ST_FALLEN = 3, ST_FAULT = 4 };

struct Controller {
  uint8_t  state;
  float    vCmd;                     // van toc dat gui den stepper
  float    vTarget, vTargetF;        // van toc muc tieu (tho / lam muot)
  float    wTarget, wTargetF;        // vi sai 2 banh
  float    xRef, xOffset;
  float    xPrev, vEst;              // van toc odometry uoc luong va loc
  float    posIntegral, thetaRef;    // trang thai PID vong ngoai
  float    lastF;                    // luc dieu khien LQR
  uint32_t armHoldMs, satMs, lastMotionCmdMs, lastCtrlUs, overruns;
  bool     armRequest;
  bool     jogging; float jogL, jogR; uint32_t jogEndMs;   // thử chiều quay bánh khi IDLE

  Controller();
  void poll();
  void step(float dt);
  void setState(uint8_t s);
  void cmdStart();
  void cmdStop();
  void cmdSpeed(float cmPerSec);
  void cmdTurn(float degPerSec);
  bool cmdJog(float lCmPerSec, float rCmPerSec, uint32_t ms);   // chỉ khi IDLE
  float x() const;
};
extern Controller bot;
