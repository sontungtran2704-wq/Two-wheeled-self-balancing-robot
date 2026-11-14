#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
uint32_t esp_random(){return 0x12345678;}
#include "PhoneAuth.h"
int main(){
  using namespace phone;
  fake_imu::nvs.clear();fake_imu::writes=0;auth::load();assert(auth::count==0&&!auth::storageError&&fake_imu::writes==0);
  char line[161]{};assert(!auth::prepare("a\nshort",line));assert(auth::prepare("Owner\npassword123",line));assert(!std::strstr(line,"password123"));
  assert(auth::commit(line)&&auth::count==1&&fake_imu::writes==1);
  char hash[65];auth::hex(auth::accounts[0].hash,32,hash);
  assert(!std::strcmp(hash,"edaf0fc76c4b6db3b148804f6900e8e0357ddf04a880a93ec0afca839ad59760"));
  assert(auth::verify("OWNER\npassword123"));assert(!auth::verify("owner\nwrongpass"));assert(!auth::verify("other\npassword123"));assert(!auth::prepare("owner\npassword123",line));assert(fake_imu::writes==1);
  assert(auth::prepare("second\npassword456",line));assert(auth::commit(line));assert(auth::count==2&&auth::verify("owner\npassword123")&&auth::verify("second\npassword456"));
  const unsigned writes=fake_imu::writes;auth::count=0;auth::load();assert(auth::count==2&&auth::verify("second\npassword456")&&fake_imu::writes==writes);
  fake_imu::writeFailure=true;assert(auth::prepare("third\npassword789",line));assert(!auth::commit(line));assert(auth::count==2&&auth::verify("owner\npassword123"));fake_imu::writeFailure=false;
  for(int i=2;i<8;++i){const std::string body="user_"+std::to_string(i)+"\npassword123";assert(auth::prepare(body.c_str(),line));assert(auth::commit(line));}
  assert(auth::count==8&&!auth::prepare("overflow\npassword123",line));
  auth::load();assert(auth::count==8&&auth::verify("user_7\npassword123"));
  fake_imu::nvs["phone-auth/users-v1"][10]^=1;auth::load();assert(auth::count==0&&auth::storageError&&!auth::prepare("resetuser\npassword123",line));
  std::cout<<"PASS: PBKDF2 SHA256 matches Python golden vector; multiple hashed accounts/readback, duplicate/range/capacity rejection, reboot reuse without writes, write failure and corrupt NVS fail closed\n";
}
