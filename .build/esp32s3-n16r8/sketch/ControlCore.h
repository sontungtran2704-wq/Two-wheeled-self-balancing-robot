#line 1 "D:\\blancing\\Robot_Collab\\03_Firmware_ESP32S3\\balancebot\\ControlCore.h"
#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include "GeneratedConfig.h"
namespace robot {
enum class State : uint8_t { Idle, Arming, Run, Fault };
struct Tuning { double vkp,vki,vkd,ykp,yki,ykd,zero; };
struct Sensors { double theta=0,omega=0,v=0,yaw=0; uint32_t timestamp_us=0; bool valid=false; };
struct Output { double u=0,alpha=0,left=0,right=0,theta_ref=0,aref=0,vref=0,wref=0; bool saturated=false,motion_active=false; double ub=0,um=0; };
struct PID { double kp,ki,kd,i=0,d=0,prev=0; bool ready=false; void reset(); double update(double ref,double y,double dt,double limit,bool saturated); };
class NumericController {
 public:
  PID velocity{cfg::velocity_kp,cfg::velocity_ki,cfg::velocity_kd},yaw{cfg::yaw_kp,cfg::yaw_ki,cfg::yaw_kd};
  Output tick(double theta,double omega,double v,double yaw_rate,double vtarget,double wtarget,bool motion,bool fresh);
  void reset();
  double k0=cfg::k0,k1=cfg::k1;
 private:
  double q_=0,d_=0,vref_=0,wref_=0,aref_=0,yacc_=0,stable_=0;
  uint32_t n_=0; bool sat_=false,active_=false,stopping_=false;
};
class Device {
 public:
  explicit Device(uint32_t session_seed=1):session_(session_seed ? session_seed : 1) {}
  void tick(uint32_t now,const Sensors& sensors);
  std::string command(const char* line,uint32_t now);
  std::string telemetry(uint32_t now) const;
  void commandTo(const char* line,uint32_t now,char* out,size_t capacity);
  void telemetryTo(uint32_t now,char* out,size_t capacity) const;
  void fault(const char* reason);
  State state() const {return state_;}
  bool enabled() const {return state_==State::Run;}
  uint32_t session() const {return session_;}
  const Output& output() const {return output_;}
  bool commandFresh(uint32_t now) const {return leased_ && uint32_t(now-last_command_)<=cfg::lease_us;}
  const char* faultReason() const {return fault_;}
  uint32_t overruns() const {return overruns_;}
  Tuning tuning() const;
  bool restoreTuning(const Tuning& t);
 private:
  NumericController control_; Output output_; Sensors sensors_;
  State state_=State::Idle;
  uint32_t last_tick_=0,arm_since_=0,last_command_=0,session_,seq_=0,saturation_us_=0,overruns_=0;
  int32_t jitter_us_=0;
  bool ticked_=false,arm_timing_=false,motion_=false,leased_=false,have_seq_=false;
  double target_v_=0,target_w_=0;
  double pitchZero_=0;
  const char* fault_="NONE";
  bool sensorsFresh(uint32_t now) const;
  void clearMotion();
};
}
