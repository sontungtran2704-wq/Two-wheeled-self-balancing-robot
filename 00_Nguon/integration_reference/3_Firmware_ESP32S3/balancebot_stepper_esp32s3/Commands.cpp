#include "Commands.h"
#include <Arduino.h>
#include "AngleFilter.h"
#include "Config.h"
#include "Controller.h"
#include "Imu.h"
#include "LineReader.h"
#include "Log.h"
#include "Net.h"
#include "Params.h"

static LineReader serialRx;

void pollSerialCommands() {
  while (Serial.available()) serialRx.feed((char)Serial.read(), handleCommand);
}

static bool busy() {
  if (bot.state == ST_RUN) { outln("ERR:busy"); return true; }
  return false;
}

static void cmdSetK(const char* args) {
  float v[6]; char extra;
  int n = sscanf(args, "%f,%f,%f,%f,%f,%f%c", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &extra);
  bool ok = (n == 6);
  for (int i = 0; ok && i < 3; i++) if (!isfinite(v[i]) || fabsf(v[i]) > K_LIMIT) ok = false;
  if (ok && (!isfinite(v[3]) || fabsf(v[3]) > 500.0f)) ok = false;
  if (ok && (!isfinite(v[4]) || fabsf(v[4]) < 1e-3f || fabsf(v[4]) > 50.0f)) ok = false;
  if (ok && (!isfinite(v[5]) || fabsf(v[5]) > 500.0f)) ok = false;
  // Dau vat ly cua mo hinh: A41 < 0, B4 > 0, FrefGain = -a/b > 0. Chan lenh $K cu (4 gain LQR) bi hieu sai thu tu.
  if (ok && !(v[3] < 0.0f && v[4] > 0.0f && v[5] > 0.0f)) ok = false;
  bool live = (bot.state == ST_RUN || bot.state == ST_ARMING);
  if (ok && live) {
    for (int i = 0; i < 3; i++) if (params.K[i] * v[i] < 0.0f) ok = false;
    if (params.A41 * v[3] < 0.0f || params.B4 * v[4] < 0.0f || params.FrefGain * v[5] < 0.0f) ok = false;
    if (!ok) { outln("ERR:K sign flip while armed (STOP truoc neu muon doi dau)"); return; }
  }
  if (ok) {
    for (int i = 0; i < 3; i++) params.K[i] = v[i];
    params.A41 = v[3]; params.B4 = v[4]; params.FrefGain = v[5];
    outln(live ? "K3 UPDATED (LIVE, chua SAVE)" : "K3 UPDATED");
    params.print();
  } else outln("ERR:bad $K (can 6 so: k_theta,k_thetaDot,k_velocity,A41<0,B4>0,FrefGain>0)");
}

static void cmdSetPid(const char* args) {
  float v[3]; char extra;
  int n = sscanf(args, "%f,%f,%f%c", &v[0], &v[1], &v[2], &extra);
  bool ok = (n == 3);
  for (int i = 0; ok && i < 3; i++) if (!isfinite(v[i]) || v[i] < 0.0f) ok = false;
  if (ok && (v[0] > POS_KP_MAX || v[1] > POS_KI_MAX || v[2] > POS_KD_MAX)) ok = false;
  if (!ok) { outln("ERR:bad $P (can 3 so >= 0: Kp<=3, Ki<=1, Kd<=2)"); return; }
  params.posKp = v[0]; params.posKi = v[1]; params.posKd = v[2];
  bot.posIntegral = 0.0f;
  outln(bot.state == ST_RUN ? "PID UPDATED (LIVE, chua SAVE)" : "PID UPDATED");
  params.print();
}

static void cmdJog(const char* args) {
  float l, r; unsigned ms; char extra;
  int n = sscanf(args, "%f,%f,%u%c", &l, &r, &ms, &extra);
  if (n != 3 || !isfinite(l) || !isfinite(r)) { outln("ERR:bad JOG (JOG,<L cm/s>,<R cm/s>,<ms>)"); return; }
  if (!bot.cmdJog(l, r, ms)) outln("ERR:JOG chi khi IDLE");
  else outln("JOG ON (STOP de dung)");
}

static void cmdWifi(const char* args) {
  if (busy()) return;
  const char* comma = strchr(args, ',');
  size_t ls = comma ? (size_t)(comma - args) : strlen(args);
  const char* pw = comma ? comma + 1 : "";
  if (net.setCreds(args, ls, pw)) outln("WIFI SAVED, gui REBOOT de ap dung");
  else outln("ERR:bad WIFI (ssid 1..32, pass rong hoac 8..64 ky tu)");
}

void handleCommand(char* s) {
  if (s[0] == '\0') return;

  if (strcmp(s, "START") == 0) {
    bot.cmdStart();
  } else if (strcmp(s, "STOP") == 0) {
    bot.cmdStop();
  } else if (strncmp(s, "SPEED,", 6) == 0) {
    bot.cmdSpeed(atof(s + 6));
  } else if (strncmp(s, "TURN,", 5) == 0) {
    bot.cmdTurn(atof(s + 5));
  } else if (strncmp(s, "ZP,", 3) == 0) {
    float d = atof(s + 3);
    if (fabsf(d) <= 15.0f) { params.zeropoint = d * DEG2RAD; params.print(); }
  } else if (strcmp(s, "ZP+") == 0) {
    params.zeropoint += 0.1f * DEG2RAD; params.print();
  } else if (strcmp(s, "ZP-") == 0) {
    params.zeropoint -= 0.1f * DEG2RAD; params.print();
  } else if (strcmp(s, "ZPHERE") == 0) {
    if (bot.state != ST_RUN && fabsf(filt.thetaDot) < 0.2f) { params.zeropoint = filt.theta; params.print(); }
    else outln("ERR:ZPHERE needs not-RUN and still");
  } else if (strcmp(s, "CALGYRO") == 0) {
    if (busy()) return;
    outln(imu.calibrateGyro() ? "GYRO OK" : "GYRO FAIL (xe dang rung/di chuyen)");
  } else if (strcmp(s, "SAVE") == 0) {
    if (busy()) return;                                    // ghi flash làm trễ vòng điều khiển -> không khi đang RUN
    params.save(); outln("SAVED");
  } else if (strncmp(s, "WIFI,", 5) == 0) {
    cmdWifi(s + 5);
  } else if (strcmp(s, "REBOOT") == 0) {
    if (busy()) return;
    outln("REBOOTING");
    delay(200);
    ESP.restart();
  } else if (strcmp(s, "DEFAULTS") == 0) {
    if (busy()) return;
    Params::clearStored();
    if (net.fromNvs) net.saveCreds();                      // giữ WiFi đã lưu, chỉ xoá K và zeropoint
    params.setDefaults();
    params.print();
  } else if (strcmp(s, "GET") == 0) {
    params.print();
    net.print();
    outf("overruns=%lu uptime=%lus reset=%d\n", (unsigned long)bot.overruns, (unsigned long)(millis() / 1000UL), bbResetReason());
  } else if (strncmp(s, "$K,", 3) == 0) {
    cmdSetK(s + 3);
  } else if (strncmp(s, "$P,", 3) == 0) {
    cmdSetPid(s + 3);
  } else if (strncmp(s, "JOG,", 4) == 0) {
    cmdJog(s + 4);
  } else {
    outln("ERR:unknown");
  }
}
