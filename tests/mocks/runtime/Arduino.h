#pragma once
#include "../Arduino.h"
#include <cstring>
#include <deque>
#include <memory>
#include <string>
#include <vector>
constexpr int pdTRUE=1,pdPASS=1;
struct FakeQueue {size_t capacity,size;std::deque<std::vector<uint8_t>> values;};
using QueueHandle_t=FakeQueue*;
namespace fake_runtime {
inline std::vector<std::unique_ptr<FakeQueue>> queues;
inline std::string serialIn,serialOut,tcpOut;
inline unsigned restarts=0;
struct Restart:std::exception{};
}
inline QueueHandle_t xQueueCreate(size_t count,size_t size) {
  fake_runtime::queues.push_back(std::make_unique<FakeQueue>(FakeQueue{count,size,{}}));
  return fake_runtime::queues.back().get();
}
inline int xQueueSend(QueueHandle_t q,const void* p,int) {
  if(q->values.size()==q->capacity)return 0;
  const auto* b=static_cast<const uint8_t*>(p);q->values.emplace_back(b,b+q->size);return pdTRUE;
}
inline int xQueueReceive(QueueHandle_t q,void* p,int) {
  if(q->values.empty())return 0;
  std::memcpy(p,q->values.front().data(),q->size);q->values.pop_front();return pdTRUE;
}
inline void xQueueReset(QueueHandle_t q){q->values.clear();}
inline int xTaskCreatePinnedToCore(void(*)(void*),const char*,int,void*,int,void*,int){return pdPASS;}
inline uint32_t millis(){return micros()/1000;}
inline uint32_t esp_random(){return 77;}
struct FakeSerial {
  void begin(int){}
  int available(){return int(fake_runtime::serialIn.size());}
  char read(){char c=fake_runtime::serialIn.front();fake_runtime::serialIn.erase(0,1);return c;}
  void write(const uint8_t* b,size_t n){fake_runtime::serialOut.append(reinterpret_cast<const char*>(b),n);}
  void write(char c){fake_runtime::serialOut+=c;}
};
inline FakeSerial Serial;
struct FakeESP {void restart(){++fake_runtime::restarts;throw fake_runtime::Restart{};}};
inline FakeESP ESP;
