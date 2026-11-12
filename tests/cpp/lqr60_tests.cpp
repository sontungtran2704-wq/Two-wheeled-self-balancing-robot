#include "ControlCore.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
using namespace robot;
static Sensors sample(uint32_t t,double deg,double omega=0) {
  return {deg*3.14159265358979323846/180.,omega,0,0,t,true};
}
int main() {
  for(double angle : {-59.,-20.,20.,59.}) {
    Device d; uint32_t t=1000;
    d.tick(t,sample(t,angle));
    assert(d.command("START",t)=="ACK:START");
    for(int i=0;i<200;i++){t+=5000;d.tick(t,sample(t,angle));}
    assert(d.state()==State::Run);
    t+=5000;d.tick(t,sample(t,angle));
    assert(!d.output().motion_active && d.output().um==0);
    assert(d.command("MOTION,1",t)=="ERR:LQR_ONLY");
    t+=5000;d.tick(t,sample(t,angle>0?60.1:-60.1));
    assert(d.state()==State::Fault && !d.enabled());
    assert(std::strcmp(d.faultReason(),"FALL")==0);
    assert(d.output().left==0 && d.output().right==0);
    assert(d.command("RESET",t)=="ACK:RESET" && !d.enabled());
  }
  Device d;uint32_t t=1000;
  d.tick(t,sample(t,60.1));assert(d.command("START",t)=="ERR:POSE");
  d.tick(t,sample(t,0,1));assert(d.command("START",t)=="ERR:POSE");
  d.tick(t,sample(t,10));assert(d.command("START",t)=="ACK:START");
  t+=5000;d.tick(t,sample(t,61));assert(d.state()==State::Fault);
  d.command("RESET",t);d.tick(t,sample(t,10));d.command("START",t);
  for(int i=0;i<200;i++){t+=5000;d.tick(t,sample(t,10));}
  for(int i=0;i<300 && d.enabled();i++){t+=5000;d.tick(t,sample(t,10));}
  assert(std::strcmp(d.faultReason(),"SPEED_RAIL")==0);
  assert(!d.enabled());
  std::cout<<"PASS: tilted ARM, +/-60 FALL, Arming FALL, rate guard, PID locked, RESET, SPEED_RAIL\n";
}
