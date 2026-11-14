#line 1 "D:\\blancing\\Robot_Collab\\03_Firmware_ESP32_S3\\balancebot\\PhoneWeb.h"
#pragma once
#include "PhoneHttp.h"
#include "PhonePage.h"
#include "PhoneAuth.h"
namespace phone {
// Owned exclusively by the existing network task (core 0). The control task
// receives the same bounded Request queue as USB/TCP; it never serves HTTP.
// One short HTTP exchange at a time; RX <=192 B and TX <=1024 B per poll.
class Web {
  WiFiServer listener{80};WiFiClient peer;
  Request request{};bool active=false,responding=false;
  uint32_t since=0,replySequence=0,sessionSince=0,attemptSince=0;unsigned attempts=0;
  bool sessionActive=false,controlGranted=false,startPending=false,registrationWaiting=false;
  char secret[33]{},sessionUser[33]{},head[512]{},data[2800]{},lastReply[1200]{},cookieHeader[140]{};
  const char* body=nullptr;size_t headSize=0,bodySize=0,offset=0;
  void (*command)(const char*,uint8_t)=nullptr;
  void (*snapshot)(char*,size_t)=nullptr;
  void revoke(){const bool moving=controlGranted||startPending;sessionActive=false;controlGranted=false;startPending=false;secret[0]=0;if(moving&&command){command("SPEED,0",3);command("TURN,0",3);}}
  void close(){peer.stop();active=false;responding=false;registrationWaiting=false;}
  void response(int code,const char* text,const char* type="text/plain; charset=utf-8",size_t size=0,bool gzip=false,const char* extra="") {
    body=text;bodySize=size?size:std::strlen(text);offset=0;responding=true;
    headSize=size_t(std::snprintf(head,sizeof head,"HTTP/1.1 %d Response\r\nContent-Type: %s\r\nContent-Length: %u\r\n%s%sCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nConnection: close\r\n\r\n",code,type,unsigned(bodySize),gzip?"Content-Encoding: gzip\r\n":"",extra));
  }
  void error(int code,const char* text){std::snprintf(data,sizeof data,"{\"error\":\"%s\"}",text);response(code,data,"application/json");}
  bool sameOrigin()const {char value[96];std::snprintf(value,sizeof value,"http://%s",request.host);return !*request.origin||!std::strcmp(value,request.origin);}
  bool authenticated() {
    if(sessionActive&&uint32_t(millis()-sessionSince)>=28800000){revoke();return false;}
    if(!sessionActive)return false;
    const char* p=request.cookie;while(*p){while(*p==' '||*p==';')++p;const char* end=std::strchr(p,';');if(!end)end=p+std::strlen(p);
      if(size_t(end-p)==41&&!std::memcmp(p,"bb_phone=",9)&&!std::memcmp(p+9,secret,32))return true;
      p=end;
    }return false;
  }
  void authPost(bool registration) {
    if(!sameOrigin()){error(403,"Yêu cầu khác nguồn bị từ chối.");return;}
    if(uint32_t(millis()-attemptSince)>=60000){attemptSince=millis();attempts=0;}
    if(attempts>=20){error(429,"Thử quá nhiều lần. Vui lòng đợi một phút.");return;}++attempts;
    char user[33]{};const char* password=nullptr;
    if(!auth::credentials(request.body(),user,password)){error(400,"Tên tài khoản 3–32 chữ/số/_; mật khẩu 8–128 byte.");return;}
    if(registration) {
      if(auth::storageError.load()){error(503,"Không đọc được bộ nhớ tài khoản.");return;}
      if(auth::exists(user)){error(409,"Tên tài khoản đã tồn tại.");return;}
      if(auth::count.load()>=auth::maxAccounts){error(409,"Bộ nhớ đã đủ 8 tài khoản.");return;}
      snapshot(data,sizeof data);
      if(std::strstr(data,",st=1,")||std::strstr(data,",st=2,")){error(409,"Dừng motor trước khi lưu tài khoản.");return;}
      char line[161]{};
      if(!auth::prepare(request.body(),line)){error(400,"Không tạo được tài khoản.");return;}
      std::memset(request.bytes+request.header,0,request.bodySize);
      registrationWaiting=true;command(line,3);return;
    }
    const bool ok=auth::verify(request.body());std::memset(request.bytes+request.header,0,request.bodySize);
    if(!ok){error(401,"Sai tài khoản hoặc mật khẩu.");return;}
    revoke();
    std::snprintf(secret,sizeof secret,"%08lx%08lx%08lx%08lx",static_cast<unsigned long>(esp_random()),static_cast<unsigned long>(esp_random()),static_cast<unsigned long>(esp_random()),static_cast<unsigned long>(esp_random()));
    sessionActive=true;sessionSince=millis();std::strcpy(sessionUser,user);
    std::snprintf(cookieHeader,sizeof cookieHeader,"Set-Cookie: bb_phone=%s; HttpOnly; SameSite=Strict; Path=/; Max-Age=28800\r\n",secret);
    response(200,"{\"success\":true}","application/json",0,false,cookieHeader);
  }
  bool localHost() {
    char ap[80],sta[80];std::snprintf(ap,sizeof ap,"%s",WiFi.softAPIP().toString().c_str());
    std::snprintf(sta,sizeof sta,"%s",WiFi.localIP().toString().c_str());
    char host[80];std::strcpy(host,request.host);char* port=std::strchr(host,':');
    if(port){if(std::strcmp(port,":80"))return false;*port=0;}
    return (!std::strcmp(host,ap)||!std::strcmp(host,sta))&&std::strcmp(host,"0.0.0.0");
  }
  void dispatch() {
    if(!localHost()){response(403,"ERR:HOST");return;}
    const bool get=!std::strcmp(request.method,"GET"),post=!std::strcmp(request.method,"POST");
    if(get&&(!std::strcmp(request.path,"/")||!std::strcmp(request.path,"/welcome"))){const bool logged=authenticated()&&std::strcmp(request.path,"/welcome");response(200,reinterpret_cast<const char*>(logged?page:welcome),"text/html; charset=utf-8",logged?pageSize:welcomeSize,true);return;}
    if(get&&!std::strcmp(request.path,"/icons/icon.svg")){response(200,reinterpret_cast<const char*>(icon),"image/svg+xml",iconSize);return;}
    if(get&&!std::strcmp(request.path,"/manifest.json")){response(200,reinterpret_cast<const char*>(manifest),"application/json",manifestSize);return;}
    if(get&&!std::strcmp(request.path,"/api/auth/me")){const bool logged=authenticated();std::snprintf(data,sizeof data,"{\"username\":%s%s%s}",logged?"\"":"",logged?sessionUser:"null",logged?"\"":"");response(200,data,"application/json");return;}
    if(post&&(!std::strcmp(request.path,"/api/auth/login")||!std::strcmp(request.path,"/api/auth/register"))){authPost(!std::strcmp(request.path,"/api/auth/register"));return;}
    if(!authenticated()){error(401,"Vui lòng đăng nhập.");return;}
    if(get&&!std::strcmp(request.path,"/session")){response(200,secret);return;}
    if(!request.authorized(secret)){response(403,"ERR:TOKEN_OR_ORIGIN");return;}
    if(post&&!std::strcmp(request.path,"/api/auth/logout")){revoke();response(200,"{\"success\":true}","application/json",0,false,"Set-Cookie: bb_phone=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0\r\n");return;}
    if(get&&!std::strcmp(request.path,"/status")) {
      snapshot(data,sizeof data);
      const size_t n=std::strlen(data);std::snprintf(data+n,sizeof data-n,"\nreply_id=%lu\n%.1199s",static_cast<unsigned long>(replySequence),lastReply);
      response(200,data);return;
    }
    if(post&&!std::strcmp(request.path,"/command")) {
      if(!request.bodySize){response(400,"ERR:EMPTY");return;}
      if(request.bodySize>160){response(413,"ERR:LINE_TOO_LONG");return;}
      for(const char* p=request.body();*p;++p)if(static_cast<unsigned char>(*p)<32||static_cast<unsigned char>(*p)>126){response(400,"ERR:FORMAT");return;}
      const bool start=!std::strcmp(request.body(),"START"),stop=!std::strcmp(request.body(),"STOP")||!std::strcmp(request.body(),"ESTOP");
      if(stop){controlGranted=false;startPending=false;}
      if(!std::strncmp(request.body(),"CMD,",4)||!std::strncmp(request.body(),"SPEED,",6)||!std::strncmp(request.body(),"TURN,",5)||!std::strncmp(request.body(),"MOTION,",7)){
        if(!controlGranted){response(403,"ERR:PRESS_START");return;}
      }
      if(!std::strncmp(request.body(),"WEBUSER,",8)){response(403,"ERR:PRIVATE_COMMAND");return;}
      if(start){snapshot(data,sizeof data);if(std::strstr(data,",st=2,")&&std::strstr(data,",imu_calibrated=1,")&&std::strstr(data,",imu_sample_fresh=1,")){controlGranted=true;response(200,"ACK:CONTROL_GRANTED");return;}startPending=true;}
      std::strcpy(lastReply,"QUEUED");command(request.body(),3);
      // Immediate transport errors/STOP acknowledgement are available here.
      // Ordinary commands are acknowledged only when the control task replies.
      std::snprintf(data,sizeof data,"%s",lastReply);response(202,data);return;
    }
    response(404,"ERR:NOT_FOUND");
  }
public:
  void begin(void (*accept)(const char*,uint8_t),void (*status)(char*,size_t)) {
    command=accept;snapshot=status;
    listener.begin();
  }
  void reply(const char* text){
    std::snprintf(lastReply,sizeof lastReply,"%s",text);++replySequence;
    if(startPending&&(!std::strcmp(text,"ACK:START")||!std::strncmp(text,"ERR:",4))){controlGranted=!std::strcmp(text,"ACK:START");startPending=false;}
    if(registrationWaiting&&(!std::strcmp(text,"ACK:WEBUSER")||!std::strncmp(text,"ERR:",4))){registrationWaiting=false;if(!std::strcmp(text,"ACK:WEBUSER")){revoke();response(201,"{\"registered\":true}","application/json",0,false,"Set-Cookie: bb_phone=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0\r\n");}else error(409,"Chưa lưu tài khoản: motor phải tắt và bộ nhớ phải ghi thành công.");}
  }
  void poll() {
    if(!active&&listener.hasClient()){peer=listener.accept();peer.setNoDelay(true);request=Request{};active=true;responding=false;since=millis();}
    if(!active)return;
    const bool authRequest=!std::strncmp(request.path,"/api/auth/",10);
    if(!peer.connected()||uint32_t(millis()-since)>(authRequest?8000u:1500u)){close();return;}
    if(!responding) {
      for(unsigned n=0;n<192&&peer.available()&&!request.complete&&!request.error;++n)request.feed(char(peer.read()));
      if(request.error)response(request.error,"ERR:HTTP_REQUEST");
      else if(request.complete&&!registrationWaiting)dispatch();
    }
    if(responding) {
      const bool header=offset<headSize;
      const char* p=header?head+offset:body+offset-headSize;
      size_t remaining=header?headSize-offset:headSize+bodySize-offset;
      if(remaining>1024)remaining=1024;
      const int sent=send(peer.fd(),p,remaining,MSG_DONTWAIT);
      if(sent>0)offset+=size_t(sent);
      else if(sent==0||(errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)){close();return;}
      if(offset==headSize+bodySize)close();
    }
  }
};
}
