#pragma once
#include <cstdint>
#include <stdexcept>
using portMUX_TYPE=int;
using TickType_t=uint32_t;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)(x))
#define portEXIT_CRITICAL(x) ((void)(x))
#define pdMS_TO_TICKS(x) (x)
namespace fake_imu {
struct End:std::exception{};
inline uint32_t clock=0,deadline=0;
inline void delay(uint32_t ms){clock+=ms*1000;if(clock>=deadline)throw End{};}
}
inline uint32_t micros(){return fake_imu::clock;}
inline TickType_t xTaskGetTickCount(){return micros()/1000;}
inline void vTaskDelay(uint32_t ms){fake_imu::delay(ms);}
inline void vTaskDelayUntil(TickType_t* wake,uint32_t ms){*wake+=ms;fake_imu::delay(ms);}
inline void vTaskDelete(void*){throw fake_imu::End{};}
