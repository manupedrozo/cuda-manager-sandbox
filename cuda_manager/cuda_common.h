#ifndef CUDA_COMMON
#define CUDA_COMMON

#include <nvrtc.h>
#include <cuda.h>
#include <iostream>

#define CUDA_SAFE_CALL(x)                                         \
  do {                                                            \
    CUresult result = x;                                          \
    if (result != CUDA_SUCCESS) {                                 \
      const char *msg;                                            \
      cuGetErrorName(result, &msg);                               \
      std::cerr << "error: " #x " failed with error "             \
                << msg << '\n';                                   \
      exit(1);                                                    \
    }                                                             \
  } while(0)

#define NVRTC_SAFE_CALL(x)                                        \
  do {                                                            \
    nvrtcResult result = x;                                       \
    if (result != NVRTC_SUCCESS) {                                \
      std::cerr << "error: " #x " failed with error "             \
                << nvrtcGetErrorString(result) << '\n';           \
      exit(1);                                                    \
    }                                                             \
  } while(0)

namespace cuda_manager {

enum ArgType {
  BUFFER,
  VALUE
};

struct Arg {
  ArgType type;
};

struct ValueArg {
  ArgType type;
  float value;
};

struct BufferArg {
  ArgType type;
  void *ptr;
  int id;
  size_t size;
  bool is_in;
};

}

#endif

