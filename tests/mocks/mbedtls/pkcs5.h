#pragma once
// Host crypto backend: Windows PBKDF2-HMAC-SHA256, same inputs/output as the
// target mbedTLS API. Target builds use the real ESP32 SDK header/library.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <bcrypt.h>
#include <cstddef>
#include <cstdint>
constexpr int MBEDTLS_MD_SHA256=6;
inline int mbedtls_pkcs5_pbkdf2_hmac_ext(int,const unsigned char* password,size_t plen,const unsigned char* salt,size_t slen,unsigned rounds,uint32_t size,unsigned char* out) {
  BCRYPT_ALG_HANDLE algorithm=nullptr;
  NTSTATUS status=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,BCRYPT_ALG_HANDLE_HMAC_FLAG);
  if(status)return int(status);
  status=BCryptDeriveKeyPBKDF2(algorithm,const_cast<PUCHAR>(password),ULONG(plen),const_cast<PUCHAR>(salt),ULONG(slen),rounds,out,size,0);
  BCryptCloseAlgorithmProvider(algorithm,0);return int(status);
}
