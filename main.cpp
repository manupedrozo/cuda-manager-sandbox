// Lots of code duplication here, but thats on purpose

#include <cuda.h>
#include <nvrtc.h>
#include <vector>
#include <iostream>
#include "cuda_common.h"
#include "cuda_argument_parser.h"
#include "cuda_manager.h"
#include "cuda_compiler.h"

#define NUM_THREADS 128
#define NUM_BLOCKS 32 

const char *KERNEL_NAME     = "saxpy";
const char *KERNEL_FILENAME = "saxpy.cu";
const char *KERNEL_PATH     = "saxpy.cu";
const char *PTX_PATH        = "saxpy";

// Requires compiled ptx
void test_arg_parser() {
  CudaManager cuda_manager;

  // Setup input and output buffers
  size_t n = NUM_THREADS * NUM_BLOCKS;
  size_t buffer_size = n * sizeof(float);
  float a = 2.5f;
  float *x = new float[n], *y = new float[n], *o = new float[n];
  
  for (size_t i = 0; i < n; ++i) {
    x[i] = static_cast<float>(i);
    y[i] = static_cast<float>(i * 2);
  }

  Arg arg_a = {a, NULL, sizeof(float), false, true};
  Arg arg_x = {0, x, buffer_size, true, true};
  Arg arg_y = {0, y, buffer_size, true, true};
  Arg arg_o = {0, o, buffer_size, true, false};
  Arg arg_n = {(float)n, NULL, sizeof(size_t), false, true};

  std::vector<Arg> args {arg_a, arg_x, arg_y, arg_o, arg_n};

  // Arguments to a string
  std::cout << "Arguments to string: \n";
  std::string _arguments = args_to_string(KERNEL_NAME, args);
  const char *arguments = _arguments.c_str();

  // Parse arguments back from the string
  std::vector<Arg> parsed_args;
  char *kernel_path;
  bool succ = parse_arguments(arguments, parsed_args, &kernel_path);

  if (!succ) {
    exit(1);
  }

  std::cout << "Parsed arguments from string kernel [" << kernel_path << "]: \n";
  print_args(parsed_args);

  // Get ptx and kernel handle
  CudaCompiler cuda_compiler;
  char *ptx = cuda_compiler.read_ptx_from_file(PTX_PATH);

  CUmodule module;
  CUfunction kernel;
  CUDA_SAFE_CALL(cuModuleLoadDataEx(&module, ptx, 0, 0, 0));
  CUDA_SAFE_CALL(cuModuleGetFunction(&kernel, module, kernel_path));

  // Free PTX and kernel_path
  delete[] kernel_path;
  delete[] ptx;

  // Launch kernel with parsed arguments
  cuda_manager.launch_kernel(kernel, parsed_args, NUM_BLOCKS, NUM_THREADS);

  for (size_t i = 0; i < 10; ++i) { // first 10 results only
    std::cout << a << " * " << x[i] << " + " << y[i] << " = " << o[i] << '\n';
  }

  // Free resources
  CUDA_SAFE_CALL(cuModuleUnload(module));
  delete[] x;
  delete[] y;
  delete[] o;
}

void manager_launch_kernel_test() {
  CudaManager cuda_manager;

  // Compile and get a kernel handle
  CudaCompiler cuda_compiler;
  char *ptx = cuda_compiler.compile_to_ptx(KERNEL_PATH);

  CUmodule module;
  CUfunction kernel;
  CUDA_SAFE_CALL(cuModuleLoadDataEx(&module, ptx, 0, 0, 0));
  CUDA_SAFE_CALL(cuModuleGetFunction(&kernel, module, KERNEL_NAME));

  // Free PTX
  delete[] ptx;

  // Setup input and output buffers
  size_t n = NUM_THREADS * NUM_BLOCKS;
  size_t buffer_size = n * sizeof(float);
  float a = 2.5f;
  float *x = new float[n], *y = new float[n], *o = new float[n];
  
  for (size_t i = 0; i < n; ++i) {
    x[i] = static_cast<float>(i);
    y[i] = static_cast<float>(i * 2);
  }

  Arg arg_a = {a, NULL, sizeof(float), false, true};
  Arg arg_x = {0, x, buffer_size, true, true};
  Arg arg_y = {0, y, buffer_size, true, true};
  Arg arg_o = {0, o, buffer_size, true, false};
  Arg arg_n = {(float)n, NULL, sizeof(size_t), false, true};

  std::vector<Arg> args {arg_a, arg_x, arg_y, arg_o, arg_n};

  cuda_manager.launch_kernel(kernel, args, NUM_BLOCKS, NUM_THREADS);

  for (size_t i = 0; i < 10; ++i) { // first 10 results only
    std::cout << a << " * " << x[i] << " + " << y[i] << " = " << o[i] << '\n';
  }

  // Free resources
  CUDA_SAFE_CALL(cuModuleUnload(module));
  delete[] x;
  delete[] y;
  delete[] o;
}

void manual_launch_kernel_test() {
  // Initialize cuda manager and compiler
  CudaManager cuda_manager;
  CudaCompiler cuda_compiler;

  // Compile the file to a PTX
  char *_ptx = cuda_compiler.compile_to_ptx(KERNEL_PATH);

  // Save the PTX file and read it again (for testing purposes)
  cuda_compiler.save_ptx_to_file(_ptx, PTX_PATH);
  delete[] _ptx;

  char *ptx = cuda_compiler.read_ptx_from_file(PTX_PATH);

  // Load PTX and get a kernel handle
  CUmodule module;
  CUfunction kernel;
  CUDA_SAFE_CALL(cuModuleLoadDataEx(&module, ptx, 0, 0, 0));
  CUDA_SAFE_CALL(cuModuleGetFunction(&kernel, module, KERNEL_NAME));

  // Free PTX
  delete[] ptx;

  // Setup input and output buffers
  size_t n = NUM_THREADS * NUM_BLOCKS;
  size_t buffer_size = n * sizeof(float);
  float a = 2.5f;
  float *hX = new float[n], *hY = new float[n], *hOut = new float[n];
  
  for (size_t i = 0; i < n; ++i) {
    hX[i] = static_cast<float>(i);
    hY[i] = static_cast<float>(i * 2);
  }

  CUdeviceptr dX, dY, dOut;
  CUDA_SAFE_CALL(cuMemAlloc(&dX, buffer_size));
  CUDA_SAFE_CALL(cuMemAlloc(&dY, buffer_size));
  CUDA_SAFE_CALL(cuMemAlloc(&dOut, buffer_size));
  CUDA_SAFE_CALL(cuMemcpyHtoD(dX, hX, buffer_size));
  CUDA_SAFE_CALL(cuMemcpyHtoD(dY, hY, buffer_size));

  // Execute
  void *args[] = { &a, &dX, &dY, &dOut, &n };
  CUDA_SAFE_CALL(
    cuLaunchKernel(kernel, 
                   NUM_BLOCKS, 1, 1, // grid dim 
                   NUM_THREADS, 1, 1, // block dim
                   0, NULL, // shared mem, stream
                   args, 0) // args
  );
  // Syncronize
  CUDA_SAFE_CALL(cuCtxSynchronize());

  // Check results
  CUDA_SAFE_CALL(cuMemcpyDtoH(hOut, dOut, buffer_size));

  for (size_t i = 0; i < 10; ++i) { // first 10 results only
    std::cout << a << " * " << hX[i] << " + " << hY[i] << " = " << hOut[i] << '\n';
  }

  // Free resources
  CUDA_SAFE_CALL(cuMemFree(dX));
  CUDA_SAFE_CALL(cuMemFree(dY));
  CUDA_SAFE_CALL(cuMemFree(dOut));
  CUDA_SAFE_CALL(cuModuleUnload(module));
  delete[] hX;
  delete[] hY;
  delete[] hOut;
}

int main(void) {
  test_arg_parser();
  manager_launch_kernel_test();
  manual_launch_kernel_test();
}
