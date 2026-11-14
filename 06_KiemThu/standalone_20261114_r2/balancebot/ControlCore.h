#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <cstring>
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
  // Runtime enables this only after hardware initialization/confirmation.
  void enableAutonomous(bool allowed) {auto_feature_=allowed;auto_blocked_=!allowed;auto_timing_=false;}
  void suspendAutonomous() {auto_blocked_=true;auto_timing_=false;}
  // Configuration/calibration interrupts the dwell, but must not clear an explicit STOP.
  void restartAutonomousCountdown() {auto_timing_=false;}
  bool autonomousWaiting() const {return auto_feature_&&!auto_blocked_&&(state_==State::Idle||
    (state_==State::Fault&&(!std::strcmp(fault_,"FALL")||!std::strcmp(fault_,"SPEED_RAIL"))));}
  uint32_t autonomousElapsed(uint32_t now) const {return auto_timing_?uint32_t(now-auto_since_):0;}
 private:
  NumericController control_; Output output_; Sensors sensors_;
  State state_=State::Idle;
  uint32_t last_tick_=0,arm_since_=0,last_command_=0,session_,seq_=0,saturation_us_=0,overruns_=0;
  int32_t jitter_us_=0;
  bool ticked_=false,arm_timing_=false,motion_=false,leased_=false,have_seq_=false;
  bool auto_feature_=false,auto_blocked_=true,auto_timing_=false;
  uint32_t auto_since_=0;
  double target_v_=0,target_w_=0;
  double pitchZero_=0;
  const char* fault_="NONE";
  bool sensorsFresh(uint32_t now) const;
  void clearMotion();
  void automaticStart(uint32_t now, bool continuous);
};
}
