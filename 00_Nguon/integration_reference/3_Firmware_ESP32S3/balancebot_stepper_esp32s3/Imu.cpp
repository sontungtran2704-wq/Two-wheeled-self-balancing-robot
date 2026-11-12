#include "Imu.h"
#include <Wire.h>

Imu imu;

Imu::Imu() : ax(0), ay(0), az(0), gx(0), gy(0), gz(0), failCount(0) {
  gyroBias[0] = gyroBias[1] = gyroBias[2] = 0;
}

bool Imu::writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg); Wire.write(val);
  return Wire.endTransmission() == 0;
}

bool Imu::begin() {
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  Wire.setTimeOut(10);                       // ms - tránh treo vòng điều khiển nếu bus kẹt
  if (!writeReg(0x6B, 0x01)) return false;   // thoát sleep, clock = PLL gyro X
  delay(50);
  writeReg(0x19, 0x00);                      // sample rate = 1 kHz
  writeReg(0x1A, 0x02);                      // DLPF ~94 Hz
  writeReg(0x1B, 0x08);                      // gyro ±500 dps
  writeReg(0x1C, 0x08);                      // accel ±4 g
  return true;
}

bool Imu::readRaw(float* a, float* g) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint16_t)MPU_ADDR, (uint8_t)14, (bool)true) != 14) return false;
  int16_t r[7];
  for (int i = 0; i < 7; i++) {
    uint8_t hi = Wire.read();
    uint8_t lo = Wire.read();
    r[i] = (int16_t)((hi << 8) | lo);
  }
  a[0] = r[0] / ACC_LSB_PER_G; a[1] = r[1] / ACC_LSB_PER_G; a[2] = r[2] / ACC_LSB_PER_G;
  g[0] = r[4] / GYRO_LSB_PER_DPS * DEG2RAD;
  g[1] = r[5] / GYRO_LSB_PER_DPS * DEG2RAD;
  g[2] = r[6] / GYRO_LSB_PER_DPS * DEG2RAD;
  return true;
}

bool Imu::update() {
  float a[3], g[3];
  if (!readRaw(a, g)) {
    if (failCount < 255) failCount++;        // chặn tràn số (bản cũ: sau 256 lỗi quay về 0)
    return false;
  }
  failCount = 0;
  ax = a[0]; ay = a[1]; az = a[2];
  gx = g[0] - gyroBias[0]; gy = g[1] - gyroBias[1]; gz = g[2] - gyroBias[2];
  return true;
}

// Hiệu chuẩn gyro: xe đặt yên. Trả về true nếu đủ ổn định.
bool Imu::calibrateGyro() {
  const int N = 400;
  double sum[3] = {0, 0, 0}, sq[3] = {0, 0, 0};
  int n = 0, fails = 0;
  uint32_t t0 = micros();
  while (n < N) {
    if ((uint32_t)(micros() - t0) < 2000) continue;
    t0 += 2000;
    float a[3], g[3];
    if (!readRaw(a, g)) {
      if (++fails > 50) return false;        // cảm biến mất kết nối -> không treo vô hạn
      continue;
    }
    for (int k = 0; k < 3; k++) { sum[k] += g[k]; sq[k] += (double)g[k] * g[k]; }
    n++;
  }
  float mean[3]; float maxStd = 0;
  for (int k = 0; k < 3; k++) {
    mean[k] = (float)(sum[k] / N);
    float var = (float)(sq[k] / N) - mean[k] * mean[k];
    float sd = sqrtf(var > 0 ? var : 0);
    if (sd > maxStd) maxStd = sd;
  }
  if (maxStd > 0.02f) return false;          // 0.02 rad/s: xe đang bị rung / di chuyển
  for (int k = 0; k < 3; k++) gyroBias[k] = mean[k];
  return true;
}
