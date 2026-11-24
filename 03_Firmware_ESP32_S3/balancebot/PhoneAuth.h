// Quản lý tài khoản web trong NVS riêng. Kiểm tra thông tin đăng nhập để PhoneWeb cấp phiên.
#pragma once
#include <Preferences.h>
#include <mbedtls/pkcs5.h>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>
// Up to eight accounts, separate from IMU/PID NVS. Slots are append-only.
// Hashing runs in the network task. Only the control task may commit a record,
// after checking motors are off; a flash write cannot overlap arming.
namespace phone { namespace auth {
constexpr unsigned iterations=20000;
constexpr unsigned maxAccounts=8;
struct Account {uint32_t magic=0,rounds=iterations;char username[33]{};uint8_t salt[16]{},hash[32]{};uint32_t crc=0;};
struct Database {uint32_t magic=0,count=0;Account users[maxAccounts]{};uint32_t crc=0;};
inline Account accounts[maxAccounts]{};
inline std::atomic<unsigned> count{0};
inline std::atomic<bool> storageError{false};
inline uint32_t checksum(const void* value,size_t length) {
  const auto* bytes=static_cast<const uint8_t*>(value);uint32_t crc=0xffffffffu;
  for(size_t i=0;i<length;++i){crc^=bytes[i];for(int n=0;n<8;++n)crc=(crc>>1)^(0xedb88320u&uint32_t(-int32_t(crc&1)));}
  return ~crc;
}
inline bool usernameValid(const char* s) {
  const size_t n=std::strlen(s);if(n<3||n>32)return false;
  for(;*s;++s)if(!((*s>='a'&&*s<='z')||(*s>='0'&&*s<='9')||*s=='_'))return false;
  return true;
}
inline bool valid(const Account& a) {return a.magic==0x50484131u&&a.rounds>=10000&&a.rounds<=200000&&a.username[32]==0&&usernameValid(a.username)&&a.crc==checksum(&a,offsetof(Account,crc));}
inline bool valid(const Database& d) {
  if(d.magic!=0x50484431u||d.count>maxAccounts||d.crc!=checksum(&d,offsetof(Database,crc)))return false;
  for(unsigned i=0;i<d.count;++i){if(!valid(d.users[i]))return false;for(unsigned j=0;j<i;++j)if(!std::strcmp(d.users[i].username,d.users[j].username))return false;}return true;
}
inline bool exists(const char* user) {const auto n=count.load(std::memory_order_acquire);for(unsigned i=0;i<n;++i)if(!std::strcmp(accounts[i].username,user))return true;return false;}
inline void load() {
  Database value{};Preferences p;bool ok=false,bad=false;
  if(p.begin("phone-auth",true)){const size_t length=p.getBytesLength("users-v1");ok=length==sizeof value&&p.getBytes("users-v1",&value,sizeof value)==sizeof value&&valid(value);bad=length&&!ok;p.end();}
  // An absent namespace is the normal first boot. Corrupt records fail closed.
  storageError.store(bad,std::memory_order_release);
  if(ok)for(unsigned i=0;i<value.count;++i)std::memcpy(&accounts[i],&value.users[i],sizeof(Account));
  count.store(ok?value.count:0,std::memory_order_release);
}
inline bool credentials(const char* body,char (&user)[33],const char*& password) {
  const char* split=std::strchr(body,'\n');if(!split||size_t(split-body)>32)return false;
  const size_t n=size_t(split-body);std::memcpy(user,body,n);user[n]=0;
  for(char* c=user;*c;++c)if(*c>='A'&&*c<='Z')*c=char(*c+32);
  password=split+1;const size_t len=std::strlen(password);
  return usernameValid(user)&&len>=8&&len<=128&&!std::strchr(password,'\n')&&!std::strchr(password,'\r');
}
inline bool derive(const char* password,const uint8_t* salt,uint8_t* result,unsigned rounds=iterations) {
  return mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256,reinterpret_cast<const unsigned char*>(password),std::strlen(password),salt,16,rounds,32,result)==0;
}
inline void hex(const uint8_t* data,size_t n,char* out) {constexpr char digits[]="0123456789abcdef";for(size_t i=0;i<n;++i){out[2*i]=digits[data[i]>>4];out[2*i+1]=digits[data[i]&15];}out[2*n]=0;}
inline bool unhex(const char* text,uint8_t* out,size_t n) {
  if(std::strlen(text)!=2*n)return false;
  for(size_t i=0;i<n;++i){unsigned v=0;for(unsigned j=0;j<2;++j){char c=text[2*i+j];if(c>='0'&&c<='9')v=v*16+unsigned(c-'0');else if(c>='a'&&c<='f')v=v*16+unsigned(c-'a'+10);else return false;}out[i]=uint8_t(v);}return true;
}
// Tạo lệnh WEBUSER nội bộ từ tên tài khoản, salt và hash mật khẩu để runtime xử lý việc ghi NVS.
inline bool prepare(const char* body,char (&line)[161]) {
  if(storageError.load(std::memory_order_acquire)||count.load(std::memory_order_acquire)>=maxAccounts)return false;
  char user[33]{},saltHex[33]{},hashHex[65]{};const char* password=nullptr;uint8_t salt[16]{},hash[32]{};
  if(!credentials(body,user,password)||exists(user))return false;
  for(size_t i=0;i<16;i+=4){const uint32_t random=esp_random();std::memcpy(salt+i,&random,4);}
  if(!derive(password,salt,hash))return false;
  hex(salt,16,saltHex);hex(hash,32,hashHex);
  std::snprintf(line,sizeof line,"WEBUSER,%s,%s,%s",user,saltHex,hashHex);return true;
}
// Nhận bản ghi từ runtime khi motor tắt, lưu cơ sở tài khoản rồi mới cập nhật danh sách trong RAM.
inline bool commit(const char* line) {
  const unsigned n=count.load(std::memory_order_acquire);
  if(storageError.load(std::memory_order_acquire)||n>=maxAccounts||std::strlen(line)>160)return false;
  char copy[161];std::strcpy(copy,line);char* user=std::strchr(copy,',');if(!user)return false;*user++=0;
  char* salt=std::strchr(user,',');if(!salt)return false;*salt++=0;
  char* hash=std::strchr(salt,',');if(!hash)return false;*hash++=0;
  if(std::strcmp(copy,"WEBUSER")||!usernameValid(user)||exists(user))return false;
  Account value{};std::memset(static_cast<void*>(&value),0,sizeof value);value.magic=0x50484131u;value.rounds=iterations;std::strcpy(value.username,user);
  if(!unhex(salt,value.salt,16)||!unhex(hash,value.hash,32))return false;
  value.crc=checksum(&value,offsetof(Account,crc));Database database{},readback{};std::memset(static_cast<void*>(&database),0,sizeof database);database.magic=0x50484431u;database.count=n+1;
  for(unsigned i=0;i<n;++i)std::memcpy(&database.users[i],&accounts[i],sizeof(Account));
  std::memcpy(&database.users[n],&value,sizeof value);database.crc=checksum(&database,offsetof(Database,crc));
  Preferences p;if(!p.begin("phone-auth",false))return false;
  const bool ok=p.putBytes("users-v1",&database,sizeof database)==sizeof database&&p.getBytes("users-v1",&readback,sizeof readback)==sizeof readback&&valid(readback)&&!std::memcmp(&database,&readback,sizeof database);p.end();
  if(ok){std::memcpy(&accounts[n],&value,sizeof value);count.store(n+1,std::memory_order_release);}return ok;
}
inline bool verify(const char* body) {
  const unsigned n=count.load(std::memory_order_acquire);if(!n)return false;
  char user[33]{};const char* password=nullptr;uint8_t hash[32]{};
  if(!credentials(body,user,password))return false;
  unsigned index=0;bool found=false;for(unsigned i=0;i<n;++i)if(!std::strcmp(user,accounts[i].username)){index=i;found=true;}
  if(!derive(password,accounts[index].salt,hash,accounts[index].rounds))return false;
  unsigned different=unsigned(!found);
  for(size_t i=0;i<32;++i)different|=hash[i]^accounts[index].hash[i];return different==0;
}
}}
