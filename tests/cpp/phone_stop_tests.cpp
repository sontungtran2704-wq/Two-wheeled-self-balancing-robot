#include "ControlCore.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
int main(){
  robot::Device d;robot::Sensors s;s.valid=true;uint32_t now=1000;s.timestamp_us=now;d.tick(now,s);
  assert(d.command("START",now)=="ACK:START");
  auto advance=[&](int ticks){for(int i=0;i<ticks;++i){now+=5000;s.timestamp_us=now;d.tick(now,s);}};
  advance(500);assert(d.enabled());assert(d.command("MOTION,1",now)=="ACK:MOTION");
  assert(d.command("SPEED,10",now)=="ACK:SPEED");assert(d.command("TURN,20",now)=="ACK:TURN");advance(20);
  assert(d.command("SPEED,0",now)=="ACK:SPEED");assert(d.command("TURN,0",now)=="ACK:TURN");advance(200);
  assert(d.enabled()&&d.state()==robot::State::Run);
  assert(d.command("ESTOP",now)=="ACK:ESTOP");assert(!d.enabled()&&d.state()==robot::State::Fault);assert(!std::strcmp(d.faultReason(),"ESTOP"));
  assert(d.command("START",now)=="ERR:RESET_REQUIRED");assert(d.command("RESET",now)=="ACK:RESET");assert(!d.enabled()&&d.state()==robot::State::Idle);
  std::cout<<"PASS: speed/yaw zero preserves Run and balancing; ESTOP disables motor/latches fault and requires RESET before START\n";
}
