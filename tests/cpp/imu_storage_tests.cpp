// Execute the real IMU task against in-memory Wire/NVS/clock fakes. No board access.
#include "ImuHal.cpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
static void boot() {
  imu::recalibrate=false;imu::ready=false;imu::latest={};imu::theta=0;
  for(auto& b:imu::bias)b=0;
  imu::savedCalibration=false;imu::restoredCalibration=false;imu::calibrationBusy=false;
  imu::sampleTime=0;fake_imu::clock=0;fake_imu::gyroY=65;fake_imu::readFailure=false;fake_imu::writeFailure=false;
}
static void run(uint32_t duration=1000000) {
  fake_imu::deadline=fake_imu::clock+duration;
  try{imu::task(nullptr);}catch(const fake_imu::End&){}
}
int main() {
  boot();run();assert(fake_imu::writes==0&&!imu::calibrated()&&imu::diagnostic()==7);
  imu::requestCalibration();run(4000000);
  assert(imu::calibrated()&&imu::calibrationSaved()&&!imu::calibrationRestored());assert(fake_imu::writes==1);
  assert(imu::snapshot().valid&&std::abs(imu::snapshot().omega)<1e-12);
  const auto stored=fake_imu::nvs["imu-cal-v1/gyro"];
  // Only the firmware changes; retaining NVS must not authorize firmware B with calibration A.
  boot();fake_imu::app.app_elf_sha256[0]=2;run();
  assert(!imu::calibrated()&&!imu::snapshot().valid&&!imu::calibrationSaved()&&imu::diagnostic()==7);
  assert(fake_imu::writes==1&&fake_imu::nvs["imu-cal-v1/gyro"]==stored);
  robot::Device newFirmware;newFirmware.enableAutonomous(true);
  uint32_t now=micros();newFirmware.tick(now,imu::snapshot());
  assert(newFirmware.command("START",now)=="ERR:IMU");
  for(int i=0;i<1200;++i){now+=5000;auto invalid=imu::snapshot();invalid.timestamp_us=now;newFirmware.tick(now,invalid);}
  assert(!newFirmware.enabled());
  imu::requestCalibration();run(4000000);
  assert(imu::calibrated()&&imu::calibrationSaved()&&fake_imu::writes==2);
  const auto firmwareB=fake_imu::nvs["imu-cal-v1/gyro"];assert(firmwareB!=stored);
  boot();run();assert(imu::calibrated()&&imu::calibrationRestored()&&fake_imu::writes==2);
  // Reusing the exact same image is indistinguishable from power cycling and retains its calibration.
  boot();run();assert(imu::calibrated()&&fake_imu::writes==2);
  boot();fake_imu::app.app_elf_sha256[0]=1;run();assert(!imu::calibrated());
  fake_imu::nvs["imu-cal-v1/gyro"]=stored;fake_imu::writes=1;
  boot();run();assert(imu::calibrated()&&imu::calibrationSaved()&&imu::calibrationRestored());
  assert(fake_imu::writes==1&&fake_imu::nvs["imu-cal-v1/gyro"]==stored);
  // A fresh gyro read differs from the saved mean: restore must not silently re-estimate it.
  boot();fake_imu::gyroY=70;run();assert(imu::calibrated());
  assert(std::abs(imu::snapshot().omega-5./65.5*3.14159265358979323846/180.)<1e-12);
  assert(fake_imu::writes==1&&fake_imu::nvs["imu-cal-v1/gyro"]==stored);
  boot();fake_imu::gyroY=350;run();assert(!imu::calibrated()&&!imu::snapshot().valid);
  assert(fake_imu::writes==1&&fake_imu::nvs["imu-cal-v1/gyro"]==stored);
  boot();fake_imu::nvs["imu-cal-v1/gyro"][20]^=1;run();assert(!imu::calibrated()&&imu::diagnostic()==7&&fake_imu::writes==1);
  fake_imu::nvs["imu-cal-v1/gyro"]=stored;
  boot();run();imu::requestCalibration();fake_imu::gyroY=70;run(4000000);
  assert(imu::calibrated()&&imu::calibrationSaved()&&fake_imu::writes==2);
  const auto recalibrated=fake_imu::nvs["imu-cal-v1/gyro"];assert(recalibrated!=stored);
  boot();run();imu::requestCalibration();fake_imu::writeFailure=true;run(4000000);
  assert(!imu::calibrated()&&!imu::calibrationSaved()&&imu::diagnostic()==8);
  assert(fake_imu::nvs["imu-cal-v1/gyro"]==recalibrated);
  boot();fake_imu::readFailure=true;run();assert(!imu::calibrated()&&!imu::snapshot().valid);
  std::cout<<"PASS: new firmware rejects previous calibration and blocks manual/auto START; explicit calibration binds image; same-image power cycles reuse without writes; corrupt/moving/I2C/write failures block readiness\n";
}
