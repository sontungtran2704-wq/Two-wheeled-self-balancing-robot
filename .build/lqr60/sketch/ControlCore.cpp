#line 1 "D:\\blancing\\Robot_Collab\\03_Firmware_ESP32S3\\balancebot_lqr_60_test\\ControlCore.cpp"
#include "ControlCore.h"
#include "TestSafetyConfig.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <limits>
#include <algorithm>
namespace robot {
namespace {
double clip(double v,double lo,double hi) {return v<lo?lo:(v>hi?hi:v);}
double ramp(double v,double t,double step) {return v+clip(t-v,-step,step);}
bool number(const char* s,double& v) {
  if(!s || !*s || *s==' ' || *s=='\t') return false;
  errno=0;char* end=nullptr;v=std::strtod(s,&end);
  return end!=s && *end=='\0' && errno!=ERANGE && std::isfinite(v);
}
bool integer(const char* s,uint32_t& v) {
  if(!s || !*s) return false;
  uint64_t z=0;
  for(;*s;++s) {if(*s<'0'||*s>'9') return false; z=z*10+(*s-'0');if(z>UINT32_MAX)return false;}
  v=uint32_t(z);return true;
}
}
void PID::reset() {i=d=prev=0;ready=false;}
double PID::update(double ref,double y,double dt,double limit,bool saturated) {
  const double e=ref-y,raw=ready?(y-prev)/dt:0;
  ready=true;prev=y;d+=dt/(cfg::derivative_tau_s+dt)*(raw-d);
  const double candidate=clip(i+e*dt,-1,1),z=kp*e+ki*candidate-kd*d;
  if(!saturated && (std::abs(z)<=limit || z*e<0))i=candidate;
  return clip(kp*e+ki*i-kd*d,-limit,limit);
}
void NumericController::reset() {
  q_=d_=vref_=wref_=aref_=yacc_=stable_=0;n_=0;
  sat_=active_=stopping_=false;velocity.reset();yaw.reset();
}
Output NumericController::tick(double theta,double omega,double v,double yr,double vt,double wt,bool motion,bool fresh) {
  // Dedicated bench image: outer loops cannot be enabled by any caller.
  motion=false;vt=wt=0;
  if(std::abs(theta)<cfg::stable_rad && std::abs(omega)<cfg::stable_rate)stable_+=cfg::dt;
  else stable_=0;
  if(motion && stable_>=1.) {active_=true;stopping_=false;}
  if(!motion && active_)stopping_=true;
  vt=fresh&&motion?clip(vt,-cfg::velocity_max,cfg::velocity_max):0;
  wt=fresh&&motion?clip(wt,-cfg::yaw_rate_max,cfg::yaw_rate_max):0;
  if(n_%4==0) {
    vref_=ramp(vref_,active_?vt:0,cfg::velocity_slew*cfg::pid_dt);
    wref_=ramp(wref_,active_?wt:0,cfg::yaw_slew*cfg::pid_dt);
    const double desired=active_?velocity.update(vref_,v,cfg::pid_dt,cfg::accel_max,sat_):0;
    yacc_=active_?yaw.update(wref_,yr,cfg::pid_dt,cfg::yaw_accel_max,sat_):0;
    aref_+=cfg::pid_dt/(cfg::accel_filter_s+cfg::pid_dt)*(desired-aref_);
    if(stopping_ && std::abs(v)<.01 && std::abs(yr)<.03 && std::abs(aref_)<.01) {
      active_=stopping_=false;velocity.reset();yaw.reset();
    }
  }
  const double tr=std::atan2(aref_,cfg::g)+std::asin(clip(cfg::r*cfg::M*aref_/(cfg::mb*cfg::l*std::hypot(cfg::g,aref_)),-1,1));
  const double ub=-cfg::k0*theta-cfg::k1*omega,um=cfg::k0*tr+aref_;
  const double bal=clip(ub,-cfg::amax,cfg::amax),u=bal+clip(um,-cfg::amax-bal,cfg::amax-bal);
  const double L=q_-d_,R=q_+d_,step=cfg::amax*cfg::dt;
  const double Llo=std::max(-cfg::vmax,L-step),Lhi=std::min(cfg::vmax,L+step);
  const double Rlo=std::max(-cfg::vmax,R-step),Rhi=std::min(cfg::vmax,R+step);
  const double qnew=clip(q_+cfg::dt*u,(Llo+Rlo)/2,(Lhi+Rhi)/2),ua=(qnew-q_)/cfg::dt;
  const double dnew=clip(d_+cfg::dt*cfg::track/2*yacc_,std::max(qnew-Lhi,Rlo-qnew),std::min(qnew-Llo,Rhi-qnew)),daa=(dnew-d_)/cfg::dt;
  sat_=std::abs(u-ub-um)>1e-9 || std::abs(ua-u)>1e-9 || std::abs(daa-cfg::track/2*yacc_)>1e-9;
  q_=qnew;d_=dnew;++n_;
  return {ua,2*daa/cfg::track,q_-d_,q_+d_,tr,aref_,vref_,wref_,sat_,active_,ub,um};
}
bool Device::sensorsFresh(uint32_t now) const {
  return sensors_.valid && uint32_t(now-sensors_.timestamp_us)<=cfg::imu_max_age_us &&
    std::isfinite(sensors_.theta)&&std::isfinite(sensors_.omega)&&std::isfinite(sensors_.v)&&std::isfinite(sensors_.yaw);
}
void Device::clearMotion() {
  target_v_=target_w_=0;motion_=leased_=have_seq_=false;seq_=0;
  output_={};control_.reset();arm_timing_=false;saturation_us_=0;
}
void Device::fault(const char* reason) {state_=State::Fault;fault_=reason;clearMotion();}
Tuning Device::tuning() const {
  return {control_.velocity.kp,control_.velocity.ki,control_.velocity.kd,
          control_.yaw.kp,control_.yaw.ki,control_.yaw.kd,pitchZero_};
}
bool Device::restoreTuning(const Tuning& t) {
  if(state_==State::Run || state_==State::Arming)return false;
  const double values[]={t.vkp,t.vki,t.vkd,t.ykp,t.yki,t.ykd,t.zero};
  for(double v:values)if(!std::isfinite(v))return false;
  if(t.vkp<0||t.vkp>5||t.vki<0||t.vki>2||t.vkd<0||t.vkd>.2||
     t.ykp<0||t.ykp>5||t.yki<0||t.yki>2||t.ykd<0||t.ykd>.2||std::abs(t.zero)>=cfg::arm_rad)return false;
  control_.velocity={t.vkp,t.vki,t.vkd};control_.yaw={t.ykp,t.yki,t.ykd};
  sensors_.theta+=pitchZero_-t.zero;pitchZero_=t.zero;clearMotion();return true;
}
void Device::tick(uint32_t now,const Sensors& s) {
  sensors_=s;
  sensors_.theta-=pitchZero_;
  const uint32_t interval=now-last_tick_;
  if(ticked_) {
    jitter_us_=int32_t(interval)-5000;
    if(interval>5500)++overruns_;
    if((state_==State::Run || state_==State::Arming) && interval>15000) fault("DEADLINE");
  }
  last_tick_=now;ticked_=true;
  if(state_==State::Idle || state_==State::Fault)return;
  if(!sensorsFresh(now)) {fault("IMU");return;}
  if(std::abs(sensors_.theta)>test_safety::fall_rad) {fault("FALL");return;}
  if(state_==State::Arming) {
    if(std::abs(s.omega)<cfg::arm_rate) {
      if(!arm_timing_) {arm_timing_=true;arm_since_=now;}
      if(uint32_t(now-arm_since_)>=1000000) {clearMotion();state_=State::Run;}
    } else arm_timing_=false;
    return;
  }
  output_=control_.tick(sensors_.theta,s.omega,s.v,s.yaw,target_v_,target_w_,motion_,commandFresh(now));
  // A command may legitimately saturate briefly; prolonged speed rail means balance authority is gone.
  if(std::abs((output_.left+output_.right)/2)>=cfg::vmax-1e-9)saturation_us_+=5000;
  else saturation_us_=0;
  if(saturation_us_>=300000)fault("SPEED_RAIL");
}
void Device::commandTo(const char* line,uint32_t now,char* out,size_t capacity) {
  const auto reply=[&](const char* s){std::snprintf(out,capacity,"%s",s);};
  if(!line || std::strlen(line)>160) {reply("ERR:LINE_TOO_LONG");return;}
  char buf[161];std::strcpy(buf,line);char* tok[8]={buf};size_t count=1;
  for(char* p=buf;*p;++p) {
    if(*p=='\n'||*p=='\r'||static_cast<unsigned char>(*p)<32) {reply("ERR:FORMAT");return;}
    if(*p==',') {if(count>=8) {reply("ERR:FORMAT");return;}*p=0;tok[count++]=p+1;}
  }
  for(size_t i=0;i<count;++i)if(!*tok[i]){reply("ERR:FORMAT");return;}
  if(count==1 && (!std::strcmp(line,"ZPHERE") || !std::strcmp(line,"ZP+") || !std::strcmp(line,"ZP-"))) {
    if(state_!=State::Idle){reply("ERR:STATE");return;}
    if(!sensorsFresh(now)){reply("ERR:IMU");return;}
    const double rawPitch=sensors_.theta+pitchZero_;
    if(std::abs(rawPitch)>=cfg::arm_rad || std::abs(sensors_.omega)>=0.02){
      reply("ERR:POSE");return;
    }
    constexpr double tenthDegree=0.00174532925199433;
    double nextZero=rawPitch;
    if(!std::strcmp(line,"ZP+"))nextZero=pitchZero_+tenthDegree;
    if(!std::strcmp(line,"ZP-"))nextZero=pitchZero_-tenthDegree;
    if(std::abs(nextZero)>=cfg::arm_rad){reply("ERR:POSE");return;}
    pitchZero_=nextZero;
    sensors_.theta=rawPitch-pitchZero_;
    std::snprintf(out,capacity,"ACK:%s,zero_rad=%.6f",line,pitchZero_);
    return;
  }
  if(!std::strcmp(tok[0],"$K")) {reply("ERR:READ_ONLY_GAIN");return;}
  if(!std::strcmp(tok[0],"GET") && count==1) {
    char status[900];telemetryTo(now,status,sizeof status);
    std::snprintf(out,capacity,"%s\nPID:V,%.9g,%.9g,%.9g\nPID:Y,%.9g,%.9g,%.9g",status,control_.velocity.kp,control_.velocity.ki,control_.velocity.kd,control_.yaw.kp,control_.yaw.ki,control_.yaw.kd);return;
  }
  if(count==1 && !std::strcmp(tok[0],"ESTOP")) {fault("ESTOP");reply("ACK:ESTOP");return;}
  if(count==1 && !std::strcmp(tok[0],"STOP")) {if(state_!=State::Fault)state_=State::Idle;clearMotion();reply("ACK:STOP");return;}
  if(count==1 && !std::strcmp(tok[0],"RESET")) {
    if(state_==State::Run || state_==State::Arming) {reply("ERR:MOTORS_ON");return;}
    state_=State::Idle;fault_="NONE";clearMotion();reply("ACK:RESET");return;
  }
  if(!std::strcmp(tok[0],"MOTION") || !std::strcmp(tok[0],"SPEED") ||
     !std::strcmp(tok[0],"TURN") || !std::strcmp(tok[0],"CMD") || !std::strcmp(tok[0],"$P")) {
    reply("ERR:LQR_ONLY");return;
  }
  if(count==1 && !std::strcmp(tok[0],"START")) {
    if(state_==State::Fault) {reply("ERR:RESET_REQUIRED");return;}
    if(state_!=State::Idle) {reply("ERR:STATE");return;}
    if(!sensorsFresh(now)) {reply("ERR:IMU");return;}
    if(std::abs(sensors_.theta)>test_safety::fall_rad || std::abs(sensors_.omega)>=cfg::arm_rate) {reply("ERR:POSE");return;}
    clearMotion();++session_;if(!session_)++session_;state_=State::Arming;
    arm_timing_=true;arm_since_=now;reply("ACK:START");return;
  }
  if(!std::strcmp(tok[0],"$P") && count==5) {
    if(state_==State::Run||state_==State::Arming) {reply("ERR:MOTORS_ON");return;}
    double kp,ki,kd;
    if(!number(tok[2],kp)||!number(tok[3],ki)||!number(tok[4],kd)||kp<0||kp>5||ki<0||ki>2||kd<0||kd>.2) {reply("ERR:GAIN_RANGE");return;}
    PID* pid=!std::strcmp(tok[1],"V")?&control_.velocity:(!std::strcmp(tok[1],"Y")?&control_.yaw:nullptr);
    if(!pid) {reply("ERR:AXIS");return;}pid->kp=kp;pid->ki=ki;pid->kd=kd;pid->reset();reply("ACK:$P");return;
  }
  if(!std::strcmp(tok[0],"MOTION") && count==2) {
    if(std::strcmp(tok[1],"0") && std::strcmp(tok[1],"1")) {reply("ERR:FORMAT");return;}
    if(state_!=State::Run) {reply("ERR:STATE");return;}
    motion_=tok[1][0]=='1';if(!motion_) {target_v_=target_w_=0;leased_=false;}
    reply("ACK:MOTION");return;
  }
  if((!std::strcmp(tok[0],"SPEED")||!std::strcmp(tok[0],"TURN")) && count==2) {
    double value;const bool speed=!std::strcmp(tok[0],"SPEED");
    if(!number(tok[1],value)||std::abs(value)>(speed?20.:45.83662361046586)) {reply("ERR:RANGE");return;}
    if(state_!=State::Run) {reply("ERR:STATE");return;}
    if(speed)target_v_=value/100.;else target_w_=value*3.14159265358979323846/180.;
    last_command_=now;leased_=true;reply(speed?"ACK:SPEED":"ACK:TURN");return;
  }
  if(!std::strcmp(tok[0],"CMD") && count==5) {
    uint32_t session,sequence;double v,w;
    if(!integer(tok[1],session)||!integer(tok[2],sequence)||!number(tok[3],v)||!number(tok[4],w)) {reply("ERR:FORMAT");return;}
    if(session!=session_) {reply("ERR:SESSION");return;}
    if(state_!=State::Run) {reply("ERR:STATE");return;}
    if(have_seq_ && (sequence==seq_ || uint32_t(sequence-seq_)>=0x80000000u)) {reply("ERR:SEQUENCE");return;}
    if(std::abs(v)>cfg::velocity_max||std::abs(w)>cfg::yaw_rate_max) {reply("ERR:RANGE");return;}
    seq_=sequence;have_seq_=true;target_v_=v;target_w_=w;last_command_=now;leased_=true;reply("ACK:CMD");return;
  }
  reply("ERR:FORMAT");
}
std::string Device::command(const char* line,uint32_t now) {char out[1200];commandTo(line,now,out,sizeof out);return out;}
void Device::telemetryTo(uint32_t now,char* out,size_t capacity) const {
  std::snprintf(out,capacity,"t=%lu,theta=%.9g,omega=%.9g,v=%.9g,yaw=%.9g,left=%.9g,right=%.9g,u=%.9g,st=%u,motion=%u,session=%lu,seq=%lu,config=%s,hash=%s,fresh=%u,sat=%u,jitter_us=%ld,overruns=%lu,fault=%s",
    static_cast<unsigned long>(now/1000),sensors_.theta,sensors_.omega,sensors_.v,sensors_.yaw,output_.left,output_.right,output_.u,unsigned(state_),unsigned(output_.motion_active),static_cast<unsigned long>(session_),static_cast<unsigned long>(last_tick_/5000),cfg::id,cfg::sha256,unsigned(commandFresh(now)),unsigned(output_.saturated),static_cast<long>(jitter_us_),static_cast<unsigned long>(overruns_),fault_);
}
std::string Device::telemetry(uint32_t now) const {char out[900];telemetryTo(now,out,sizeof out);return out;}
}
