#line 1 "D:\\blancing\\Robot_Collab\\03_Firmware_ESP32S3\\balancebot\\ImuHal.cpp"
#include "ImuHal.h"
#include "ImuCalibration.h"
#include "HardwareConfig.h"
#include <Wire.h>
#include <cmath>
#include <atomic>
namespace imu {
static portMUX_TYPE lock=portMUX_INITIALIZER_UNLOCKED;
static robot::Sensors latest;
static bool recalibrate=false,ready=false;
// 0=pending, 1=ready, 2=I2C read, 3=WHO_AM_I, 4=register write,
// 5=acceleration outside static range, 6=gyro motion or variance.
static std::atomic<unsigned> imuDiagnostic{0};
unsigned diagnostic(){return imuDiagnostic.load();}
static std::atomic<unsigned> imuIdentity{256};
unsigned identity(){return imuIdentity.load();}
static std::atomic<unsigned> calibrationAccelMg{0};
unsigned calibrationAccelMilliG(){return calibrationAccelMg.load();}
static double bias[3]={},theta=0;
static bool writeReg(uint8_t reg,uint8_t value) {
  Wire.beginTransmission(hardware::imuAddress);Wire.write(reg);Wire.write(value);return Wire.endTransmission()==0;
}
static bool readBytes(uint8_t reg,uint8_t* data,uint8_t n) {
  Wire.beginTransmission(hardware::imuAddress);Wire.write(reg);
  if(Wire.endTransmission(false)!=0 || Wire.requestFrom(hardware::imuAddress,n,true)!=n)return false;
  for(uint8_t i=0;i<n;++i)data[i]=Wire.read();return true;
}
static bool raw(double a[3],double g[3]) {
  uint8_t b[14];if(!readBytes(0x3B,b,14))return false;
  for(int i=0;i<3;++i) {
    a[i]=int16_t((uint16_t(b[2*i])<<8)|b[2*i+1])/8192.;
    g[i]=int16_t((uint16_t(b[8+2*i])<<8)|b[9+2*i])/65.5*(3.14159265358979323846/180.);
  }
  return true;
}
static bool setupImu() {
  Wire.begin(hardware::sda,hardware::scl,400000);Wire.setTimeOut(10);
  imuIdentity=256;
  if(!writeReg(0x6B,1)){imuDiagnostic=4;return false;}
  vTaskDelay(pdMS_TO_TICKS(100));
  uint8_t who=0;
  if(!readBytes(0x75,&who,1)){imuDiagnostic=2;return false;}
  imuIdentity=who;
  // The previous firmware could read compatible register maps without an ID
  // check. Permit that diagnostic path only in the motor-locked image.
  const bool mpu6500 = who == 0x70;
  if(who!=0x68 && !mpu6500 && hardware::confirmed){imuDiagnostic=3;return false;}
  const bool ok=writeReg(0x19,4) && writeReg(0x1A,2) && writeReg(0x1B,8) && writeReg(0x1C,8);
  uint8_t registers[4]{};
  uint8_t accelFilter=0;
  const bool filterOk=!mpu6500 ||
      (writeReg(0x1D,2) && readBytes(0x1D,&accelFilter,1) && (accelFilter & 0x0F)==2);
  const bool verified=ok && filterOk && readBytes(0x19,registers,4) &&
      registers[0]==4 && registers[1]==2 && registers[2]==8 && registers[3]==8;
  if(!verified)imuDiagnostic=4;
  return verified;
}
static std::atomic<unsigned> accelStdMg{0},gyroStdMrad{0},sampleTime{0};
unsigned calibrationAccelStdMg(){return accelStdMg.load();}
unsigned calibrationGyroStdMrad(){return gyroStdMrad.load();}
bool sampleFresh(){const auto t=sampleTime.load();return t && uint32_t(micros()-t)<=20000;}
static void publish(const double g[3],bool controlValid) {
  const uint32_t now=micros();
  robot::Sensors s{theta,g[1]-(controlValid?bias[1]:0),0,
      (g[2]-(controlValid?bias[2]:0))/std::max(.8,std::cos(theta)),now,controlValid};
  portENTER_CRITICAL(&lock);latest=s;portEXIT_CRITICAL(&lock);
  sampleTime=now;
}
static bool calibrate() {
  for(int window=0;window<3;++window) {
    CalibrationWindow stats;
    for(int i=0;i<500;++i) {
      double a[3],g[3];
      if(!raw(a,g)){imuDiagnostic=2;return false;}
      stats.add(a,g);
      theta=std::atan2(-a[0],a[2])-hardware::pitchTrim;
      publish(g,false);
      vTaskDelay(pdMS_TO_TICKS(5));
    }
    calibrationAccelMg=static_cast<unsigned>(std::lround(stats.normMean*1000));
    accelStdMg=static_cast<unsigned>(std::lround(stats.accelStd()*1000));
    gyroStdMrad=static_cast<unsigned>(std::lround(stats.gyroStd()*1000));
    const unsigned result=stats.result();
    if(result==1){
      for(int j=0;j<3;++j)bias[j]=stats.gmean[j];
      theta=std::atan2(-stats.amean[0],stats.amean[2])-hardware::pitchTrim;
      return true;
    }
    if(window==2)imuDiagnostic=result;
  }
  return false;
}
void requestCalibration(){portENTER_CRITICAL(&lock);recalibrate=true;ready=false;latest.valid=false;portEXIT_CRITICAL(&lock);}
bool calibrated(){portENTER_CRITICAL(&lock);const bool value=ready;portEXIT_CRITICAL(&lock);return value;}
robot::Sensors snapshot(){portENTER_CRITICAL(&lock);const auto s=latest;portEXIT_CRITICAL(&lock);return s;}
void task(void*) {
  if(!hardware::pinsConfigured){vTaskDelete(nullptr);return;}
  bool initialized=setupImu();requestCalibration();uint32_t previous=micros();
  TickType_t wake=xTaskGetTickCount();
  for(;;) {
    portENTER_CRITICAL(&lock);const bool requested=recalibrate;recalibrate=false;portEXIT_CRITICAL(&lock);
    if(requested) {
      imuDiagnostic=0;
      calibrationAccelMg=0;
      if(!initialized)initialized=setupImu();
      vTaskDelay(pdMS_TO_TICKS(1000));const bool ok=initialized && calibrate();
      if(ok)imuDiagnostic=1;
      portENTER_CRITICAL(&lock);ready=ok;portEXIT_CRITICAL(&lock);previous=micros();wake=xTaskGetTickCount();
    }
    double a[3],g[3];
    if(initialized && raw(a,g)) {
      const uint32_t now=micros();const double dt=double(uint32_t(now-previous))*1e-6;previous=now;
      if(dt>0 && dt<.02) {
        const double omega=g[1]-bias[1],norm=std::sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
        const bool controlValid=calibrated();
        if(controlValid)theta+=omega*dt;
        else theta=std::atan2(-a[0],a[2])-hardware::pitchTrim;
        // Accelerometer is a weak long-term correction, and is acceleration contaminated.
        if(controlValid && norm>.85&&norm<1.15)theta+=dt/(.5+dt)*(std::atan2(-a[0],a[2])-hardware::pitchTrim-theta);
        publish(g,controlValid);
      }
    }
    vTaskDelayUntil(&wake,pdMS_TO_TICKS(5));
  }
}
}

