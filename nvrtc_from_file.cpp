#include <cuda.h>
#include <nvrtc.h>
#include <iostream>
#include <fstream>

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

const char *KERNEL_NAME = "saxpy";
const char *KERNEL_FILENAME = "saxpy.cu";
const char *KERNEL_PATH = "saxpy.cu";

int main(void) {
  // Read kernel file
  std::ifstream inputFile(KERNEL_PATH, std::ifstream::in | std::ifstream::ate);

  if (!inputFile.is_open()) {
    std::cerr << "Unable to open file \n";
    exit(1);
  }

  size_t inputSize = (size_t)inputFile.tellg();
  char *kernelString = new char[inputSize + 1];

  inputFile.seekg(0, std::ios::beg);
  inputFile.read(kernelString, inputSize);
  inputFile.close();

  kernelString[inputSize] = '\x0';

  std::cout << "Kernel:\n" << kernelString << "\n";

  // Create nvrtc program for compilation
  nvrtcProgram prog;
  // Program, kernel, name, number of headers, headers, include names
  NVRTC_SAFE_CALL(
      nvrtcCreateProgram(&prog, kernelString, KERNEL_FILENAME, 0, NULL, NULL)
  );

  delete[] kernelString;

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

  delete[] ptx;

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

  for (size_t i = 0; i < 10; ++i) { // first 10 results only
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
