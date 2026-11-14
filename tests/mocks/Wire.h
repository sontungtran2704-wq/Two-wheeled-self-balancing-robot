#pragma once
#include <array>
#include <cstdint>
namespace fake_imu {inline int16_t gyroY=65;inline bool readFailure=false;}
struct FakeWire {
  std::array<uint8_t,256> registers{};std::array<uint8_t,14> response{};
  int reg=0,writes=0,index=0;
  void begin(int,int,int){}
  void setTimeOut(int){}
  void beginTransmission(int){writes=0;}
  void write(uint8_t b){if(!writes)reg=b;else registers[reg]=b;++writes;}
  int endTransmission(bool=true){return 0;}
  int requestFrom(int,int n,bool){
    if(fake_imu::readFailure)return 0;
    response.fill(0);index=0;
    if(reg==0x3B){response[4]=0x20;response[10]=uint8_t(uint16_t(fake_imu::gyroY)>>8);response[11]=uint8_t(fake_imu::gyroY);}
    else if(reg==0x75)response[0]=0x70;
    else for(int i=0;i<n;++i)response[i]=registers[reg+i];
    return n;
  }
  uint8_t read(){return response[index++];}
};
inline FakeWire Wire;
