#pragma once
#include <string>
constexpr int WIFI_AP_STA=3,WL_CONNECTED=3;
namespace fake_runtime {
inline bool apOk=true,autoReconnect=false,persistent=true,tcpCandidate=false;
inline int mode=0,status=6;
inline unsigned begins=0,reconnects=0,serverBegins=0;
inline std::string ssid,password;
}
struct FakeIP {std::string value;std::string toString()const{return value;}};
struct FakeWiFi {
  void persistent(bool p){fake_runtime::persistent=p;}
  bool mode(int m){fake_runtime::mode=m;return true;}
  bool softAP(const char*,const char*){return fake_runtime::apOk;}
  void setAutoReconnect(bool b){fake_runtime::autoReconnect=b;}
  int begin(const char* ssid,const char* password){++fake_runtime::begins;fake_runtime::ssid=ssid;fake_runtime::password=password;return status();}
  void reconnect(){++fake_runtime::reconnects;}
  int status(){return fake_runtime::status;}
  FakeIP softAPIP(){return {"192.168.4.1"};}
  FakeIP localIP(){return {"192.168.32.50"};}
};
inline FakeWiFi WiFi;
struct WiFiClient {
  bool live=false;
  bool connected(){return live;}
  void stop(){live=false;}
  void setNoDelay(bool){}
  int available(){return 0;}
  char read(){return 0;}
  int fd(){return 1;}
};
struct WiFiServer {
  explicit WiFiServer(int){}
  void begin(){++fake_runtime::serverBegins;}
  bool hasClient(){return fake_runtime::tcpCandidate;}
  WiFiClient accept(){fake_runtime::tcpCandidate=false;return {true};}
};
