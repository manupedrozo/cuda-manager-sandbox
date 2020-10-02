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

class Arg {
  public:
  size_t size;
  bool is_buffer;
  bool is_in;

  Arg(size_t size, bool is_buffer, bool is_in) {
    this->size = size;
    this->is_buffer = is_buffer;
    this->is_in = is_in;
  }

  virtual void *get_value_ptr() { return nullptr; }
  virtual int get_id() { return -1; }
};

template <class T>
class ValueArg : public Arg {
  public:
    T value;

    ValueArg(T value, size_t size, bool is_in): Arg(size, false, is_in) {
      this->value = value;
    }

    virtual void *get_value_ptr() {
      return (void *) &value;
    }
};

class BufferArg : public Arg {
  public:
    int id;

  BufferArg(int id, size_t size, bool is_in): Arg(size, true, is_in) {
    this->id = id;
  }

  virtual int get_id() {
    return id;
  }
};

}

#endif

