//==============================================================================
//  LineReader - gom từng ký tự thành 1 dòng lệnh (kết thúc bằng \n hoặc \r)
//  Dùng chung cho Serial và TCP (mỗi kênh có 1 LineReader riêng).
//==============================================================================
#pragma once
#include <Arduino.h>

typedef void (*LineHandler)(char* line);

struct LineReader {
  char    buf[96];
  uint8_t len;

  LineReader() : len(0) { buf[0] = '\0'; }
  void reset() { len = 0; }

  void feed(char c, LineHandler handler) {
    if (c == '\n' || c == '\r') {
      if (len > 0) { buf[len] = '\0'; len = 0; handler(buf); }
    } else if (len < sizeof(buf) - 1) {
      buf[len++] = c;
    }
  }
};
