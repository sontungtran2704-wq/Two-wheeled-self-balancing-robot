#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <string>
#include <iostream>
#include <algorithm>
static uint32_t clockMs=0;
uint32_t millis(){return clockMs;}
uint32_t esp_random(){return 0x12345678;}
constexpr int MSG_DONTWAIT=1;
struct Channel {bool live=false,candidate=false;std::string input,output;size_t rx=0;int chunk=512,fail=0;};
static Channel channel;
struct WiFiClient {
  bool connected(){return channel.live;}
  void stop(){channel.live=false;}
  void setNoDelay(bool){}
  int available(){return int(channel.input.size()-channel.rx);}
  int read(){return static_cast<unsigned char>(channel.input[channel.rx++]);}
  int fd(){return 1;}
};
struct WiFiServer {
  explicit WiFiServer(int){}
  void begin(){}
  bool hasClient(){return channel.candidate;}
  WiFiClient accept(){channel.candidate=false;return {};}
};
struct IP {std::string value;std::string toString(){return value;}};
struct Wifi {IP softAPIP(){return {"192.168.4.1"};}IP localIP(){return {"0.0.0.0"};}} WiFi;
int send(int,const char* p,size_t size,int){if(channel.fail){errno=channel.fail;return -1;}size=std::min(size,size_t(channel.chunk));channel.output.append(p,size);return int(size);}
#include "PhoneWeb.h"
static phone::Web web;
static std::string accepted,cookie;static unsigned calls=0;static std::vector<std::string> commands;
void accept(const char* text,uint8_t source){assert(source==3);if(!std::strncmp(text,"WEBUSER,",8)){web.reply(phone::auth::commit(text)?"ACK:WEBUSER":"ERR:ACCOUNT_STORAGE");return;}++calls;accepted=text;commands.emplace_back(text);web.reply(!std::strcmp(text,"START")?"ACK:START":"ACK:TEST");}
void status(char* out,size_t cap){std::snprintf(out,cap,"t=123,session=1,st=0,imu_calibrated=0");}
std::string req(const std::string& method,const std::string& path,const std::string& body="",const std::string& token="12345678123456781234567812345678",const std::string& host="192.168.4.1") {
  return method+" "+path+" HTTP/1.1\r\nHost: "+host+"\r\nOrigin: http://"+host+"\r\nX-Robot-Token: "+token+"\r\n"+(cookie.empty()?"":"Cookie: "+cookie+"\r\n")+"Content-Length: "+std::to_string(body.size())+"\r\n\r\n"+body;
}
std::string exchange(const std::string& text) {
  channel=Channel{};channel.live=channel.candidate=true;channel.input=text;
  for(unsigned n=0;n<1000&&channel.live;++n){web.poll();clockMs+=2;}
  assert(!channel.live);return channel.output;
}
phone::Request parse(const std::string& text){phone::Request r;for(char c:text)r.feed(c);return r;}
void validResponse(const std::string& text,const std::string& body){const auto split=text.find("\r\n\r\n");assert(split!=std::string::npos);assert(text.substr(split+4)==body);assert(text.find("Content-Length: "+std::to_string(body.size())+"\r\n")!=std::string::npos);}
int main(){
  phone::auth::load();web.begin(accept,status);
  auto r=parse(req("POST","/command","START"));assert(r.complete&&!r.error&&!std::strcmp(r.body(),"START"));assert(r.authorized("12345678123456781234567812345678"));
  assert(parse("POST /command HTTP/1.1\r\nHost: x\r\nContent-Length: 2\r\nContent-Length: 2\r\n\r\nOK").error==400);
  assert(parse("POST /command HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n").error==400);
  assert(parse("POST /command HTTP/1.1\r\nHost: x\r\nContent-Length: -1\r\n\r\n").error==400);
  assert(parse(req("POST","/command",std::string(193,'X'))).error==413);
  assert(parse("GET / HTTP/1.1\r\nHost: "+std::string(1100,'X')).error==431);
  assert(parse("POST /command HTTP/1.1\r\nHost: x\r\n\r\n").error==411);
  const std::string embedded(reinterpret_cast<const char*>(phone::page),phone::pageSize);
  validResponse(exchange(req("GET","/")),std::string(reinterpret_cast<const char*>(phone::welcome),phone::welcomeSize));
  assert(exchange(req("GET","/session")).find("401")!=std::string::npos);
  assert(exchange(req("POST","/command","START")).find("401")!=std::string::npos);assert(!calls);
  assert(exchange(req("POST","/api/auth/register","tester\npassword123")).find("201")!=std::string::npos);
  assert(exchange(req("POST","/api/auth/register","tester\npassword123")).find("409")!=std::string::npos);
  assert(exchange(req("POST","/api/auth/login","tester\nwrongpass")).find("401")!=std::string::npos);
  const auto login=exchange(req("POST","/api/auth/login","tester\npassword123"));assert(login.find("Set-Cookie: bb_phone=")!=std::string::npos);
  cookie="bb_phone=12345678123456781234567812345678";
  assert(exchange(req("POST","/command","CMD,1,1,0.05,0")).find("403")!=std::string::npos);assert(!calls);
  assert(exchange(req("POST","/command","WEBUSER,injected,0,0")).find("403")!=std::string::npos);assert(!calls);
  assert(exchange(req("POST","/command",std::string(161,'X'))).find("413")!=std::string::npos);
  auto page=exchange(req("GET","/"));validResponse(page,embedded);assert(page.find("Content-Encoding: gzip\r\n")!=std::string::npos);assert(phone::page[0]==0x1f&&phone::page[1]==0x8b);
  auto session=exchange(req("GET","/session"));validResponse(session,"12345678123456781234567812345678");
  assert(exchange(req("POST","/command","START","bad")).find("403")!=std::string::npos);assert(!calls);
  assert(exchange(req("POST","/command","START","12345678123456781234567812345678","attacker.test")).find("403")!=std::string::npos);assert(!calls);
  auto foreign=req("POST","/command","START");foreign.replace(foreign.find("Origin: http://192.168.4.1"),24,"Origin: http://evil.test");assert(exchange(foreign).find("403")!=std::string::npos);assert(!calls);
  assert(exchange(req("POST","/command","START\nSTOP")).find("400")!=std::string::npos);assert(!calls);
  validResponse(exchange(req("POST","/command","START")),"ACK:START");assert(calls==1&&accepted=="START");
  web.reply("ACK:CALGYRO_PENDING");const auto calStatus=exchange(req("GET","/status"));assert(calStatus.find("reply_id=")!=std::string::npos&&calStatus.find("ACK:CALGYRO_PENDING")!=std::string::npos);
  // Backpressure must retain bytes, including partial header/body writes.
  channel=Channel{};channel.live=channel.candidate=true;channel.input=req("GET","/");channel.fail=EAGAIN;
  for(int n=0;n<10;++n){web.poll();clockMs+=2;}assert(channel.live&&channel.output.empty());channel.fail=0;
  for(int n=0;n<1000&&channel.live;++n){web.poll();clockMs+=2;}validResponse(channel.output,embedded);
  web.reply(std::string(1199,'X').c_str());auto longStatus=exchange(req("GET","/status"));assert(longStatus.find(std::string(1199,'X'))!=std::string::npos);
  channel=Channel{};channel.live=channel.candidate=true;channel.input=req("GET","/");channel.fail=EPIPE;web.poll();web.poll();assert(!channel.live);
  clockMs=0xfffffff0;channel=Channel{};channel.live=channel.candidate=true;channel.input="POST /command";web.poll();clockMs+=1600;web.poll();assert(!channel.live&&calls==1);
  // A new connection after timeout has no stale request or response bytes.
  validResponse(exchange(req("POST","/command","STOP")),"ACK:TEST");assert(calls==2&&accepted=="STOP");
  assert(exchange(req("POST","/command","CMD,1,2,0.05,0")).find("403")!=std::string::npos);assert(calls==2);
  validResponse(exchange(req("POST","/command","START")),"ACK:START");const auto pauseAt=commands.size();
  validResponse(exchange(req("POST","/command","CONTROL_STOP")),"ACK:CONTROL_STOP");
  assert(commands.size()==pauseAt+2&&commands[pauseAt]=="SPEED,0"&&commands[pauseAt+1]=="TURN,0");
  const auto stoppedAt=commands.size();assert(exchange(req("POST","/command","CMD,1,3,0.05,0")).find("403")!=std::string::npos);assert(commands.size()==stoppedAt);
  assert(exchange(req("POST","/api/auth/logout")).find("Max-Age=0")!=std::string::npos);assert(exchange(req("GET","/status")).find("401")!=std::string::npos);
  assert(exchange(req("POST","/api/auth/login","tester\npassword123")).find("Set-Cookie: bb_phone=")!=std::string::npos);
  clockMs+=28800001;assert(exchange(req("GET","/session")).find("401")!=std::string::npos);
  std::cout<<"PASS: bounded phone HTTP, exact offline page, token/Host/Origin checks, malformed/oversize rejection, command routing, truthful status, partial/EAGAIN TX, fatal/timeout/wrap recovery\n";
}
