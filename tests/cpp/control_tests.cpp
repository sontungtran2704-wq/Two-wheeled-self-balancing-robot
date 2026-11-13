#include "ControlCore.h"
#include "WifiSettings.h"
#include "StepNco.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace robot;
static Sensors good(uint32_t t) { return {0,0,0,0,t,true}; }
static void advance(Device& d,uint32_t& t,int n) { for(int i=0;i<n;++i) {t+=5000; d.tick(t,good(t));} }
static void arm(Device& d,uint32_t& t) {
  d.tick(t,good(t)); assert(d.command("START",t)=="ACK:START");
  advance(d,t,201); assert(d.state()==State::Run);
}
int main() {
  Device d; uint32_t t=1000; d.tick(t,good(t));
  const auto defaults=d.tuning();
  assert(defaults.vkp==3 && defaults.vki==1 && defaults.vkd==.2);
  assert(d.state()==State::Idle && !d.enabled());
  assert(d.command("START",t)=="ACK:START");
  advance(d,t,100); assert(!d.enabled());
  Sensors tilted=good(t+5000); tilted.theta=.08; t+=5000; d.tick(t,tilted);
  advance(d,t,199); assert(d.state()==State::Arming);
  advance(d,t,2); assert(d.state()==State::Run && d.enabled());
  assert(d.command("MOTION,1",t)=="ACK:MOTION");
  assert(d.command("SPEED,20",t)=="ACK:SPEED");
  advance(d,t,220); assert(d.output().motion_active); // expired lease retains speed feedback
  assert(!d.commandFresh(t)); assert(std::abs(d.output().vref)<1e-12);
  assert(d.command("ESTOP",t)=="ACK:ESTOP"); assert(d.state()==State::Fault);
  assert(d.command("START",t)=="ERR:RESET_REQUIRED");
  d.command("STOP",t); assert(d.state()==State::Fault);
  assert(d.command("RESET",t)=="ACK:RESET"); assert(!d.enabled());
  arm(d,t);
  auto session=std::to_string(d.session());
  assert(d.command(("CMD,"+session+",4294967295,0.1,0.2").c_str(),t)=="ACK:CMD");
  assert(d.command(("CMD,"+session+",0,0.1,0.2").c_str(),t)=="ACK:CMD");
  assert(d.command(("CMD,"+session+",0,0.1,0.2").c_str(),t)=="ERR:SEQUENCE");
  assert(d.command(("CMD,"+session+",4294967295,0.1,0.2").c_str(),t)=="ERR:SEQUENCE");
  assert(d.command("CMD,0,1,0,0",t)=="ERR:SESSION");
  for(const char* bad: {"SPEED,nan","SPEED,inf","SPEED,20x","SPEED,20,1","TURN,46","MOTION,2","CMD,1,-1,0,0","CMD,4294967296,0,0,0","$P,V,1,0,0,4"})
    assert(d.command(bad,t).rfind("ERR:",0)==0);
  assert(d.command(std::string(161,'A').c_str(),t)=="ERR:LINE_TOO_LONG");
  assert(d.command("$P,V,1,0.1,0.01",t)=="ERR:MOTORS_ON");
  assert(d.command("$K,1,2",t)=="ERR:READ_ONLY_GAIN");
  d.command("STOP",t);
  assert(d.command("$P,V,1,0.1,0.01",t)=="ACK:$P");
  assert(d.command("$P,Y,6,0,0",t)=="ERR:GAIN_RANGE");
  arm(d,t); t+=15000; auto s=good(t); d.tick(t,s); assert(d.state()==State::Run);
  t+=15001; d.tick(t,good(t)); assert(d.state()==State::Fault);
  d.command("RESET",t); arm(d,t); t+=5000; s=good(t-15001); d.tick(t,s); assert(d.state()==State::Fault);
  d.command("RESET",t); arm(d,t); t+=5000;s=good(t);s.omega=std::numeric_limits<double>::quiet_NaN();d.tick(t,s);assert(d.state()==State::Fault);
  Device wrap; uint32_t tw=0xffff0000; arm(wrap,tw); assert(wrap.enabled());
  Device trim;
  auto trimSample=good(5000);trimSample.theta=.02;
  trim.tick(5000,trimSample);
  assert(trim.command("ZPHERE",5000).find("ACK:ZPHERE,zero_rad=0.020000")==0);
  assert(trim.telemetry(5000).find(",theta=0,")!=std::string::npos);
  assert(trim.command("ZP+",25001)=="ERR:IMU");
  trimSample.timestamp_us=10000;trimSample.theta=.1;trim.tick(10000,trimSample);
  assert(trim.command("ZPHERE",10000)=="ERR:POSE");
  NumericController c;
  auto settings=trim.tuning();
  Device restored;
  assert(restored.restoreTuning(settings));
  assert(restored.tuning().zero==settings.zero);
  auto invalid=settings;invalid.vkp=6;
  assert(!restored.restoreTuning(invalid));
  uint32_t rt=0;arm(restored,rt);
  assert(!restored.restoreTuning(settings));
  auto o=c.tick(.2,0,0,0,0,0,false,true); assert(std::abs(o.u-3)<1e-12 && o.saturated);
  assert(std::abs(o.left-.015)<1e-12 && std::abs(o.right-.015)<1e-12);
  for(int i=0;i<1000;++i) {o=c.tick(.2,0,0,0,0,0,false,true);assert(std::abs(o.left)<=.600000000001 && std::abs(o.right)<=.600000000001);}
  NumericController stop;
  for(int i=0;i<205;++i) o=stop.tick(0,0,0,0,0,0,true,true);
  assert(o.motion_active); for(int i=0;i<5;++i)o=stop.tick(0,0,0,0,0,0,false,true); assert(!o.motion_active);
  NumericController rails;double previousL=0,previousR=0;
  for(int i=0;i<2000;++i) {
    o=rails.tick(i<300?0:(i<1000?.2:-.2),0,0,0,.2,.8,true,true);
    assert(std::abs(o.left-previousL)<=.015000000001 && std::abs(o.right-previousR)<=.015000000001);
    assert(std::abs(o.left)<=.600000000001 && std::abs(o.right)<=.600000000001);
    previousL=o.left;previousR=o.right;
  }
  Device gains;Sensors gs;gs.valid=true;gains.tick(1000,gs);
  const std::string prefix=std::string("LQR2,")+cfg::sha256+",";
  assert(gains.command((prefix+"-23.793396834761598,-2.7542335398052704").c_str(),1000).find("ACK:LQR2,")==0);
  assert(gains.command((prefix+"23,2").c_str(),1000)=="ERR:UNSTABLE_GAIN");
  assert(gains.command((prefix+"-23,-1000").c_str(),1000)=="ERR:UNSTABLE_GAIN");
  assert(gains.command((prefix+"nan,-2").c_str(),1000)=="ERR:GAIN_RANGE");
  assert(gains.command("LQR2,bad,-23,-2",1000)=="ERR:CONFIG");
  assert(gains.command("GET",1000).find("LQR:-23.7933968348,-2.75423353981")!=std::string::npos);
  assert(gains.telemetry(1000).find("lqr2=1")!=std::string::npos);
  gains.command("START",1000);assert(gains.command((prefix+"-23,-2").c_str(),1000)=="ERR:STATE");
  gains.fault("ESTOP");assert(gains.command((prefix+"-23,-2").c_str(),1000)=="ERR:STATE");
  wifi_settings::Staging wifi;bool commit=false;
  assert(!std::strcmp(wifi.command("WIFI_SAVE",1,0,true,commit),"ERR:WIFI_SEQUENCE")&&!commit);
  assert(!std::strcmp(wifi.command("WIFI_SSID,486f6d65",1,10,true,commit),"ACK:WIFI_SSID"));
  assert(!std::strcmp(wifi.command("WIFI_PASS,70617373776f7264313233",2,20,true,commit),"ERR:OWNER"));
  assert(!std::strcmp(wifi.command("WIFI_PASS,70617373776f7264313233",1,30,true,commit),"ACK:WIFI_PASS"));
  assert(!std::strcmp(wifi.command("WIFI_SAVE",1,40,true,commit),"ACK:WIFI_SAVED")&&commit);
  assert(wifi.value().valid()&&!std::strcmp(wifi.value().ssid,"Home"));
  assert(!std::strcmp(wifi.command("WIFI_SAVE",1,50,false,commit),"ERR:STATE")&&!commit);
  wifi.command("WIFI_SSID,486f6d65",1,60,true,commit);
  assert(!std::strcmp(wifi.command("WIFI_PASS,00",1,70,true,commit),"ERR:WIFI_PASSWORD"));
  wifi.command("WIFI_SSID,486f6d65",1,100,true,commit);
  assert(!std::strcmp(wifi.command("WIFI_PASS,",1,10000101,true,commit),"ERR:WIFI_SEQUENCE"));
  wifi.command("WIFI_SSID,486f6d65",1,200,true,commit);
  assert(!std::strcmp(wifi.command("WIFI_PASS,",1,210,true,commit),"ACK:WIFI_PASS"));
  assert(wifi.value().valid());
  StepNco nco; nco.setRate(15000); int rises=0,last=-100;
  for(int i=0;i<40000;++i) {auto e=nco.tick(); if(e.rise){++rises;assert(i-last>=2);last=i;}assert(!(e.rise&&e.fall));}
  assert(rises==15000);
  nco.setRate(-15000);auto e=nco.tick();assert(e.direction_changed && !e.rise);
  nco.disable(); for(int i=0;i<10;++i)assert(!nco.tick().rise);
  std::cout << "PASS: state, arming, fault latch, lease, parser, session, sequence wrap, dt, stale IMU, NaN, clock wrap, allocation, mode, NCO\n";
}
