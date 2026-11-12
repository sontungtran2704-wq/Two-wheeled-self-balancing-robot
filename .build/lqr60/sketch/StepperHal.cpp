#line 1 "D:\\blancing\\Robot_Collab\\03_Firmware_ESP32S3\\balancebot_lqr_60_test\\StepperHal.cpp"
#include "StepperHal.h"
#include "HardwareConfig.h"
#include "GeneratedConfig.h"
#include "driver/gpio.h"
#include "soc/soc.h"
#include "soc/gpio_reg.h"
namespace stepper {
static portMUX_TYPE lock=portMUX_INITIALIZER_UNLOCKED;
static robot::StepNco channels[2];
static uint32_t age=1200;
static bool active=false,watchdog=false;
static hw_timer_t* timer=nullptr;
static inline void IRAM_ATTR level(int pin,bool high) {
  REG_WRITE(high?GPIO_OUT_W1TS_REG:GPIO_OUT_W1TC_REG,1UL<<pin);
}
static void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&lock);
  if(active && ++age>=1200) {
    active=false;watchdog=true;channels[0].disable();channels[1].disable();level(hardware::enable,true);
  }
  for(int i=0;i<2;++i) {
    const auto e=channels[i].tick();
    const int step=i?hardware::stepR:hardware::stepL,dir=i?hardware::dirR:hardware::dirL;
    if(e.fall)level(step,false);
    if(e.direction_changed)level(dir,(channels[i].direction()>0)!=(i?hardware::invertR:hardware::invertL));
    if(e.rise && active)level(step,true);
  }
  portEXIT_CRITICAL_ISR(&lock);
}
bool begin() {
  if(!hardware::pinsConfigured)return true;
  // External EN pull-up is required to keep A4988 disabled before setup executes.
  // Preload the output latch before driving active-low EN.
  const auto enablePin = static_cast<gpio_num_t>(hardware::enable);
  if (gpio_set_level(enablePin, 1) != ESP_OK ||
      gpio_set_direction(enablePin, GPIO_MODE_OUTPUT) != ESP_OK) return false;
  if(!hardware::confirmed)return true;
  for(int p:{hardware::stepL,hardware::stepR,hardware::dirL,hardware::dirR}) {digitalWrite(p,LOW);pinMode(p,OUTPUT);}
  digitalWrite(hardware::dirL,hardware::invertL?LOW:HIGH);digitalWrite(hardware::dirR,hardware::invertR?LOW:HIGH);
  timer=timerBegin(1000000);if(!timer)return false;
  timerAttachInterrupt(timer,onTimer);timerAlarm(timer,25,true,0);return true;
}
void apply(double left,double right,bool enabled) {
  if(!hardware::confirmed)return;
  portENTER_CRITICAL(&lock);
  if(!enabled) {
    active=false;channels[0].disable();channels[1].disable();level(hardware::enable,true);
    // Explicit stop/reset is required before the watchdog latch can clear.
    watchdog=false;
  } else if(!watchdog) {
    channels[0].setRate(left*cfg::steps_per_m);channels[1].setRate(right*cfg::steps_per_m);
    age=0;active=true;level(hardware::enable,false);
  }
  portEXIT_CRITICAL(&lock);
}
void snapshot(int64_t& left,int64_t& right,bool& fault) {
  portENTER_CRITICAL(&lock);left=channels[0].steps();right=channels[1].steps();fault=watchdog;portEXIT_CRITICAL(&lock);
}
}
