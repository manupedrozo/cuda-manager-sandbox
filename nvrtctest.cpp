#include <cuda.h>
#include <nvrtc.h>
#include <iostream>

#define NUM_THREADS 128
#define NUM_BLOCKS 32 

#define NVRTC_SAFE_CALL(x)                                        \
  do {                                                            \
    nvrtcResult result = x;                                       \
    if (result != NVRTC_SUCCESS) {                                \
      std::cerr << "\nerror: " #x " failed with error "           \
                << nvrtcGetErrorString(result) << '\n';           \
      exit(1);                                                    \
    }                                                             \
  } while(0)
#define CUDA_SAFE_CALL(x)                                         \
  do {                                                            \
    CUresult result = x;                                          \
    if (result != CUDA_SUCCESS) {                                 \
      const char *msg;                                            \
      cuGetErrorName(result, &msg);                               \
      std::cerr << "\nerror: " #x " failed with error "           \
                << msg << '\n';                                   \
      exit(1);                                                    \
    }                                                             \
  } while(0)

const char *KERNEL_STRING = "                                   \n\
extern \"C\" __global__                                         \n\
void saxpy(float a, float *x, float *y, float *out, size_t n)   \n\
{                                                               \n\
  size_t tid = blockIdx.x * blockDim.x + threadIdx.x;           \n\
  if (tid < n) {                                                \n\
    out[tid] = a * x[tid] + y[tid];                             \n\
  }                                                             \n\
}                                                               \n";
const char *KERNEL_NAME = "saxpy";

int main(void) {
  // Create nvrtc program for compilation
  nvrtcProgram prog;
  // Program, kernel, name, number of headers, headers, include names
  NVRTC_SAFE_CALL(
      nvrtcCreateProgram(&prog, KERNEL_STRING, "program.cu", 0, NULL, NULL)
  );

  // Compilation options
  const char *opts[] = {"--fmad=false"};
  // Compile the program
  nvrtcResult compileResult = nvrtcCompileProgram(prog, 1, opts);

  // Get compilation log
  size_t logSize;
  NVRTC_SAFE_CALL(
    nvrtcGetProgramLogSize(prog, &logSize)
  );
  char *log = new char[logSize];
  NVRTC_SAFE_CALL(nvrtcGetProgramLog(prog, log));
  std::cout << "Compilation log: \n" << log << "End of compilation\n";
  delete[] log;

  if (compileResult != NVRTC_SUCCESS) {
    exit(1);
  }

  // Get PTX from the program
  size_t ptxSize;
  NVRTC_SAFE_CALL(nvrtcGetPTXSize(prog, &ptxSize));
  char *ptx = new char[ptxSize];
  NVRTC_SAFE_CALL(nvrtcGetPTX(prog, ptx));

  // We dont need the program any more
  NVRTC_SAFE_CALL(nvrtcDestroyProgram(&prog));

  // Load the PTX and get a handle to the kernel
  CUdevice cuDevice;
  CUcontext context;
  CUmodule module;
  CUfunction kernel;
  CUDA_SAFE_CALL(cuInit(0));
  CUDA_SAFE_CALL(cuDeviceGet(&cuDevice, 0));
  CUDA_SAFE_CALL(cuCtxCreate(&context, 0, cuDevice));
  CUDA_SAFE_CALL(cuModuleLoadDataEx(&module, ptx, 0, 0, 0));
  CUDA_SAFE_CALL(cuModuleGetFunction(&kernel, module, KERNEL_NAME));


  int deviceId;
  cudaGetDevice(&deviceId);
  std::cout << deviceId << " <- device id\n";

  // Setup input and output buffers
  size_t n = NUM_THREADS * NUM_BLOCKS;
  size_t bufferSize = n * sizeof(float);
  float a = 5.1f;
  float *hX = new float[n], *hY = new float[n], *hOut = new float[n];
  
  for (size_t i = 0; i < n; ++i) {
    hX[i] = static_cast<float>(i);
    hY[i] = static_cast<float>(i * 2);
  }

  CUdeviceptr dX, dY, dOut;
  CUDA_SAFE_CALL(cuMemAlloc(&dX, bufferSize));
  CUDA_SAFE_CALL(cuMemAlloc(&dY, bufferSize));
  CUDA_SAFE_CALL(cuMemAlloc(&dOut, bufferSize));
  CUDA_SAFE_CALL(cuMemcpyHtoD(dX, hX, bufferSize));
  CUDA_SAFE_CALL(cuMemcpyHtoD(dY, hY, bufferSize));

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
  CUDA_SAFE_CALL(cuMemcpyDtoH(hOut, dOut, bufferSize));
  for (size_t i = 0; i < n; ++i) {
    std::cout << a << " * " << hX[i] << " + " << hY[i] << " = " << hOut[i] << '\n';
  }

  // Free resources
  CUDA_SAFE_CALL(cuMemFree(dX));
  CUDA_SAFE_CALL(cuMemFree(dY));
  CUDA_SAFE_CALL(cuMemFree(dOut));
  CUDA_SAFE_CALL(cuModuleUnload(module));
  CUDA_SAFE_CALL(cuCtxDestroy(context));
  delete[] hX;
  delete[] hY;
  delete[] hOut;

  return 0;
}
