#include "cuda_manager.h"
#include <cuda.h>
#include <nvrtc.h>
#include <fstream>
#include <iostream>

CudaManager::CudaManager() {
  std::cout << "Initializing CUDA Manager...\n";
  CUDA_SAFE_CALL(cuInit(0));

  // Get devices info
  int device_count = 0;
  CUDA_SAFE_CALL(cuDeviceGetCount(&device_count));
  std::cout << "Device count: " << device_count << '\n'; 

  CUdevice  devices[device_count];
  CUcontext contexts[device_count];

  int major = 0, minor = 0;
  char device_name[256];
  for (int i = 0; i < device_count; ++i) {
    // Get device
    CUDA_SAFE_CALL(cuDeviceGet(&devices[i], i));

    CUDA_SAFE_CALL(cuDeviceGetAttribute(&major, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, devices[i]));
    CUDA_SAFE_CALL(cuDeviceGetAttribute(&minor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, devices[i]));
    CUDA_SAFE_CALL(cuDeviceGetName(device_name, 256, devices[i]));
    std::cout << i << ") GPU name: " << device_name << "  (SM: " << major << '.' << minor << ")\n"; 

    // Initialize context for device
    CUDA_SAFE_CALL(cuCtxCreate(&contexts[i], i, devices[i]));
  }

  // We are only using one device & context for now
  context = contexts[0];
  device = devices[0];

}

CudaManager::~CudaManager() {
  std::cout << "Destructing CUDA Manager...\n";
  CUDA_SAFE_CALL(cuCtxDestroy(context));
}

void CudaManager::compile_to_ptx(const char *source_path, char **result_ptx) {
  std::cout << "Compiling cuda kernel file [" << source_path << "]...\n";
  // Read kernel file
  std::ifstream input_file(source_path, std::ifstream::in | std::ifstream::ate);

  if (!input_file.is_open()) {
    std::cerr << "Unable to open file\n";
    exit(1);
  }

  size_t input_size = (size_t)input_file.tellg();
  char *kernel_string = new char[input_size + 1];

  input_file.seekg(0, std::ifstream::beg);
  input_file.read(kernel_string, input_size);
  input_file.close();

  kernel_string[input_size] = '\x0';

  // Create nvrtc program for compilation
  nvrtcProgram prog;
  // TODO Check if program name (3rd param) is needed, "default_program" is used when null.
  NVRTC_SAFE_CALL(
      nvrtcCreateProgram(&prog, kernel_string, NULL, 0, NULL, NULL) 
  );

  delete[] kernel_string;

  // Compilation options
  const char *opts[] = {"--fmad=false"};
  // Compile the program
  nvrtcResult compile_result = nvrtcCompileProgram(prog, 1, opts);

  // Get compilation log
  size_t log_size;
  NVRTC_SAFE_CALL(
    nvrtcGetProgramLogSize(prog, &log_size)
  );
  char *log = new char[log_size];
  NVRTC_SAFE_CALL(nvrtcGetProgramLog(prog, log));
  std::cout << log;
  delete[] log;

  if (compile_result != NVRTC_SUCCESS) {
    exit(1);
  }
  std::cout << "Compilation successful\n";

  // Get PTX from the program
  size_t ptx_size;
  NVRTC_SAFE_CALL(nvrtcGetPTXSize(prog, &ptx_size));
  char *ptx = new char[ptx_size];
  NVRTC_SAFE_CALL(nvrtcGetPTX(prog, ptx));

  *result_ptx = ptx;

  // Destroy the program
  NVRTC_SAFE_CALL(nvrtcDestroyProgram(&prog));
}

