#include "ControlCore.h"
#include "SavedImuCalibration.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>
#include <iostream>
using namespace robot;
constexpr double rad(double deg){return deg*3.14159265358979323846/180.;}
static Sensors sample(uint32_t t,double degrees=0){return {rad(degrees),0,0,0,t,true};}
static void advance(Device& d,uint32_t& t,unsigned ticks,double deg=0) {
  for(unsigned i=0;i<ticks;++i){t+=5000;d.tick(t,sample(t,deg));}
}
static void automatic(Device& d,uint32_t& t) {
  d.enableAutonomous(true);d.tick(t,sample(t));advance(d,t,1001);assert(d.enabled());
}
int main(){
  assert(std::abs(cfg::arm_rad-rad(10))<1e-15&&std::abs(cfg::fall_rad-rad(40))<1e-15&&cfg::auto_rearm_s==5);
  // A running robot must oppose drift immediately, before the drive/yaw stability gate opens.
  for(double drift:{-.45,.45}) {
    NumericController c;
    auto o=c.tick(rad(5),0,drift,0,.1,.2,true,true);
    assert(c.velocity.ready&&o.aref*drift<0);
    assert(!o.motion_active&&o.vref==0&&o.wref==0&&!c.yaw.ready);
    assert(std::abs(o.aref)<=cfg::accel_max);
    for(int i=0;i<600;++i) {
      o=c.tick(rad(5),0,drift,0,.1,.2,true,true);
      assert(!o.motion_active&&o.vref==0&&o.wref==0);
    }
    c.reset();o=c.tick(rad(5),0,drift,0,.1,.2,true,false);
    assert(c.velocity.ready&&o.aref*drift<0&&o.vref==0&&o.wref==0);
    c.reset();o=c.tick(rad(5),0,drift,0,.1,.2,false,false);
    assert(!c.velocity.ready&&o.aref==0&&!o.motion_active);
  }
  std::cout<<"PASS: r4 immediately opposes +/-0.45 m/s drift before drive/yaw gate; no-link zero target; disabled state unchanged\n";
  Device manual;uint32_t manualTime=1000;manual.tick(manualTime,sample(manualTime,10));
  assert(manual.command("START",manualTime)=="ERR:POSE");
  manualTime+=5000;manual.tick(manualTime,sample(manualTime,9));
  assert(manual.command("START",manualTime)=="ACK:START");
  advance(manual,manualTime,199,9);assert(!manual.enabled());advance(manual,manualTime,1,9);assert(manual.enabled());
  Device offline;uint32_t offlineTime=1000;automatic(offline,offlineTime);advance(offline,offlineTime,2000);
  assert(offline.enabled()&&!offline.commandFresh(offlineTime)&&offline.output().motion_active);
  assert(offline.output().vref==0&&offline.output().wref==0&&offline.output().left==0&&offline.output().right==0);
  Device d;uint32_t t=1000;d.enableAutonomous(true);d.tick(t,sample(t));
  advance(d,t,999);assert(!d.enabled()&&d.state()==State::Idle);advance(d,t,1);assert(d.enabled());
  const auto runningSession=d.session();
  assert(d.command(("CMD,"+std::to_string(runningSession)+",1,.2,.3").c_str(),t)=="ACK:CMD");
  t+=5000;d.tick(t,sample(t,40));assert(d.state()==State::Fault&&!d.enabled());assert(!std::strcmp(d.faultReason(),"FALL"));
  advance(d,t,1100,10);assert(!d.enabled()); // Strictly below 10, either sign.
  advance(d,t,1100,-10);assert(!d.enabled());
  advance(d,t,600,9);advance(d,t,1,11);advance(d,t,999,9);assert(!d.enabled());
  advance(d,t,2,9);assert(d.enabled()&&d.session()!=runningSession);
  assert(d.output().left==0&&d.output().right==0&&d.output().vref==0&&d.output().wref==0);
  assert(d.command(("CMD,"+std::to_string(runningSession)+",2,.2,.3").c_str(),t)=="ERR:SESSION");
  t+=5000;d.tick(t,sample(t));assert(d.output().vref==0&&d.output().wref==0);
  d.command("STOP",t);advance(d,t,1200);assert(!d.enabled()&&!d.autonomousWaiting());
  assert(d.command("START",t)=="ACK:START");advance(d,t,200);assert(d.enabled());
  d.fault("FALL");advance(d,t,500);d.command("STOP",t);advance(d,t,1100);assert(!d.enabled());
  for(const char* fault:{"ESTOP","IMU","DEADLINE","STEP_WATCHDOG","INITIALIZATION"}) {
    Device f;uint32_t ft=1000;automatic(f,ft);f.fault(fault);advance(f,ft,1200);
    assert(f.state()==State::Fault&&!f.enabled()&&!f.autonomousWaiting());
  }
  // Exercise an actual speed-rail fault from the unchanged numeric controller.
  Device rail;uint32_t railTime=1000;automatic(rail,railTime);
  const auto railSession=rail.session();
  for(unsigned i=0;i<300 && rail.enabled();++i)advance(rail,railTime,1,20);
  assert(rail.state()==State::Fault&&!std::strcmp(rail.faultReason(),"SPEED_RAIL"));
  advance(rail,railTime,1100,10);assert(!rail.enabled());
  advance(rail,railTime,900,0);
  railTime+=5000;auto railSample=sample(railTime);railSample.omega=cfg::stable_rate;rail.tick(railTime,railSample);
  advance(rail,railTime,1000,0);assert(!rail.enabled());advance(rail,railTime,1,0);
  assert(rail.enabled()&&!std::strcmp(rail.faultReason(),"NONE")&&rail.session()!=railSession);
  assert(rail.output().left==0&&rail.output().right==0&&rail.output().vref==0&&rail.output().wref==0);
  rail.fault("SPEED_RAIL");advance(rail,railTime,600);rail.command("STOP",railTime);advance(rail,railTime,1100);
  assert(!rail.enabled()&&rail.state()==State::Fault);

  // CALGYRO/SAVE service callbacks restart the dwell, not require a web START.
  Device serviced;uint32_t serviceTime=1000;serviced.enableAutonomous(true);serviced.tick(serviceTime,sample(serviceTime));
  advance(serviced,serviceTime,900);serviced.restartAutonomousCountdown();
  for(int i=0;i<800;++i){serviceTime+=5000;auto pending=sample(serviceTime);pending.valid=false;serviced.tick(serviceTime,pending);}
  advance(serviced,serviceTime,1000);assert(!serviced.enabled());advance(serviced,serviceTime,1);assert(serviced.enabled());
  Device afterSave;uint32_t saveTime=1000;afterSave.enableAutonomous(true);afterSave.tick(saveTime,sample(saveTime));
  advance(afterSave,saveTime,900);afterSave.restartAutonomousCountdown();advance(afterSave,saveTime,1000);assert(!afterSave.enabled());advance(afterSave,saveTime,1);assert(afterSave.enabled());
  Device deliberateStop;uint32_t dsTime=1000;deliberateStop.enableAutonomous(true);deliberateStop.tick(dsTime,sample(dsTime));
  deliberateStop.command("STOP",dsTime);deliberateStop.restartAutonomousCountdown();advance(deliberateStop,dsTime,1200);assert(!deliberateStop.enabled());
  Device tuned;uint32_t tuneTime=1000;tuned.enableAutonomous(true);tuned.tick(tuneTime,sample(tuneTime));advance(tuned,tuneTime,900);
  assert(tuned.command("$P,V,3,1,.2",tuneTime)=="ACK:$P");advance(tuned,tuneTime,1000);assert(!tuned.enabled());advance(tuned,tuneTime,1);assert(tuned.enabled());
  Device zeroed;uint32_t zpTime=1000;zeroed.enableAutonomous(true);zeroed.tick(zpTime,sample(zpTime));advance(zeroed,zpTime,900);
  assert(zeroed.command("ZPHERE",zpTime).rfind("ACK:ZPHERE",0)==0);advance(zeroed,zpTime,1000);assert(!zeroed.enabled());advance(zeroed,zpTime,1);assert(zeroed.enabled());
  Device reset;uint32_t rt=1000;automatic(reset,rt);reset.fault("FALL");
  assert(reset.command("RESET",rt)=="ACK:RESET");advance(reset,rt,1200);assert(!reset.enabled());
  Device invalid;uint32_t it=1000;invalid.enableAutonomous(true);invalid.tick(it,sample(it));advance(invalid,it,500);
  it+=5000;auto s=sample(it);s.valid=false;invalid.tick(it,s);advance(invalid,it,999);assert(!invalid.enabled());advance(invalid,it,2);assert(invalid.enabled());
  Device motion;uint32_t mt=1000;motion.enableAutonomous(true);motion.tick(mt,sample(mt));advance(motion,mt,700);
  mt+=5000;s=sample(mt);s.omega=cfg::stable_rate;motion.tick(mt,s);advance(motion,mt,999);assert(!motion.enabled());
  mt+=5000;s=sample(mt);s.yaw=std::numeric_limits<double>::quiet_NaN();motion.tick(mt,s);advance(motion,mt,999);assert(!motion.enabled());
  mt+=20000;motion.tick(mt,sample(mt));advance(motion,mt,999);assert(!motion.enabled());advance(motion,mt,2);assert(motion.enabled());
  Device stale;uint32_t st=1000;stale.enableAutonomous(true);stale.tick(st,sample(st));advance(stale,st,500);
  st+=5000;stale.tick(st,sample(st-20000));advance(stale,st,999);assert(!stale.enabled());
  Device wrap;uint32_t wt=0xfff00000;automatic(wrap,wt);assert(wrap.enabled());
  Device suspended;uint32_t pt=1000;suspended.enableAutonomous(true);suspended.tick(pt,sample(pt));advance(suspended,pt,900);
  suspended.suspendAutonomous();advance(suspended,pt,1200);assert(!suspended.enabled());
  Device locked;uint32_t lt=1000;locked.enableAutonomous(false);advance(locked,lt,1200);assert(!locked.enabled());
  assert(std::abs(d.tuning().vkp-3)<1e-15&&d.tuning().vki==1&&d.tuning().vkd==.2);

  imu::CalibrationContext context{0x70,0x68,8,9,0};imu::SavedGyro saved;
  context.firmwareSha256[0]=1;
  saved.bias[0]=.01;saved.bias[1]=-.015;saved.bias[2]=.005;saved.norm=1;saved.accelStd=.002;saved.gyroStd=.001;
  uint8_t bytes[imu::calibrationBytes];assert(imu::encodeCalibration(bytes,context,saved));
  imu::SavedGyro loaded;assert(imu::decodeCalibration(bytes,sizeof bytes,context,loaded));assert(loaded.bias[1]==saved.bias[1]);
  for(size_t i=0;i<sizeof bytes;++i) {bytes[i]^=1;assert(!imu::decodeCalibration(bytes,sizeof bytes,context,loaded));bytes[i]^=1;}
  assert(!imu::decodeCalibration(bytes,sizeof bytes-1,context,loaded));
  auto other=context;other.who=0x68;assert(!imu::decodeCalibration(bytes,sizeof bytes,other,loaded));
  other=context;other.sda=10;assert(!imu::decodeCalibration(bytes,sizeof bytes,other,loaded));
  other=context;other.pitchTrim=.01;assert(!imu::decodeCalibration(bytes,sizeof bytes,other,loaded));
  other=context;other.firmwareSha256[0]=2;assert(!imu::decodeCalibration(bytes,sizeof bytes,other,loaded));
  other=context;std::memset(other.firmwareSha256,0,32);assert(!imu::decodeCalibration(bytes,sizeof bytes,other,loaded));
  assert(!imu::encodeCalibration(bytes,other,saved));
  saved.bias[0]=std::numeric_limits<double>::quiet_NaN();assert(!imu::encodeCalibration(bytes,context,saved));
  saved.bias[0]=.2;assert(!imu::encodeCalibration(bytes,context,saved));
  std::cout<<"PASS: no-web boot/FALL/SPEED_RAIL recovery 5s, +/-10 boundary, 40 FALL, configuration dwell restart, STOP/RESET latch, other faults, stale/NaN/motion/gaps/wrap, zero targets/session, saved calibration CRC/context/ranges\n";
}
