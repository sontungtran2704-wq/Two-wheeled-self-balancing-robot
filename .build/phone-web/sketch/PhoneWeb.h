#line 1 "D:\\blancing\\Robot_Collab\\03_Firmware_ESP32_S3\\balancebot\\PhoneWeb.h"
#pragma once
#include "PhoneHttp.h"
#include "PhonePage.h"
namespace phone {
// Owned exclusively by the existing network task (core 0). The control task
// receives the same bounded Request queue as USB/TCP; it never serves HTTP.
// One short HTTP exchange at a time; RX <=192 B and TX <=1024 B per poll.
class Web {
  WiFiServer listener{80};WiFiClient peer;
  Request request{};bool active=false,responding=false;
  uint32_t since=0;char secret[33]{},head[256]{},data[1500]{},lastReply[1200]{};
  const char* body=nullptr;size_t headSize=0,bodySize=0,offset=0;
  void (*command)(const char*,uint8_t)=nullptr;
  void (*snapshot)(char*,size_t)=nullptr;
  void close(){peer.stop();active=false;responding=false;}
  void response(int code,const char* text,const char* type="text/plain; charset=utf-8") {
    body=text;bodySize=std::strlen(text);offset=0;responding=true;
    headSize=size_t(std::snprintf(head,sizeof head,"HTTP/1.1 %d Response\r\nContent-Type: %s\r\nContent-Length: %u\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nConnection: close\r\n\r\n",code,type,unsigned(bodySize)));
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
    if(get&&!std::strcmp(request.path,"/")){response(200,page,"text/html; charset=utf-8");return;}
    if(get&&!std::strcmp(request.path,"/session")){response(200,secret);return;}
    if(!request.authorized(secret)){response(403,"ERR:TOKEN_OR_ORIGIN");return;}
    if(get&&!std::strcmp(request.path,"/status")) {
      snapshot(data,sizeof data);
      const size_t n=std::strlen(data);std::snprintf(data+n,sizeof data-n,"\n%.1199s",lastReply);
      response(200,data);return;
    }
    if(post&&!std::strcmp(request.path,"/command")) {
      if(!request.bodySize){response(400,"ERR:EMPTY");return;}
      for(const char* p=request.body();*p;++p)if(static_cast<unsigned char>(*p)<32||static_cast<unsigned char>(*p)>126){response(400,"ERR:FORMAT");return;}
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
    std::snprintf(secret,sizeof secret,"%08lx%08lx%08lx%08lx",static_cast<unsigned long>(esp_random()),static_cast<unsigned long>(esp_random()),static_cast<unsigned long>(esp_random()),static_cast<unsigned long>(esp_random()));
    listener.begin();
  }
  void reply(const char* text){std::snprintf(lastReply,sizeof lastReply,"%s",text);}
  void poll() {
    if(!active&&listener.hasClient()){peer=listener.accept();peer.setNoDelay(true);request=Request{};active=true;responding=false;since=millis();}
    if(!active)return;
    if(!peer.connected()||uint32_t(millis()-since)>1500){close();return;}
    if(!responding) {
      for(unsigned n=0;n<192&&peer.available()&&!request.complete&&!request.error;++n)request.feed(char(peer.read()));
      if(request.error)response(request.error,"ERR:HTTP_REQUEST");
      else if(request.complete)dispatch();
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
