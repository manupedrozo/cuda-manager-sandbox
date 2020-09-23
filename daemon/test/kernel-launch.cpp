#include <stdio.h> 

#include "commands.h"
#include "cuda_client.h"

// Including these for easy arg parsing
#include "cuda_common.h"
#include "cuda_argument_parser.h"


#define SOCKET_PATH "/tmp/server-test"
const char *KERNEL_NAME = "saxpy";

using namespace cuda_mango;

int main(int argc, char const *argv[]) { 

  CudaClient client(SOCKET_PATH); 

  size_t n = 100;
  size_t buffer_size = n * sizeof(float);
  float a = 2.5f;
  float *x = new float[n], *y = new float[n], *o = new float[n];

  for (size_t i = 0; i < n; ++i) {
    x[i] = static_cast<float>(i);
    y[i] = static_cast<float>(i * 2);
  }

  // Allocate and write buffers
  int xid, yid, oid;
  client.memory_allocate(buffer_size, &xid);
  client.memory_allocate(buffer_size, &yid);
  client.memory_allocate(buffer_size, &oid);
  printf("Allocated buffer x id: %d\n", xid);
  printf("Allocated buffer y id: %d\n", yid);
  printf("Allocated buffer o id: %d\n", oid);

  // Arguments
  ValueArg<float> arg_a(a, sizeof(float), true);
  BufferArg arg_x(xid, buffer_size, true);
  BufferArg arg_y(yid, buffer_size, true);
  BufferArg arg_o(oid, buffer_size, false);
  ValueArg<float> arg_n(n, sizeof(float), true);

  std::vector<Arg *> args {&arg_a, &arg_x, &arg_y, &arg_o, &arg_n};

  // Arguments to a string
  std::cout << "Arguments to string: \n";
  std::string _arguments = args_to_string(KERNEL_NAME, args);
  size_t arg_size = _arguments.size();
  char *arguments = (char *) _arguments.c_str();
  std::cout << arguments << "\n";
  client.launch_kernel(arguments, arg_size);


}
