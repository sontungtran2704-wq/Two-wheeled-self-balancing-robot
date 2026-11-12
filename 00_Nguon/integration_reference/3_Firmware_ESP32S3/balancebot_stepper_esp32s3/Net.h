//==============================================================================
//  Net - WiFi (STA nếu có WiFi đã lưu, nếu không tự phát AP) + cổng TCP 8080
//  (1 client). Lệnh nhận qua TCP dùng chung bộ lệnh với Serial.
//==============================================================================
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"
#include "LineReader.h"

struct Net {
  WiFiServer server;
  WiFiClient client;
  LineReader rx;
  char       mode[4];        // "OFF" / "STA" / "AP"
  char       ssid[33];
  char       pass[65];
  bool       fromNvs;        // WiFi đang dùng lấy từ NVS
  uint32_t   lastAccept;
  uint32_t   lastStaCheck;
  int        staFail;        // WiFi.status() lúc nối WiFi nhà thất bại (-1 = không thử/không lỗi)
  uint16_t   wrFail;         // số lần liên tiếp không gửi được cho client

  Net();
  void loadCreds();                                   // đọc WiFi đã lưu từ NVS
  void saveCreds();                                   // ghi WiFi hiện tại vào NVS
  bool setCreds(const char* newSsid, size_t ssidLen, const char* newPass);  // kiểm tra + lưu
  void begin();                                       // kết nối / phát AP, mDNS, mở cổng TCP
  void poll();                                        // nhận client mới, đọc lệnh từ TCP
  void print() const;
  void writeClient(const uint8_t* data, size_t n);    // gửi cho client (không chờ nếu đầy)
};

int bbResetReason();   // esp_reset_reason() (4=PANIC, 5/6/7=WDT, 15=BROWNOUT, 1=POWERON, 3=SW); 0 nếu không phải ESP32

extern Net net;
