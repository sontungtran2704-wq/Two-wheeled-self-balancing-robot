// Real Runtime.cpp/ControlCore.cpp with in-memory task, serial, NVS and Wi-Fi fakes. No COM/RF access.
#include "Runtime.cpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
namespace fake_runtime {inline bool calibrating=false;inline unsigned motorEnables=0;}
namespace imu {
void task(void*){}
robot::Sensors snapshot(){return {0,0,0,0,micros(),true};}
void requestCalibration(){fake_runtime::calibrating=true;}
bool calibrated(){return true;}
unsigned diagnostic(){return 1;} unsigned identity(){return 0x70;}
unsigned calibrationAccelMilliG(){return 1000;} unsigned calibrationAccelStdMg(){return 0;}
unsigned calibrationGyroStdMrad(){return 0;} bool sampleFresh(){return true;}
bool calibrationSaved(){return true;} bool calibrationRestored(){return true;}
bool calibrating(){return fake_runtime::calibrating;}
}
namespace stepper {
bool begin(){return true;}
void apply(double,double,bool enabled){if(enabled)++fake_runtime::motorEnables;}
void snapshot(int64_t& l,int64_t& r,bool& fault){l=r=0;fault=false;}
}
static void runControl(uint32_t us=5000) {
  fake_imu::deadline=micros()+us;
  try{controlTask(nullptr);}catch(const fake_imu::End&){}
}
static void runNetwork(uint32_t us) {
  fake_imu::deadline=micros()+us;
  try{networkTask(nullptr);}catch(const fake_imu::End&){}catch(const fake_runtime::Restart&){}
}
static void boot() {
  fake_runtime::queues.clear();fake_runtime::serialIn.clear();fake_runtime::serialOut.clear();fake_runtime::tcpOut.clear();
  fake_imu::clock=0;fake_imu::writeFailure=false;
  fake_runtime::restarts=fake_runtime::motorEnables=fake_runtime::begins=fake_runtime::reconnects=fake_runtime::serverBegins=0;
  fake_runtime::calibrating=false;fake_runtime::autoReconnect=false;fake_runtime::apOk=true;
  fake_runtime::status=6;fake_runtime::mode=0;fake_runtime::persistent=true;fake_runtime::tcpCandidate=false;
  homeWifi={};wifiStaging.clear();wifiRestart=false;benchApActive=false;transportOwner=0;stopRequest=0;
  client={};setupRobot();
}
static std::string command(const char* text,uint8_t source=1,bool consumeReply=true) {
  Request r{};std::strncpy(r.line,text,160);r.received=micros();r.source=source;
  assert(xQueueSend(requests,&r,0)==pdTRUE);runControl();
  if(!consumeReply)return {};
  Reply p{};assert(xQueueReceive(replies,&p,0)==pdTRUE);return p.line;
}
static void stage() {
  assert(command("WIFI_SSID,486f6d65")=="ACK:WIFI_SSID");
  assert(command("WIFI_PASS,70617373776f7264313233")=="ACK:WIFI_PASS");
}
int main() {
  fake_imu::nvs.clear();fake_imu::writes=0;boot();assert(!homeWifi.valid());
  runNetwork(65000000);
  assert(benchApActive&&fake_runtime::mode==WIFI_AP_STA&&fake_runtime::autoReconnect&&!fake_runtime::persistent);
  assert(fake_runtime::begins==0&&fake_runtime::reconnects==0&&fake_runtime::serverBegins==1);
  assert(fake_runtime::serialOut.find("wifi_cfg=1")!=std::string::npos);
  acceptLine("GET",1);assert(fake_runtime::serialOut.find("EVT:WIFI2,192.168.4.1,0.0.0.0,6,EMPTY")!=std::string::npos);

  boot();fake_imu::nvs["balancebot/tuning-v1"]={1,2,3};fake_imu::nvs["imu-cal-v1/gyro"]={4,5,6};
  const auto tuning=fake_imu::nvs["balancebot/tuning-v1"],gyro=fake_imu::nvs["imu-cal-v1/gyro"];
  assert(command("WIFI_SAVE")=="ERR:WIFI_SEQUENCE");stage();assert(fake_imu::writes==0);
  assert(command("WIFI_SAVE",1,false).empty());assert(wifiRestart&&fake_imu::writes==1);
  const auto stored=fake_imu::nvs["balancebot/wifi-v1"];
  // Save ACK must reach Serial before the delayed restart; password must not be emitted.
  runNetwork(1000000);assert(fake_runtime::restarts==1);
  assert(fake_runtime::serialOut.find("ACK:WIFI_SAVED")!=std::string::npos);
  assert(fake_runtime::serialOut.find("password123")==std::string::npos);
  assert(fake_runtime::serialOut.find("70617373776f7264313233")==std::string::npos);
  assert(command("START")=="ERR:WIFI_RESTART_PENDING");
  runControl(6000000);assert(!device.enabled()&&fake_runtime::motorEnables==0);
  assert(fake_imu::nvs["balancebot/tuning-v1"]==tuning&&fake_imu::nvs["imu-cal-v1/gyro"]==gyro);

  boot();assert(homeWifi.valid()&&!std::strcmp(homeWifi.ssid,"Home"));
  fake_runtime::status=WL_CONNECTED;fake_runtime::tcpCandidate=true;runNetwork(100000);
  assert(benchApActive&&client.connected()&&fake_runtime::begins==1);
  acceptLine("GET",2);assert(fake_runtime::tcpOut.find("EVT:WIFI2,192.168.4.1,192.168.32.50,3,CONFIGURED")!=std::string::npos);
  runControl();Reply getReply{};assert(xQueueReceive(replies,&getReply,0)==pdTRUE);
  assert(std::strstr(getReply.line,"PID:V,")!=nullptr);
  assert(command("WIFI_SAVE")=="ERR:WIFI_SEQUENCE"); // In-flight staging never persists across boot.
  boot();fake_runtime::status=4;runNetwork(65000000);acceptLine("GET",1);
  assert(benchApActive&&fake_runtime::begins==1&&fake_runtime::reconnects==0);
  assert(fake_runtime::serialOut.find("EVT:WIFI2,192.168.4.1,0.0.0.0,4,CONFIGURED")!=std::string::npos);
  boot();fake_runtime::apOk=false;fake_runtime::status=WL_CONNECTED;runNetwork(100000);acceptLine("GET",1);
  assert(!benchApActive&&fake_runtime::serverBegins==1);
  assert(fake_runtime::serialOut.find("EVT:WIFI2,0.0.0.0,192.168.32.50,3,CONFIGURED")!=std::string::npos);

  boot();stage();fake_imu::writeFailure=true;
  assert(command("WIFI_SAVE")=="ERR:STORAGE"&&!wifiRestart);
  assert(fake_imu::nvs["balancebot/wifi-v1"]==stored);
  boot();fake_runtime::calibrating=true;assert(command("WIFI_SSID,486f6d65")=="ERR:CALIBRATION_BUSY");
  boot();stage();fake_imu::clock+=10000001;assert(command("WIFI_SAVE")=="ERR:WIFI_SEQUENCE");
  boot();assert(command("WIFI_SSID,486f6d65")=="ACK:WIFI_SSID");
  assert(command("WIFI_PASS,70617373776f7264313233",2)=="ERR:OWNER");
  assert(command("WIFI_PASS,00")=="ERR:WIFI_PASSWORD");
  assert(command("WIFI_SSID,0A")=="ERR:WIFI_SSID");
  assert(command("WIFI_SSID,486f6d65")=="ACK:WIFI_SSID");assert(command("WIFI_PASS,")=="ACK:WIFI_PASS");
  // Boundary lengths are bytes (hex on wire); no truncated credential may be saved.
  std::string maxSsid="WIFI_SSID,";for(int i=0;i<32;++i)maxSsid+="41";
  assert(command(maxSsid.c_str())=="ACK:WIFI_SSID");maxSsid+="41";
  assert(command(maxSsid.c_str())=="ERR:WIFI_SSID");
  assert(command("WIFI_SSID,486f6d65")=="ACK:WIFI_SSID");
  std::string maxPass="WIFI_PASS,";for(int i=0;i<63;++i)maxPass+="42";
  assert(command(maxPass.c_str())=="ACK:WIFI_PASS");maxPass+="42";
  assert(command(maxPass.c_str())=="ERR:WIFI_PASSWORD");
  const auto writesBefore=fake_imu::writes;
  boot();assert(command("START")=="ACK:START");
  assert(command("WIFI_SSID,486f6d65")=="ERR:STATE");runControl(1100000);assert(device.enabled());
  assert(command("WIFI_SSID,486f6d65")=="ERR:STATE");device.fault("FALL");
  assert(command("WIFI_SSID,486f6d65")=="ERR:STATE"&&fake_imu::writes==writesBefore);
  // Saving must not mutate the old gyro/tuning records, even in error paths.
  assert(fake_imu::nvs["balancebot/tuning-v1"]==tuning&&fake_imu::nvs["imu-cal-v1/gyro"]==gyro);
  std::cout<<"PASS: real Runtime Wi-Fi staging/ACK/persistence/Idle guard/restart motor lock/AP+STA/IP reporting/SDK reconnect/no password echo; no hardware\n";
}
