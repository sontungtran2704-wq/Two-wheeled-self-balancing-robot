#include "StepperDrive.h"
#include "Util.h"
#include "soc/soc.h"
#include "soc/gpio_reg.h"   // ghi thẳng thanh ghi set/clear GPIO (ESP32 và ESP32-S3, chân < 32)

StepperDrive stepper;

//------------------------------------------------------------------------------
// StepChannel
//------------------------------------------------------------------------------
void StepChannel::init(uint8_t stepP, uint8_t dirP, bool inv) {
  stepPin = stepP; dirPin = dirP; invert = inv;
  inc = 0; dirReq = 1; dirCur = 1; acc = 0; pulse = false; steps = 0;
}

void IRAM_ATTR StepChannel::tick() {
  if (pulse) {                                     // kết thúc xung (độ rộng = 1 tick = 25 us)
    REG_WRITE(GPIO_OUT_W1TC_REG, (1UL << stepPin));
    pulse = false;                                 // KHÔNG return: vẫn phải cộng pha ở tick này,
  }                                                // nếu không tần số thực bị thấp hơn lệnh
  uint32_t incNow = inc;
  if (incNow == 0) return;
  int8_t dr = dirReq;
  if (dr != dirCur) {                              // đổi chiều: đặt DIR trước, STEP ở tick sau
    bool level = ((dr > 0) != invert);
    if (level) REG_WRITE(GPIO_OUT_W1TS_REG, (1UL << dirPin));
    else       REG_WRITE(GPIO_OUT_W1TC_REG, (1UL << dirPin));
    dirCur = dr;
    return;
  }
  uint32_t prev = acc;
  acc = prev + incNow;
  if (acc < prev) {                                // tràn số = 1 bước
    REG_WRITE(GPIO_OUT_W1TS_REG, (1UL << stepPin));
    pulse = true;
    steps += dr;
  }
}

//------------------------------------------------------------------------------
// Timer ISR
//------------------------------------------------------------------------------
static hw_timer_t* stepTimer = nullptr;

static void IRAM_ATTR onStepTimer() {
  if (stepper.wd < STEP_WD_TICKS) stepper.wd++;
  else { stepper.ch[0].inc = 0; stepper.ch[1].inc = 0; }   // vòng điều khiển treo -> dừng phát xung
  stepper.ch[0].tick();
  stepper.ch[1].tick();
}

void StepperDrive::startTimer() {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  stepTimer = timerBegin(1000000);                          // 1 MHz
  timerAttachInterrupt(stepTimer, &onStepTimer);
  timerAlarm(stepTimer, 1000000UL / STEP_ISR_HZ, true, 0);
#else
  stepTimer = timerBegin(0, 80, true);                      // 80 MHz / 80 = 1 MHz
  timerAttachInterrupt(stepTimer, &onStepTimer, true);
  timerAlarmWrite(stepTimer, 1000000UL / STEP_ISR_HZ, true);
  timerAlarmEnable(stepTimer);
#endif
}

//------------------------------------------------------------------------------
// StepperDrive
//------------------------------------------------------------------------------
void StepperDrive::begin() {
  wd = 0;
  pinMode(PIN_EN, OUTPUT);
  enable(false);                                            // tắt driver ngay từ đầu
  ch[0].init(PIN_STEP_L, PIN_DIR_L, MOTOR_L_INVERT != 0);
  ch[1].init(PIN_STEP_R, PIN_DIR_R, MOTOR_R_INVERT != 0);
  for (int i = 0; i < 2; i++) {
    pinMode(ch[i].stepPin, OUTPUT);
    pinMode(ch[i].dirPin, OUTPUT);
    digitalWrite(ch[i].stepPin, LOW);
    digitalWrite(ch[i].dirPin, ch[i].invert ? LOW : HIGH);  // khớp dirCur = +1 ban đầu
  }
}

void StepperDrive::enable(bool on) {
  digitalWrite(PIN_EN, (on == (EN_ACTIVE_LOW != 0)) ? LOW : HIGH);
}

void StepperDrive::setVelocity(int i, float v) {
  wd = 0;
  float f = fabsf(v) * STEPS_PER_M;
  if (f > MAX_STEP_HZ) f = MAX_STEP_HZ;
  uint32_t incNew = 0;
  if (f >= 1.0f) incNew = (uint32_t)((double)f * (4294967296.0 / (double)STEP_ISR_HZ));
  ch[i].dirReq = (v >= 0.0f) ? 1 : -1;
  ch[i].inc = incNew;
}

void StepperDrive::stop() {
  setVelocity(0, 0);
  setVelocity(1, 0);
}

float StepperDrive::positionMeters() const {
  return 0.5f * ((float)ch[0].steps + (float)ch[1].steps) / STEPS_PER_M;
}
