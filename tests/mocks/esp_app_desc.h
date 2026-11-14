#pragma once
#include <cstdint>
#define ESP_APP_DESC_MAGIC_WORD 0xABCD5432
struct esp_app_desc_t {uint32_t magic_word=ESP_APP_DESC_MAGIC_WORD;uint8_t app_elf_sha256[32]{1};};
namespace fake_imu {inline esp_app_desc_t app;}
inline const esp_app_desc_t* esp_app_get_description(){return &fake_imu::app;}
