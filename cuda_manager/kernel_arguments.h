#ifndef KERNEL_ARGUMENTS_H
#define KERNEL_ARGUMENTS_H

#include <stdlib.h>

namespace cuda_manager {

enum ArgType {
  BUFFER,
  SCALAR
};

struct Arg {
  ArgType type;
};

struct ScalarArg {
  ArgType type;
  void *ptr;
};

struct BufferArg {
  ArgType type;
  int id;
  bool is_in;
};

}

#endif
