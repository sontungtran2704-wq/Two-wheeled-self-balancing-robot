// Gom từng byte của yêu cầu HTTP thành phương thức, đường dẫn, tiêu đề và thân để PhoneWeb định tuyến.
#pragma once
#include <cstddef>
#include <cstring>
#include <cstdio>
// Fixed memory HTTP/1.x request parser. No chunked bodies, pipelining or heap.
namespace phone {
struct Request {
  char bytes[1220]{};size_t used=0,header=0,bodySize=0;
  char method[8]{},path[32]{},host[80]{},origin[88]{},token[40]{},cookie[128]{};
  int error=0;bool complete=false;
  static bool equal(const char* a,const char* b) {
    while(*a&&*b){char x=*a++,y=*b++;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return false;}return *a==*b;
  }
  static bool copy(char* dst,size_t cap,const char* value) {
    if(std::strlen(value)>=cap)return false;
    std::strcpy(dst,value);return true;
  }
  void parseHeaders() {
    bytes[header-2]=0;
    char* end=std::strstr(bytes,"\r\n");if(!end){error=400;return;}*end=0;
    char version[12]{},extra[2]{};
    if(std::sscanf(bytes,"%7s %31s %11s %1s",method,path,version,extra)!=3 ||
       (std::strcmp(version,"HTTP/1.1")&&std::strcmp(version,"HTTP/1.0"))){error=400;return;}
    bool lengthSeen=false,hostSeen=false,originSeen=false,tokenSeen=false,cookieSeen=false;
    for(char* p=end+2;*p;) {
      end=std::strstr(p,"\r\n");if(!end)end=p+std::strlen(p);char* next=*end?end+2:end;*end=0;
      char* colon=std::strchr(p,':');if(!colon){error=400;return;}*colon++=0;
      while(*colon==' '||*colon=='\t')++colon;
      if(equal(p,"Content-Length")) {
        if(lengthSeen||!*colon){error=400;return;}lengthSeen=true;
        for(const char* n=colon;*n;++n){if(*n<'0'||*n>'9'){error=400;return;}bodySize=bodySize*10+size_t(*n-'0');if(bodySize>192){error=413;return;}}
      } else if(equal(p,"Transfer-Encoding")){error=400;return;}
      else if(equal(p,"Host")){if(hostSeen||!copy(host,sizeof host,colon)){error=400;return;}hostSeen=true;}
      else if(equal(p,"Origin")){if(originSeen||!copy(origin,sizeof origin,colon)){error=400;return;}originSeen=true;}
      else if(equal(p,"X-Robot-Token")){if(tokenSeen||!copy(token,sizeof token,colon)){error=400;return;}tokenSeen=true;}
      else if(equal(p,"Cookie")){if(cookieSeen||!copy(cookie,sizeof cookie,colon)){error=400;return;}cookieSeen=true;}
      p=next;
    }
    if(!hostSeen){error=400;return;}
    if(!std::strcmp(method,"POST")&&!lengthSeen){error=411;return;}
  }
  void feed(char c) {
    if(error||complete)return;
    if(!c || used>=sizeof bytes-1){error=413;return;}
    bytes[used++]=c;bytes[used]=0;
    if(!header) {
      if(used>=4&&!std::memcmp(bytes+used-4,"\r\n\r\n",4)){header=used;parseHeaders();}
      else if(used>=1024){error=431;return;}
    }
    if(header&&used==header+bodySize)complete=true;
  }
  const char* body()const{return bytes+header;}
  // So token phiên và kiểm tra Origin theo Host khi có Origin, trước khi cho truy cập API điều khiển.
  bool authorized(const char* expected)const {
    char allowed[96];std::snprintf(allowed,sizeof allowed,"http://%s",host);
    return !std::strcmp(token,expected)&&(!*origin||!std::strcmp(origin,allowed));
  }
};
}
