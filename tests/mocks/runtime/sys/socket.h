#pragma once
#include <cstddef>
constexpr int MSG_DONTWAIT=1;
inline int send(int,const char* data,size_t size,int){fake_runtime::tcpOut.append(data,size);return int(size);}
