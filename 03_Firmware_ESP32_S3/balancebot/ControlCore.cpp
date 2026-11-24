// Biến mẫu pitch, vận tốc ước lượng và mục tiêu tiến/quay thành hai lệnh bánh qua PID ngoài và LQR trong.
#include "ControlCore.h"
#include "GeneratedLqrModel.h"
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
// Lấy đạo hàm trên đại lượng phản hồi rồi lọc để bước nhảy mục tiêu ít tác động lên nhánh D.
// Giới hạn tích phân và chỉ nhận giá trị mới khi còn biên điều khiển hoặc sai số giúp kéo đầu ra khỏi bão hòa.
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
// GeneratedLqrModel.h cung cấp Ad và Bd của mô hình rời rạc hai trạng thái ở 5 ms.
// Thử ma trận kín Ad − Bd*K trước khi nhận K mới. Phép thử chỉ xét mô hình pitch tuyến tính.
bool NumericController::setGains(double k0,double k1) {
  if(!std::isfinite(k0)||!std::isfinite(k1)||std::abs(k0)>10000||std::abs(k1)>10000)return false;
  const double a=lqr_model::a00-lqr_model::b0*k0,b=lqr_model::a01-lqr_model::b0*k1;
  const double c=lqr_model::a10-lqr_model::b1*k0,d=lqr_model::a11-lqr_model::b1*k1;
  const double trace=a+d,det=a*d-b*c;
  // Strict Jury test for this exact verified discrete two-state model.
  if(!(1-det>1e-9 && 1-trace+det>1e-9 && 1+trace+det>1e-9))return false;
  k0_=k0;k1_=k1;reset();return true;
}
Output NumericController::tick(double theta,double omega,double v,double yr,double vt,double wt,bool motion,bool fresh) {
  // Mở mục tiêu tiến/quay sau 1 s liên tục với pitch dưới 3° và tốc độ pitch dưới 0,15 rad/s.
  if(std::abs(theta)<cfg::stable_rad && std::abs(omega)<cfg::stable_rate)stable_+=cfg::dt;
  else stable_=0;
  if(motion && stable_>=1.) {active_=true;stopping_=false;}
  if(!motion && active_)stopping_=true;
  // Khi hết hạn lệnh hoặc khóa chuyển động, mục tiêu tiến và quay về 0. Vòng cân bằng vẫn chạy.
  // Giới hạn lệnh tiến/lùi ở ±0,20 m/s và lệnh quay ở ±0,80 rad/s.
  vt=fresh&&motion?clip(vt,-cfg::velocity_max,cfg::velocity_max):0;
  wt=fresh&&motion?clip(wt,-cfg::yaw_rate_max,cfg::yaw_rate_max):0;
  // PID ngoài cập nhật mỗi 4 lượt LQR, tức 20 ms (50 Hz). Tăng giảm dần vận tốc đặt trước khi tính PID.
  // PID dọc cho gia tốc đặt đã lọc, giới hạn ±0,35 m/s². PID yaw cho gia tốc quay, giới hạn ±2 rad/s².
  if(n_%4==0) {
    vref_=ramp(vref_,active_?vt:0,cfg::velocity_slew*cfg::pid_dt);
    wref_=ramp(wref_,active_?wt:0,cfg::yaw_slew*cfg::pid_dt);
    // Run is already armed. Hold zero speed while the stability gate still blocks drive/yaw commands.
    const double desired=(motion||active_)?velocity.update(vref_,v,cfg::pid_dt,cfg::accel_max,sat_):0;
    yacc_=active_?yaw.update(wref_,yr,cfg::pid_dt,cfg::yaw_accel_max,sat_):0;
    aref_+=cfg::pid_dt/(cfg::accel_filter_s+cfg::pid_dt)*(desired-aref_);
    if(stopping_ && std::abs(v)<.01 && std::abs(yr)<.03 && std::abs(aref_)<.01) {
      active_=stopping_=false;velocity.reset();yaw.reset();
    }
  }
  // Tính góc nghiêng đặt từ gia tốc dọc theo mô hình, dùng g, bán kính r, khối lượng thân mb,
  // cánh tay đòn l và khối lượng hiệu dụng M. Chặn đối số asin để góc đặt luôn nằm trong miền tính.
  const double tr=std::atan2(aref_,cfg::g)+std::asin(clip(cfg::r*cfg::M*aref_/(cfg::mb*cfg::l*std::hypot(cfg::g,aref_)),-1,1));
  // Vector LQR theo thứ tự [theta (rad), omega (rad/s)]. ub = −k0*theta − k1*omega là gia tốc cân bằng.
  // um = k0*theta_ref + aref ghép góc đặt và gia tốc bù tiến/lùi. Hai hệ số mặc định mang dấu âm.
  const double ub=-k0_*theta-k1_*omega,um=k0_*tr+aref_;
  // Ưu tiên gia tốc cân bằng trong ±3 m/s², rồi cấp phần biên còn lại cho gia tốc di chuyển.
  const double bal=clip(ub,-cfg::amax,cfg::amax),u=bal+clip(um,-cfg::amax-bal,cfg::amax-bal);
  // q là tốc độ chung và d là nửa chênh lệch tốc độ (m/s). Bánh trái lấy q−d, bánh phải lấy q+d.
  // Lập biên cho từng bánh từ ±0,6 m/s và mức đổi tối đa 3 m/s² nhân 5 ms mỗi lượt.
  const double L=q_-d_,R=q_+d_,step=cfg::amax*cfg::dt;
  const double Llo=std::max(-cfg::vmax,L-step),Lhi=std::min(cfg::vmax,L+step);
  const double Rlo=std::max(-cfg::vmax,R-step),Rhi=std::min(cfg::vmax,R+step);
  // Tích phân gia tốc chung để chọn q mới trước. Sau đó dùng nửa khoảng cách hai bánh (track/2)
  // đổi gia tốc yaw thành gia tốc chênh lệch và chặn d sao cho cả hai bánh còn trong biên.
  const double qnew=clip(q_+cfg::dt*u,(Llo+Rlo)/2,(Lhi+Rhi)/2),ua=(qnew-q_)/cfg::dt;
  const double dnew=clip(d_+cfg::dt*cfg::track/2*yacc_,std::max(qnew-Lhi,Rlo-qnew),std::min(qnew-Llo,Rhi-qnew)),daa=(dnew-d_)/cfg::dt;
  // Ghi nhận phần gia tốc bị cắt để PID ngoài kìm tích phân ở lần cập nhật kế tiếp.
  sat_=std::abs(u-ub-um)>1e-9 || std::abs(ua-u)>1e-9 || std::abs(daa-cfg::track/2*yacc_)>1e-9;
  q_=qnew;d_=dnew;++n_;
  return {ua,2*daa/cfg::track,q_-d_,q_+d_,tr,aref_,vref_,wref_,sat_,active_,ub,um};
}
// Chỉ dùng mẫu có cờ hợp lệ, tuổi tối đa 15 ms và mọi tín hiệu đưa vào điều khiển đều hữu hạn.
bool Device::sensorsFresh(uint32_t now) const {
  return sensors_.valid && uint32_t(now-sensors_.timestamp_us)<=cfg::imu_max_age_us &&
    std::isfinite(sensors_.theta)&&std::isfinite(sensors_.omega)&&std::isfinite(sensors_.v)&&std::isfinite(sensors_.yaw);
}
void Device::clearMotion() {
  target_v_=target_w_=0;motion_=leased_=have_seq_=false;seq_=0;
  output_={};control_.reset();arm_timing_=false;saturation_us_=0;
}
void Device::fault(const char* reason) {state_=State::Fault;fault_=reason;clearMotion();auto_timing_=false;}
// Tự vào Run từ Idle hoặc phục hồi FALL/SPEED_RAIL sau 5 s liên tục với mẫu mới,
// pitch trước và sau trừ zero dưới 10°, tốc độ pitch/yaw dưới 0,15 rad/s và không bị khóa tự chạy.
void Device::automaticStart(uint32_t now,bool continuous) {
  if(!autonomousWaiting() || !continuous || !sensorsFresh(now) ||
     std::abs(sensors_.theta)>=cfg::arm_rad ||
     std::abs(sensors_.theta+pitchZero_)>=cfg::arm_rad ||
     std::abs(sensors_.omega)>=cfg::stable_rate || std::abs(sensors_.yaw)>=cfg::stable_rate) {
    auto_timing_=false;return;
  }
  if(!auto_timing_) {auto_timing_=true;auto_since_=now;return;}
  if(uint32_t(now-auto_since_)<uint32_t(cfg::auto_rearm_s*1000000))return;
  // Five continuous seconds already supplied the arming dwell. Never resume old commands.
  clearMotion();auto_timing_=false;fault_="NONE";
  ++session_;if(!session_)++session_;
  motion_=true;state_=State::Run;
}
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
  // Trừ điểm zero cân bằng (rad) sau khi nhận pitch từ IMU. Đây là hiệu chỉnh riêng với trim lắp và bias gyro.
  sensors_.theta-=pitchZero_;
  const uint32_t interval=now-last_tick_;
  if(ticked_) {
    jitter_us_=int32_t(interval)-5000;
    if(interval>5500)++overruns_;
    // Khoảng cách hai lượt vượt 15 ms trong Run hoặc Arming làm ngắt điều khiển vì vòng phản hồi bị trễ.
    if((state_==State::Run || state_==State::Arming) && interval>15000) fault("DEADLINE");
  }
  last_tick_=now;ticked_=true;
  if(state_==State::Idle || state_==State::Fault) {
    automaticStart(now,interval<=cfg::imu_max_age_us);
    return;
  }
  // Mẫu mất hiệu lực hoặc quá cũ khi đang chuẩn bị/chạy sẽ chốt lỗi IMU. Arming thủ công cần 1 s
  // liên tục với cả hai góc dưới 10° và tốc độ pitch dưới 0,30 rad/s.
  if(!sensorsFresh(now)) {fault("IMU");return;}
  if(state_==State::Arming) {
    if(std::abs(sensors_.theta)<cfg::arm_rad && std::abs(s.theta)<cfg::arm_rad && std::abs(s.omega)<cfg::arm_rate) {
      if(!arm_timing_) {arm_timing_=true;arm_since_=now;}
      if(uint32_t(now-arm_since_)>=1000000) {
        clearMotion();
        motion_=true; // Allow the outer PID to hold zero velocity after its stability gate.
        state_=State::Run;
      }
    } else arm_timing_=false;
    return;
  }
  // Dừng ở |pitch đã trừ zero| từ 40° trở lên.
  if(std::abs(sensors_.theta)>=cfg::fall_rad) {fault("FALL");return;}
  output_=control_.tick(sensors_.theta,s.omega,s.v,s.yaw,target_v_,target_w_,motion_,commandFresh(now));
  // A command may legitimately saturate briefly; prolonged speed rail means balance authority is gone.
  // SPEED_RAIL xét tốc độ lệnh trung bình của hai bánh chạm 0,6 m/s trong 300 ms danh định.
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
  // Lấy tư thế hiện tại làm zero hoặc dịch zero từng 0,1° khi xe ở Idle và IMU cho thấy thân gần đứng yên.
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
    restartAutonomousCountdown();
    sensors_.theta=rawPitch-pitchZero_;
    std::snprintf(out,capacity,"ACK:%s,zero_rad=%.6f",line,pitchZero_);
    return;
  }
  if(!std::strcmp(tok[0],"$K")) {reply("ERR:READ_ONLY_GAIN");return;}
  // Chỉ nhận K mới khi Idle, mẫu IMU còn mới và hash khớp cả cấu hình lẫn mô hình rời rạc.
  if(!std::strcmp(tok[0],"LQR2") && count==4) {
    if(state_!=State::Idle){reply("ERR:STATE");return;}
    if(!sensorsFresh(now)){reply("ERR:IMU");return;}
    if(std::strcmp(tok[1],cfg::sha256)||std::strcmp(tok[1],lqr_model::hash)){reply("ERR:CONFIG");return;}
    double k0,k1;
    if(!number(tok[2],k0)||!number(tok[3],k1)||std::abs(k0)>10000||std::abs(k1)>10000){reply("ERR:GAIN_RANGE");return;}
    if(!control_.setGains(k0,k1)){reply("ERR:UNSTABLE_GAIN");return;}
    restartAutonomousCountdown();
    std::snprintf(out,capacity,"ACK:LQR2,%.12g,%.12g",control_.k0(),control_.k1());return;
  }
  if(!std::strcmp(tok[0],"GET") && count==1) {
    char status[900];telemetryTo(now,status,sizeof status);
    std::snprintf(out,capacity,"%s\nPID:V,%.9g,%.9g,%.9g\nPID:Y,%.9g,%.9g,%.9g\nLQR:%.12g,%.12g",status,control_.velocity.kp,control_.velocity.ki,control_.velocity.kd,control_.yaw.kp,control_.yaw.ki,control_.yaw.kd,control_.k0(),control_.k1());return;
  }
  // STOP tắt motor và giữ lỗi đang có. ESTOP chốt lỗi riêng. RESET xóa lỗi khi motor đã tắt.
  // Cả ba đều khóa tự chạy. START thủ công tạo phiên mới và vào Arming nếu tư thế đạt.
  if(count==1 && !std::strcmp(tok[0],"ESTOP")) {suspendAutonomous();fault("ESTOP");reply("ACK:ESTOP");return;}
  if(count==1 && !std::strcmp(tok[0],"STOP")) {suspendAutonomous();if(state_!=State::Fault)state_=State::Idle;clearMotion();reply("ACK:STOP");return;}
  if(count==1 && !std::strcmp(tok[0],"RESET")) {
    if(state_==State::Run || state_==State::Arming) {reply("ERR:MOTORS_ON");return;}
    suspendAutonomous();state_=State::Idle;fault_="NONE";clearMotion();reply("ACK:RESET");return;
  }
  if(count==1 && !std::strcmp(tok[0],"START")) {
    if(state_==State::Fault) {reply("ERR:RESET_REQUIRED");return;}
    if(state_!=State::Idle) {reply("ERR:STATE");return;}
    if(!sensorsFresh(now)) {reply("ERR:IMU");return;}
    if(std::abs(sensors_.theta)>=cfg::arm_rad || std::abs(sensors_.theta+pitchZero_)>=cfg::arm_rad || std::abs(sensors_.omega)>=cfg::arm_rate) {reply("ERR:POSE");return;}
    clearMotion();auto_blocked_=false;auto_timing_=false;++session_;if(!session_)++session_;state_=State::Arming;
    arm_timing_=true;arm_since_=now;reply("ACK:START");return;
  }
  if(!std::strcmp(tok[0],"$P") && count==5) {
    if(state_==State::Run||state_==State::Arming) {reply("ERR:MOTORS_ON");return;}
    double kp,ki,kd;
    if(!number(tok[2],kp)||!number(tok[3],ki)||!number(tok[4],kd)||kp<0||kp>5||ki<0||ki>2||kd<0||kd>.2) {reply("ERR:GAIN_RANGE");return;}
    PID* pid=!std::strcmp(tok[1],"V")?&control_.velocity:(!std::strcmp(tok[1],"Y")?&control_.yaw:nullptr);
    if(!pid) {reply("ERR:AXIS");return;}restartAutonomousCountdown();pid->kp=kp;pid->ki=ki;pid->kd=kd;pid->reset();reply("ACK:$P");return;
  }
  // MOTION mở/khóa đường mục tiêu tiến và quay trong Run. Khi khóa, xóa mục tiêu và hạn lệnh.
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
    // Giao thức SPEED dùng cm/s, TURN dùng độ/s. Đổi sang m/s và rad/s trước khi dùng trong PID.
    if(speed)target_v_=value/100.;else target_w_=value*3.14159265358979323846/180.;
    last_command_=now;leased_=true;reply(speed?"ACK:SPEED":"ACK:TURN");return;
  }
  // CMD gửi đồng thời vận tốc dọc (m/s) và tốc độ quay (rad/s), có mã phiên và số thứ tự để lọc lệnh cũ.
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
  std::snprintf(out,capacity,"t=%lu,theta=%.9g,omega=%.9g,v=%.9g,yaw=%.9g,left=%.9g,right=%.9g,u=%.9g,st=%u,motion=%u,session=%lu,seq=%lu,config=%s,hash=%s,fresh=%u,sat=%u,jitter_us=%ld,overruns=%lu,fault=%s,auto_mode=%u,auto_wait=%u,auto_ms=%lu,lqr2=1",
    static_cast<unsigned long>(now/1000),sensors_.theta,sensors_.omega,sensors_.v,sensors_.yaw,output_.left,output_.right,output_.u,unsigned(state_),unsigned(output_.motion_active),static_cast<unsigned long>(session_),static_cast<unsigned long>(last_tick_/5000),cfg::id,cfg::sha256,unsigned(commandFresh(now)),unsigned(output_.saturated),static_cast<long>(jitter_us_),static_cast<unsigned long>(overruns_),fault_,unsigned(auto_feature_),unsigned(autonomousWaiting()),static_cast<unsigned long>(autonomousElapsed(now)/1000));
}
std::string Device::telemetry(uint32_t now) const {char out[900];telemetryTo(now,out,sizeof out);return out;}
}
