#line 1 "D:\\TLHT\\Two-wheeled-self-balancing-robot\\03_Firmware_ESP32S3\\balancebot\\Runtime.cpp"
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include <atomic>
#include "ControlCore.h"
#include "StepperHal.h"
#include "ImuHal.h"
#include "HardwareConfig.h"
#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "This HAL is for ESP32-S3 only"
#endif
namespace {
struct Request {char line[161];uint32_t received;uint8_t source;};
struct Reply {char line[1200];uint8_t source;};
static QueueHandle_t requests=nullptr,replies=nullptr;
static portMUX_TYPE sharedLock=portMUX_INITIALIZER_UNLOCKED;
static robot::Device device,view;
static volatile uint8_t stopRequest=0;
static uint32_t maxTickUs=0;
static std::atomic<uint32_t> queueDrops{0};
static WiFiServer server(8080);
static WiFiClient client;
#ifndef ROBOT_BENCH_AP_PASSWORD
#define ROBOT_BENCH_AP_PASSWORD "BalanceBot13"
#endif
static constexpr char benchPassword[]=ROBOT_BENCH_AP_PASSWORD;
static_assert(sizeof benchPassword>=9 && sizeof benchPassword<=64,"AP password must be 8–63 characters");
static bool benchApActive=false;
static uint8_t transportOwner=0;
struct StoredTuning { char hash[65]; robot::Tuning tuning; };
static bool saveTuning() {
  Preferences prefs;if(!prefs.begin("balancebot",false))return false;
  StoredTuning value{};std::memcpy(value.hash,cfg::sha256,65);value.tuning=device.tuning();
  const bool ok=prefs.putBytes("tuning-v1",&value,sizeof value)==sizeof value;
  prefs.end();return ok;
}
static void loadTuning() {
  Preferences prefs;if(!prefs.begin("balancebot",true))return;
  StoredTuning value{};
  if(prefs.getBytesLength("tuning-v1")==sizeof value &&
     prefs.getBytes("tuning-v1",&value,sizeof value)==sizeof value &&
     std::memcmp(value.hash,cfg::sha256,65)==0)device.restoreTuning(value.tuning);
  prefs.end();
}
static void setStop(bool emergency) {
  portENTER_CRITICAL(&sharedLock);if(emergency||!stopRequest)stopRequest=emergency?2:1;portEXIT_CRITICAL(&sharedLock);
}
static robot::Device copyView() {
  portENTER_CRITICAL(&sharedLock);const auto v=view;portEXIT_CRITICAL(&sharedLock);return v;
}
static void controlTask(void*) {
  TickType_t wake=xTaskGetTickCount();int64_t previousL=0,previousR=0;
  for(;;) {
    auto sensors=imu::snapshot();
    const uint32_t start=micros();
    int64_t L,R;bool watchdog;stepper::snapshot(L,R,watchdog);
    // Delivered pulses, corrected for body pitch rotation. This cannot detect slip/missed steps.
    sensors.v=double((L-previousL)+(R-previousR))/(2*cfg::steps_per_m*cfg::dt)+cfg::r*sensors.omega;
    previousL=L;previousR=R;
    portENTER_CRITICAL(&sharedLock);const uint8_t stop=stopRequest;stopRequest=0;portEXIT_CRITICAL(&sharedLock);
    if(stop) {
      char ignored[48];device.commandTo(stop==2?"ESTOP":"STOP",start,ignored,sizeof ignored);xQueueReset(requests);
    }
    if(watchdog)device.fault("STEP_WATCHDOG");
    device.tick(start,sensors);
    Request request;
    if(xQueueReceive(requests,&request,0)==pdTRUE) {
      Reply reply{};reply.source=request.source;
      const uint32_t dispatch=micros();
      if(uint32_t(dispatch-request.received)>100000)std::strcpy(reply.line,"ERR:STALE_QUEUE");
      else if(!hardware::confirmed && !std::strcmp(request.line,"START"))std::strcpy(reply.line,"ERR:HARDWARE_UNCONFIRMED");
      else if(!std::strcmp(request.line,"SAVE")) {
        // Only this task can arm. No flash write can overlap an armed state.
        if(device.state()==robot::State::Run || device.state()==robot::State::Arming)
          std::strcpy(reply.line,"ERR:MOTORS_ON");
        else {
          stepper::apply(0,0,false);
          std::strcpy(reply.line,saveTuning()?"ACK:SAVED":"ERR:STORAGE");
          wake=xTaskGetTickCount();
        }
      }
      else device.commandTo(request.line,dispatch,reply.line,sizeof reply.line);
      if(xQueueSend(replies,&reply,0)!=pdTRUE)++queueDrops;
    }
    const auto o=device.output();stepper::apply(o.left,o.right,device.enabled());
    const uint32_t execution=micros()-start;
    portENTER_CRITICAL(&sharedLock);view=device;if(execution>maxTickUs)maxTickUs=execution;portEXIT_CRITICAL(&sharedLock);
    vTaskDelayUntil(&wake,pdMS_TO_TICKS(5));
  }
}
static void emit(uint8_t source,const char* text) {
  const size_t n=std::strlen(text);
  if(source==1 || source==0) {Serial.write(reinterpret_cast<const uint8_t*>(text),n);Serial.write('\n');}
  if((source==2 || source==0) && client.connected()) {
    char line[1400];const int len=snprintf(line,sizeof line,"%s\n",text);
    const int sent=send(client.fd(),line,len,MSG_DONTWAIT);
    // Drop connection on partial line so next connection never parses a truncated record.
    if(sent!=len)client.stop();
  }
}
static void acceptLine(const char* line,uint8_t source) {
  if(!std::strcmp(line,"GET")) {
    char out[120];snprintf(out,sizeof out,"EVT:WIFI,%s,%s",benchApActive?"AP":"STA",
        (benchApActive?WiFi.softAPIP():WiFi.localIP()).toString().c_str());emit(source,out);
    // Queue GET behind preceding PID changes so the reply contains new values.
  }
  if(!std::strcmp(line,"STOP") || !std::strcmp(line,"ESTOP")) {
    const bool emergency=!std::strcmp(line,"ESTOP");setStop(emergency);transportOwner=0;
    emit(source,emergency?"ACK:ESTOP_QUEUED":"ACK:STOP_QUEUED");return;
  }
  if(!std::strcmp(line,"CALGYRO")) {
    auto v=copyView();
    if(v.state()==robot::State::Run||v.state()==robot::State::Arming)emit(source,"ERR:MOTORS_ON");
    else if(!hardware::pinsConfigured)emit(source,"ERR:PINMAP_UNCONFIRMED");
    else {imu::requestCalibration();emit(source,"ACK:CALGYRO_PENDING");}return;
  }
  if(transportOwner && source!=transportOwner && std::strcmp(line,"GET")) {emit(source,"ERR:OWNER");return;}
  if(!std::strcmp(line,"START"))transportOwner=source;
  Request r{};std::strncpy(r.line,line,160);r.received=micros();r.source=source;
  if(xQueueSend(requests,&r,0)!=pdTRUE) {++queueDrops;emit(source,"ERR:BUSY");}
}
struct LineReader {
  char line[161]{};size_t length=0;bool overflow=false;
  void clear(){length=0;overflow=false;}
  void consume(char c,uint8_t source) {
    if(c=='\r')return;
    if(c=='\n') {
      if(overflow)emit(source,"ERR:LINE_TOO_LONG");
      else if(length) {line[length]=0;acceptLine(line,source);}
      clear();return;
    }
    if(length==160) {overflow=true;return;}if(!overflow)line[length++]=c;
  }
};
static void networkTask(void*) {
  LineReader serialReader,tcpReader;uint32_t lastTelemetry=0,lastReconnect=0;bool serving=false;
  if(*hardware::ssid) {WiFi.mode(WIFI_STA);WiFi.begin(hardware::ssid,hardware::password);}
  else {
    WiFi.mode(WIFI_AP);
    if(WiFi.softAP("BalanceBot-Bench",benchPassword)) {
      server.begin();
      serving=true;
      benchApActive=true;
    }
  }
  for(;;) {
    const uint32_t now=millis();
    if((*hardware::ssid && WiFi.status()==WL_CONNECTED) || benchApActive) {
      if(!serving) {server.begin();serving=true;}
      if(server.hasClient()) {
        auto candidate=server.accept();
        if(client.connected())candidate.stop();else {client=candidate;client.setNoDelay(true);tcpReader.clear();}
      }
    } else if(*hardware::ssid && uint32_t(now-lastReconnect)>10000) {
      lastReconnect=now;WiFi.reconnect();
    }
    for(int n=0;n<192 && Serial.available();++n)serialReader.consume(char(Serial.read()),1);
    for(int n=0;n<192 && client.connected() && client.available();++n)tcpReader.consume(char(client.read()),2);
    Reply reply;for(int n=0;n<4 && xQueueReceive(replies,&reply,0)==pdTRUE;++n)emit(reply.source,reply.line);
    if(uint32_t(now-lastTelemetry)>=40) {
      lastTelemetry=now;char out[1100];auto v=copyView();v.telemetryTo(micros(),out,sizeof out);
      const size_t len=std::strlen(out);uint32_t duration;
      portENTER_CRITICAL(&sharedLock);duration=maxTickUs;portEXIT_CRITICAL(&sharedLock);
      snprintf(out+len,sizeof out-len,",tick_max_us=%lu,queue_drops=%lu,imu_calibrated=%u,imu_diag=%u,imu_who=%u,cal_accel_mg=%u,cal_accel_std_mg=%u,cal_gyro_std_mrad=%u,imu_sample_fresh=%u,hardware_confirmed=%u",static_cast<unsigned long>(duration),static_cast<unsigned long>(queueDrops.load()),unsigned(imu::calibrated()),imu::diagnostic(),imu::identity(),imu::calibrationAccelMilliG(),imu::calibrationAccelStdMg(),imu::calibrationGyroStdMrad(),unsigned(imu::sampleFresh()),unsigned(hardware::confirmed));emit(0,out);
    }
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}
}
void setupRobot() {
  Serial.begin(115200);
  requests=xQueueCreate(8,sizeof(Request));replies=xQueueCreate(4,sizeof(Reply));
  device=robot::Device(esp_random());view=device;
  const bool stepperReady=stepper::begin();
  if(!requests||!replies||!stepperReady) {device.fault("INITIALIZATION");view=device;return;}
  loadTuning();view=device;
  if(xTaskCreatePinnedToCore(imu::task,"imu",4096,nullptr,4,nullptr,1)!=pdPASS ||
     xTaskCreatePinnedToCore(controlTask,"balance",6144,nullptr,5,nullptr,1)!=pdPASS ||
     xTaskCreatePinnedToCore(networkTask,"link",8192,nullptr,1,nullptr,0)!=pdPASS) {
    setStop(true);stepper::apply(0,0,false);
  }
}
void loopRobot() {vTaskDelay(pdMS_TO_TICKS(1000));}



