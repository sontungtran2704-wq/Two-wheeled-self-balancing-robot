#include <Arduino.h>
#include <stdarg.h>
#include "Log.h"
#include "Net.h"

void outf(const char* fmt, ...) {
  char b[200];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(b, sizeof b, fmt, ap);
  va_end(ap);
  if (n <= 0) return;
  if (n > (int)sizeof b - 1) n = (int)sizeof b - 1;
#if defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT
  if (Serial.availableForWrite() >= n)          // USB CDC không có host đọc: bỏ dòng thay vì chặn vòng điều khiển
#endif
  Serial.write((const uint8_t*)b, (size_t)n);
  net.writeClient((const uint8_t*)b, (size_t)n);
}

void outln(const char* s) { outf("%s\n", s); }
