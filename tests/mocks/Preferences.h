#pragma once
#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
namespace fake_imu {
inline std::map<std::string,std::vector<uint8_t>> nvs;
inline unsigned writes=0;
inline bool writeFailure=false;
}
class Preferences {
  std::string name;
 public:
  bool begin(const char* ns,bool=false){name=ns;return true;}
  void end(){}
  size_t getBytesLength(const char* key){return fake_imu::nvs[name+"/"+key].size();}
  size_t getBytes(const char* key,void* data,size_t size){auto& bytes=fake_imu::nvs[name+"/"+key];if(bytes.size()!=size)return 0;std::memcpy(data,bytes.data(),size);return size;}
  size_t putBytes(const char* key,const void* data,size_t size){
    ++fake_imu::writes;if(fake_imu::writeFailure)return 0;
    auto bytes=static_cast<const uint8_t*>(data);fake_imu::nvs[name+"/"+key]={bytes,bytes+size};return size;
  }
};
