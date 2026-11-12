//==============================================================================
//  Log - đầu ra chung: Serial + client TCP (nếu có). Không bao giờ chờ nếu
//  bộ đệm TCP đầy.
//==============================================================================
#pragma once

void outf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void outln(const char* s);
