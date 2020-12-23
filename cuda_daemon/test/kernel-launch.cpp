#include <stdio.h>
#include <iostream> 

#include "cuda_client.h"
#include "cuda_compiler.h"

// Including these for easy arg parsing
#include "kernel_arguments.h"
#include "cuda_argument_parser.h"


#define SOCKET_PATH "/tmp/server-test"
const char *KERNEL_PATH = "saxpy.cu";
const char *KERNEL_NAME = "saxpy"; // Name of the function inside the kernel file

using namespace cuda_daemon;
using namespace cuda_manager;

int main(int argc, char const *argv[]) { 

  CudaClient client(SOCKET_PATH); 

  // Compile kernel to ptx
  CudaCompiler cuda_compiler;
  char *ptx;
  size_t ptx_size;
  cuda_compiler.compile_to_ptx(KERNEL_PATH, &ptx, &ptx_size);

  // Send ptx and get mem_id handle
  int kernel_mem_id;
  // TODO this should be a kernel allocation, not a memory allocation, not currently implemented in client/server
  client.memory_allocate(ptx_size, &kernel_mem_id);
  client.memory_write(kernel_mem_id, (void *) ptx, ptx_size);

  delete[] ptx;
  

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

  client.memory_write(xid, (void *) x, buffer_size);
  client.memory_write(yid, (void *) y, buffer_size);

  // Doing it this way to easily convert them to string, in reality you need to manually create the string
  ValueArg  arg_a = {VALUE, a};
  BufferArg arg_x = {BUFFER, xid, true};
  BufferArg arg_y = {BUFFER, yid, true};
  BufferArg arg_o = {BUFFER, oid, false};
  ValueArg  arg_n = {VALUE, (float)n};

  std::vector<void *> args {(void *)&arg_a, (void *)&arg_x, (void *)&arg_y, (void *)&arg_o, (void *)&arg_n};

  // Arguments to a string
  std::cout << "Arguments to string: \n";
  std::string _arguments = args_to_string(KERNEL_NAME, kernel_mem_id, args);
  size_t arg_size = _arguments.size() + 1; // + 1 for null terminator
  char *arguments = (char *) _arguments.c_str();
  std::cout << "Arg size: " << arg_size << "\n";
  std::cout << arguments << "\n";

  // Launch kernel
  client.launch_kernel(arguments, arg_size);

  client.memory_read(oid, (void *) o, buffer_size);

  for (size_t i = 0; i < 10; ++i) { // first 10 results only
    std::cout << a << " * " << x[i] << " + " << y[i] << " = " << o[i] << '\n';
  }

  // TODO deallocate kernel and buffers, currently not implemented

  delete[] x;
  delete[] y;
  delete[] o;
}
