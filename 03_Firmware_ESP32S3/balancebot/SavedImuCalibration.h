#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cmath>
namespace imu {
// Separate from PID/zero; a new firmware image must receive its own real calibration.
// Version changes whenever this record or the IMU calibration convention changes.
struct CalibrationContext {uint8_t who,address,sda,scl;double pitchTrim;uint8_t firmwareSha256[32]{};};
struct SavedGyro {double bias[3]{},norm=0,accelStd=0,gyroStd=0;};
constexpr size_t calibrationBytes=104;
inline bool validFirmwareIdentity(const CalibrationContext& context) {
  uint8_t any=0;for(uint8_t b:context.firmwareSha256)any|=b;
  return any!=0;
}
inline uint32_t calibrationCrc(const uint8_t* bytes,size_t size) {
  uint32_t crc=0xffffffffu;
  for(size_t i=0;i<size;++i) {crc^=bytes[i];for(int bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&uint32_t(-int32_t(crc&1)));}
  return ~crc;
}
inline bool validSavedGyro(const SavedGyro& value) {
  for(double b:value.bias)if(!std::isfinite(b)||std::abs(b)>.15)return false;
  return std::isfinite(value.norm)&&value.norm>=.90&&value.norm<=1.10&&
    std::isfinite(value.accelStd)&&value.accelStd>=0&&value.accelStd<=.02&&
    std::isfinite(value.gyroStd)&&value.gyroStd>=0&&value.gyroStd<=.02;
}
inline bool encodeCalibration(uint8_t (&bytes)[calibrationBytes],const CalibrationContext& context,const SavedGyro& value) {
  static_assert(sizeof(double)==8,"Calibration format requires 64-bit double");
  if(!validSavedGyro(value)||!std::isfinite(context.pitchTrim)||!validFirmwareIdentity(context))return false;
  std::memset(bytes,0,sizeof bytes);
  bytes[0]='G';bytes[1]='Y';bytes[2]='R';bytes[3]=2;
  bytes[4]=context.who;bytes[5]=context.address;bytes[6]=context.sda;bytes[7]=context.scl;
  std::memcpy(bytes+8,&context.pitchTrim,8);
  for(int i=0;i<3;++i)std::memcpy(bytes+16+8*i,&value.bias[i],8);
  std::memcpy(bytes+40,&value.norm,8);std::memcpy(bytes+48,&value.accelStd,8);std::memcpy(bytes+56,&value.gyroStd,8);
  std::memcpy(bytes+64,context.firmwareSha256,32);
  const uint32_t crc=calibrationCrc(bytes,100);
  for(int i=0;i<4;++i)bytes[100+i]=uint8_t(crc>>(8*i));
  return true;
}
inline bool decodeCalibration(const uint8_t* bytes,size_t size,const CalibrationContext& context,SavedGyro& value) {
  if(size!=calibrationBytes||!bytes||bytes[0]!='G'||bytes[1]!='Y'||bytes[2]!='R'||bytes[3]!=2||
    bytes[4]!=context.who||bytes[5]!=context.address||bytes[6]!=context.sda||bytes[7]!=context.scl)return false;
  uint32_t crc=0;for(int i=0;i<4;++i)crc|=uint32_t(bytes[100+i])<<(8*i);
  if(crc!=calibrationCrc(bytes,100)||!validFirmwareIdentity(context)||
     std::memcmp(bytes+64,context.firmwareSha256,32)!=0)return false;
  double trim=0;std::memcpy(&trim,bytes+8,8);
  if(!std::isfinite(trim)||trim!=context.pitchTrim)return false;
  SavedGyro result;
  for(int i=0;i<3;++i)std::memcpy(&result.bias[i],bytes+16+8*i,8);
  std::memcpy(&result.norm,bytes+40,8);std::memcpy(&result.accelStd,bytes+48,8);std::memcpy(&result.gyroStd,bytes+56,8);
  if(!validSavedGyro(result))return false;
  value=result;return true;
}
}
