//==============================================================================
//  Commands - bộ lệnh dùng chung cho Serial và TCP (xem danh sách trong .ino)
//==============================================================================
#pragma once

void handleCommand(char* line);   // xử lý 1 dòng lệnh (đã bỏ ký tự xuống dòng)
void pollSerialCommands();        // đọc lệnh từ Serial
