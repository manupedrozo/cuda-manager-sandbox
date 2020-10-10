#ifndef KERNEL_ARGUMENTS_H
#define KERNEL_ARGUMENTS_H

#include <stdlib.h>

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