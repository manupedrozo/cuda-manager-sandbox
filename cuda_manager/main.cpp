// Lots of code duplication here, but thats on purpose

#include <cuda.h>
#include <nvrtc.h>
#include <vector>
#include <iostream>
#include "cuda_common.h"
#include "cuda_argument_parser.h"
#include "cuda_api.h"
#include "cuda_manager.h"
#include "cuda_compiler.h"

#define NUM_THREADS 128
#define NUM_BLOCKS 32 

const char *KERNEL_NAME     = "saxpy";
const char *KERNEL_FILENAME = "saxpy.cu";
const char *KERNEL_PATH     = "saxpy.cu";
const char *PTX_PATH        = "saxpy";

using namespace cuda_manager;

void saxpy(float a, float *x, float *y, float *o, float n) {
    for (int i = 0; i < n; ++i) {
        o[i] = a * x[i] + y[i];
    }
}

void manual_launch_kernel_test() {
  // Initialize cuda manager and compiler
  CudaManager cuda_manager;
  CudaCompiler cuda_compiler;

  // Compile the file to a PTX
  char *_ptx;
  cuda_compiler.compile_to_ptx(KERNEL_PATH, &_ptx);

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
  
  for (int i = 0; i < n; ++i) {
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

  for (int i = 0; i < 10; ++i) { // first 10 results only
    std::cout << a << " * " << hX[i] << " + " << hY[i] << " = " << hOut[i] << '\n';
  }

  float *expected = new float[n];
  saxpy(a, hX, hY, expected, n);

  bool correct = true;
  for (int i = 0; i < n; ++i) {
      if (hOut[i] != expected[i]) {
          printf("Sample host: Incorrect value at %d: got %.2f vs %.2f\n", i, hOut[i], expected[i]);
          std::cout << "Sample host: Stopping...\n" << std::endl;
          correct = false;
          break;
      }
  }
  if(correct) {
      std::cout << "Sample host: SAXPY correctly performed" << std::endl;
  }

  // Free resources
  CUDA_SAFE_CALL(cuMemFree(dX));
  CUDA_SAFE_CALL(cuMemFree(dY));
  CUDA_SAFE_CALL(cuMemFree(dOut));
  CUDA_SAFE_CALL(cuModuleUnload(module));
  delete[] hX;
  delete[] hY;
  delete[] hOut;
  delete[] expected;
}

void test_api() {
  CudaApi cuda_api;
  CudaCompiler cuda_compiler;

  // Compile kernel to ptx
  char *ptx;
  size_t ptx_size;
  cuda_compiler.compile_to_ptx(KERNEL_PATH, &ptx, &ptx_size);

  // Allocate and write ptx
  int kernel_id = 0;
  cuda_api.allocate_kernel(kernel_id, ptx_size);
  cuda_api.write_kernel(kernel_id, (void *) ptx, ptx_size);

  delete[] ptx;

  // Setup input and output buffers
  size_t n = 100;
  size_t buffer_size = n * sizeof(float);
  float a = 2.5f;
  float *x = new float[n], *y = new float[n], *o = new float[n];

  for (int i = 0; i < n; ++i) {
    x[i] = static_cast<float>(i);
    y[i] = static_cast<float>(i * 2);
  }

  // Allocate and write buffers
  int xid = 0;
  int yid = 1;
  int oid = 2;
  cuda_api.allocate_memory(xid, buffer_size);
  cuda_api.allocate_memory(yid, buffer_size);
  cuda_api.allocate_memory(oid, buffer_size);
  printf("Allocated buffer x id: %d\n", xid);
  printf("Allocated buffer y id: %d\n", yid);
  printf("Allocated buffer o id: %d\n", oid);

  cuda_api.write_memory(xid, (void *) x, buffer_size);
  cuda_api.write_memory(yid, (void *) y, buffer_size);

  // Set up arguments
  int arg_count = 5;
  char *args = (char *) malloc(sizeof(ValueArg) * 2 + sizeof(BufferArg) * 3);
  char *current_arg = args;

  ValueArg *arg_a = (ValueArg *) current_arg;
  *arg_a = {VALUE, a};
  current_arg += sizeof(ValueArg);

  BufferArg *arg_x = (BufferArg *) current_arg;
  *arg_x = {BUFFER, xid, true};
  current_arg += sizeof(BufferArg);

  BufferArg *arg_y = (BufferArg *) current_arg;
  *arg_y = {BUFFER, yid, true};
  current_arg += sizeof(BufferArg);

  BufferArg *arg_o = (BufferArg *) current_arg;
  *arg_o = {BUFFER, oid, false};
  current_arg += sizeof(BufferArg);

  ValueArg *arg_n = (ValueArg *) current_arg;
  *arg_n = {VALUE, (float)n};
  current_arg += sizeof(ValueArg);

  CudaResourceArgs r_args = {0, {NUM_BLOCKS,1,1}, {NUM_THREADS,1,1}};
 
  // Launch kernel
  cuda_api.launch_kernel(kernel_id, KERNEL_NAME, r_args, args, arg_count);

  cuda_api.read_memory(oid, (void *)o, buffer_size);

  for (int i = 0; i < 10; ++i) { // first 10 results only
    std::cout << a << " * " << x[i] << " + " << y[i] << " = " << o[i] << '\n';
  }

  float *expected = new float[n];
  saxpy(a, x, y, expected, n);

  bool correct = true;
  for (int i = 0; i < n; ++i) {
      if (o[i] != expected[i]) {
          printf("Sample host: Incorrect value at %d: got %.2f vs %.2f\n", i, o[i], expected[i]);
          std::cout << "Sample host: Stopping...\n" << std::endl;
          correct = false;
          break;
      }
  }
  if(correct) {
      std::cout << "Sample host: SAXPY correctly performed" << std::endl;
  }

  cuda_api.deallocate_kernel(kernel_id);
  cuda_api.deallocate_memory(xid);
  cuda_api.deallocate_memory(yid);
  cuda_api.deallocate_memory(oid);

  free(args);
  delete[] x;
  delete[] y;
  delete[] o;
  delete[] expected;
}

int main(void) {
  test_api();
  //manual_launch_kernel_test();
}
