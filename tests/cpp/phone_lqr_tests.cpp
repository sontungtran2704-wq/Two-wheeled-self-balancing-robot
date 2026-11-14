#include "ControlCore.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
int main() {
  using namespace robot;
  Device d;assert(d.k0()==cfg::k0&&d.k1()==cfg::k1);
  const std::string prefix=std::string("LQR2,")+cfg::sha256+",";
  assert(d.command((prefix+"-23,-2.7").c_str(),1000)=="ERR:IMU");
  Sensors s;s.valid=true;s.timestamp_us=1000;d.tick(1000,s);
  assert(d.command((prefix+"-23,-2.7").c_str(),1000).find("ACK:LQR2,")==0);
  assert(d.k0()==-23&&d.k1()==-2.7);
  assert(d.command("GET",1000).find("LQR:-23,-2.7")!=std::string::npos);
  assert(d.telemetry(1000).find("lqr2=1")!=std::string::npos);
  assert(d.command((prefix+"23,2").c_str(),1000)=="ERR:UNSTABLE_GAIN");
  assert(d.command((prefix+"-23,-1000").c_str(),1000)=="ERR:UNSTABLE_GAIN");
  assert(d.command((prefix+"nan,-2").c_str(),1000)=="ERR:GAIN_RANGE");
  assert(d.command((prefix+"-10001,-2").c_str(),1000)=="ERR:GAIN_RANGE");
  assert(d.command("LQR2,bad,-23,-2",1000)=="ERR:CONFIG");
  assert(d.k0()==-23&&d.k1()==-2.7);
  assert(d.command((prefix+"-23,-2.7").c_str(),20000)=="ERR:IMU");
  assert(d.command("START",1000)=="ACK:START");
  assert(d.command((prefix+"-23,-2.7").c_str(),1000)=="ERR:STATE");
  d.fault("ESTOP");assert(d.command((prefix+"-23,-2.7").c_str(),1000)=="ERR:STATE");
  Device reboot;assert(reboot.k0()==cfg::k0&&reboot.k1()==cfg::k1);
  NumericController c;assert(c.setGains(-23,-2.7));c.reset();assert(c.k0()==-23);
  auto o=c.tick(.01,.01,0,0,0,0,false,true);assert(std::abs(o.ub-.257)<1e-12);
  assert(!c.setGains(std::numeric_limits<double>::infinity(),-2.7));assert(c.k0()==-23);
  std::cout<<"PASS: explicit LQR2 requires Idle/fresh calibrated IMU/exact config, rejects invalid/unstable K, confirms/readbacks applied K, default K unchanged and reboot restores defaults\n";
}
