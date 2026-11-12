#include "Controller.h"
#include "AngleFilter.h"
#include "Imu.h"
#include "Log.h"
#include "Params.h"
#include "StepperDrive.h"
#include "Util.h"

Controller bot;

Controller::Controller()
  : state(ST_IDLE), vCmd(0), vTarget(0), vTargetF(0), wTarget(0), wTargetF(0),
    xRef(0), xOffset(0), xPrev(0), vEst(0), posIntegral(0), thetaRef(0), lastF(0),
    armHoldMs(0), satMs(0), lastMotionCmdMs(0), lastCtrlUs(0), overruns(0), armRequest(false),
    jogging(false), jogL(0), jogR(0), jogEndMs(0) {}

float Controller::x() const { return stepper.positionMeters() - xOffset; }

void Controller::setState(uint8_t s) {
  state = s;
  jogging = false;
  if (s != ST_RUN) {
    vCmd = 0; vTargetF = 0; wTargetF = 0;
    stepper.stop();
    stepper.enable(false);
  }
  if (s != ST_RUN) { posIntegral = 0; thetaRef = 0; vEst = 0; }
  armHoldMs = 0; satMs = 0;
  static const char* names[] = {"IDLE", "ARMING", "RUN", "FALLEN", "FAULT"};
  outf("EVT:%s\n", names[s]);
}

void Controller::cmdStart() {
  if (state == ST_IDLE || state == ST_FALLEN || state == ST_FAULT) {
    if (!imu.healthy()) { outln("ERR:IMU"); return; }
    armRequest = false;
    setState(ST_ARMING);
  }
}
void Controller::cmdStop() { setState(ST_IDLE); }
void Controller::cmdSpeed(float cmPerSec) {
  vTarget = clampf(cmPerSec / 100.0f, -V_TARGET_MAX, V_TARGET_MAX);
  lastMotionCmdMs = millis();
}
void Controller::cmdTurn(float degPerSec) {
  float wr = degPerSec * DEG2RAD;
  wTarget = clampf(wr * TRACK_M * 0.5f, -W_MAX, W_MAX);
  lastMotionCmdMs = millis();
}

bool Controller::cmdJog(float lCmPerSec, float rCmPerSec, uint32_t ms) {
  if (state != ST_IDLE) return false;
  jogL = clampf(lCmPerSec / 100.0f, -JOG_V_MAX, JOG_V_MAX);
  jogR = clampf(rCmPerSec / 100.0f, -JOG_V_MAX, JOG_V_MAX);
  if (ms < 100) ms = 100;
  if (ms > JOG_MS_MAX) ms = JOG_MS_MAX;
  jogEndMs = millis() + ms;
  jogging = true;
  stepper.enable(true);
  return true;
}

void Controller::poll() {
  uint32_t now = micros();
  if ((uint32_t)(now - lastCtrlUs) < CTRL_US) return;
  float dt = (float)(uint32_t)(now - lastCtrlUs) * 1e-6f;
  if (dt > 4.0f * CTRL_US * 1e-6f) {
    if (state == ST_RUN && dt > STALL_FAULT_MS * 1e-3f) {        // vòng điều khiển bị treo quá lâu khi đang chạy
      lastCtrlUs = now; outln("EVT:STALL"); setState(ST_FAULT); return;
    }
    lastCtrlUs = now; overruns++; dt = CTRL_US * 1e-6f;
  }
  else lastCtrlUs += CTRL_US;
  dt = clampf(dt, 0.5f * CTRL_US * 1e-6f, 2.0f * CTRL_US * 1e-6f);
  step(dt);
}

void Controller::step(float dt) {
  // ---- IMU va goc nghieng ----
  imu.update();
  if (imu.failCount == 0) filt.update(imu, dt);
  if (!imu.healthy() && state != ST_FAULT) { outln("EVT:IMU_FAIL"); setState(ST_FAULT); return; }
  if (!isfinite(filt.theta) || !isfinite(filt.thetaDot)) {
    outln("EVT:NAN"); filt.theta = 0; setState(ST_FAULT); return;
  }
  const float th = filt.theta - params.zeropoint;
  const uint32_t dtMs = (uint32_t)(dt * 1000.0f + 0.5f);

  switch (state) {
    case ST_IDLE:
      if (jogging) {
        if ((int32_t)(millis() - jogEndMs) >= 0) { jogging = false; stepper.stop(); stepper.enable(false); outln("EVT:JOG_END"); }
        else { stepper.setVelocity(0, jogL); stepper.setVelocity(1, jogR); }
      }
      return;
    case ST_FAULT: return;
    case ST_FALLEN:
#if AUTO_REARM
      if (fabsf(th) < ARM_DEG * DEG2RAD) armRequest = true;
#endif
      if (armRequest) { armRequest = false; setState(ST_ARMING); }
      return;
    case ST_ARMING:
      if (fabsf(th) < ARM_DEG * DEG2RAD && fabsf(filt.thetaDot) < 0.5f) armHoldMs += dtMs;
      else armHoldMs = 0;
      if (armHoldMs >= ARM_HOLD_MS) {
        xOffset = stepper.positionMeters(); xRef = 0; xPrev = 0;
        vCmd = vEst = posIntegral = thetaRef = 0;
        vTarget = vTargetF = wTarget = wTargetF = 0;
        setState(ST_RUN);
        stepper.enable(true);
      }
      return;
    case ST_RUN: break;
  }

  if (fabsf(th) > FALL_DEG * DEG2RAD) { setState(ST_FALLEN); return; }

  // ---- Odometry tu so xung STEP ----
  // Day la van toc banh uoc luong tu xung da phat, khong phat hien duoc mat buoc/truot banh.
  const float xNow = x();
  const float vRaw = (xNow - xPrev) / dt;
  xPrev = xNow;
  const float alphaV = dt / (V_EST_TAU_S + dt);
  vEst += alphaV * (vRaw - vEst);

  if ((vTarget != 0.0f || wTarget != 0.0f) &&
      (uint32_t)(millis() - lastMotionCmdMs) > CMD_TIMEOUT_MS) {
    vTarget = 0.0f; wTarget = 0.0f;
    outln("EVT:CMD_TIMEOUT");
  }

  // Lam muot lenh van toc/quay. SPEED duoc tich phan thanh xRef.
  const float dv = A_MAX * 0.5f * dt;
  vTargetF += clampf(vTarget - vTargetF, -dv, dv);
  wTargetF += clampf(wTarget - wTargetF, -dv, dv);
  xRef += vTargetF * dt;
  xRef = clampf(xRef, xNow - X_REF_MAX, xNow + X_REF_MAX);

  // ---- PID vong ngoai: vi tri -> goc nghieng tham chieu ----
  const float posError = xRef - xNow;
  const float iCandidate = clampf(posIntegral + posError * dt, -POS_I_LIMIT, POS_I_LIMIT);
  float thetaUnsat = params.posKp * posError + params.posKi * iCandidate - params.posKd * vEst;
  thetaRef = clampf(thetaUnsat, -THETA_REF_MAX, THETA_REF_MAX);
  // Anti-windup: chi tich phan neu chua bao hoa hoac loi dang keo ra khoi bao hoa.
  if (fabsf(thetaUnsat - thetaRef) < 1e-6f ||
      (thetaUnsat > THETA_REF_MAX && posError < 0) ||
      (thetaUnsat < -THETA_REF_MAX && posError > 0)) posIntegral = iCandidate;

  // ---- LQR vong trong: [theta-thetaRef, thetaDot, v_x] -> luc F ----
  const float eTheta = th - thetaRef;
  const float Fff = params.FrefGain * thetaRef;
  const float F = Fff - (params.K[0] * eTheta + params.K[1] * filt.thetaDot + params.K[2] * vEst);
  lastF = F;

  // Mo hinh tuyen tinh: x_dd = A41*theta + B4*F.
  // Tich phan gia toc thanh van toc dat cho bo phat xung STEP/DIR.
  float acc = params.A41 * th + params.B4 * F;
  acc = clampf(acc, -A_MAX, A_MAX);
  vCmd += acc * dt;
  bool sat = false;
  if (vCmd > V_MAX) { vCmd = V_MAX; sat = true; }
  if (vCmd < -V_MAX) { vCmd = -V_MAX; sat = true; }
  if (sat) satMs += dtMs; else satMs = 0;
  if (satMs > SAT_FAULT_MS) { outln("EVT:RUNAWAY"); setState(ST_FAULT); return; }

  const float w = clampf(wTargetF, -W_MAX, W_MAX);
  stepper.setVelocity(0, clampf(vCmd - w, -V_MAX, V_MAX));
  stepper.setVelocity(1, clampf(vCmd + w, -V_MAX, V_MAX));
}
