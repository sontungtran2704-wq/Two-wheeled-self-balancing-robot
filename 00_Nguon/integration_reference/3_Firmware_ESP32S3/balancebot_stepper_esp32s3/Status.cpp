#include "Status.h"
#include "AngleFilter.h"
#include "Config.h"
#include "Controller.h"
#include "Log.h"
#include "Params.h"

StatusLed statusLed;

void StatusLed::begin() {
  if (PIN_LED >= 0) pinMode(PIN_LED, OUTPUT);
}

void StatusLed::update(uint8_t state) {
  uint32_t ms = millis();
  bool on = false;
  switch (state) {
    case ST_IDLE:   on = false; break;
    case ST_ARMING: on = (ms / 250) & 1; break;
    case ST_RUN:    on = true; break;
    case ST_FALLEN: on = (ms / 1000) & 1; break;
    default:        on = (ms / 100) & 1; break;
  }
  if ((int8_t)on == last) return;                           // chỉ ghi khi đổi trạng thái (LED RGB ghi chậm ~1 ms)
  last = (int8_t)on;
  if (PIN_RGB >= 0) neopixelWrite(PIN_RGB, 0, on ? 24 : 0, 0);   // xanh lá (LED RGB onboard ESP32-S3)
  if (PIN_LED >= 0) digitalWrite(PIN_LED, on);
}

void sendTelemetry() {
  outf("t=%.3f,theta=%.5f,theta_dot=%.4f,theta_ref=%.5f,x=%.4f,x_dot=%.4f,u=%.2f,st=%u\n",
       millis() / 1000.0f, filt.theta - params.zeropoint, filt.thetaDot, bot.thetaRef,
       bot.x(), bot.vEst, bot.lastF, (unsigned)bot.state);
}
