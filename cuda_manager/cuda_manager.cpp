#include "cuda_manager.h"
#include "cuda_common.h"
#include "kernel_arguments.h"
#include <cuda.h>
#include <vector>
#include <iostream>

namespace cuda_manager {

CudaManager::CudaManager(): memory_manager() {
  std::cout << "Initializing CUDA Manager...\n";
  CUDA_SAFE_CALL(cuInit(0));

  // Get devices info
  CUDA_SAFE_CALL(cuDeviceGetCount((int *)&device_count));
  std::cout << "Device count: " << device_count << '\n'; 

  devices = new CUdevice[device_count]();
  contexts = new CUcontext[device_count]();

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
  CUDA_SAFE_CALL(cuCtxSetCurrent(contexts[0]));
}

CudaManager::~CudaManager() {
  std::cout << "Destructing CUDA Manager...\n";
  for (int i = 0; i < device_count; ++i) {
    CUDA_SAFE_CALL(cuCtxDestroy(contexts[i]));
  }
}


void CudaManager::launch_kernel_from_ptx(const char *ptx, const char* function_name, CudaResourceArgs &r_args, const char *args, int arg_count) {
  // Set context where to launch the kernel
  CUDA_SAFE_CALL(cuCtxSetCurrent(contexts[r_args.device_id]));

  // Load module in current context and get kernel handle
  CUfunction kernel;
  CUmodule module;
  CUDA_SAFE_CALL(cuModuleLoadDataEx(&module, ptx, 0, 0, 0));
  CUDA_SAFE_CALL(cuModuleGetFunction(&kernel, module, function_name));

  // Launch kernel in current context
  launch_kernel(kernel, r_args, args, arg_count);

  // Unload module
  CUDA_SAFE_CALL(cuModuleUnload(module));
}


void CudaManager::launch_kernel(const CUfunction kernel, CudaResourceArgs &r_args, const char *args, int arg_count) {
  CUDA_SAFE_CALL(cuCtxSetCurrent(contexts[r_args.device_id]));
  void *kernel_args[arg_count]; // Args to be passed on kernel launch
  std::vector<CUdeviceptr *> buffers;

#ifndef NDEBUG
  std::cout << "Parsing arguments... [" << arg_count << "]\n";
#endif

  const char *current_arg = args;
  for (int i = 0; i < arg_count; ++i) {
    Arg *base = (Arg *) current_arg;

    switch (base->type) {
      case BUFFER: 
      {
        BufferArg *arg = (BufferArg *) base; 
        current_arg += sizeof(BufferArg);

#ifndef NDEBUG
        std::cout << "Buffer arg[1/2]: id = " << arg->id << "  is_in = " << arg->is_in << "\n";
#endif

        // Get memory buffer by id
        MemoryBuffer memory_buffer = memory_manager.get_buffer(arg->id);

#ifndef NDEBUG
        std::cout << "Buffer arg[2/2]: size = "  << memory_buffer.size << 
            "  d_ptr = " << (void *)memory_buffer.d_ptr << "\n";
#endif

        CUdeviceptr *cuptr = new CUdeviceptr;
        *cuptr = memory_buffer.d_ptr;
        buffers.push_back(cuptr);

        kernel_args[i] = (void *) cuptr;
        break;
      }
      case SCALAR:
      {
        ScalarArg *arg = (ScalarArg *) base;
        current_arg += sizeof(ScalarArg);

#ifndef NDEBUG
        std::cout << "Scalar arg: ptr = " << arg->ptr << "\n";
#endif
        kernel_args[i] = arg->ptr;

        break;
      }
    }
  }

#ifndef NDEBUG
  std::cout << "Executing...\n";
#endif
  // Execute
  CUDA_SAFE_CALL(
      cuLaunchKernel(kernel, 
        r_args.grid_dim.x , r_args.grid_dim.y , r_args.grid_dim.z , // grid dim 
        r_args.block_dim.x, r_args.block_dim.y, r_args.block_dim.z, // block dim
        0, NULL, // shared mem, stream
        kernel_args, 0) // args, extras
      );

  // Synchronize
  CUDA_SAFE_CALL(cuCtxSynchronize());

#ifndef NDEBUG
  std::cout << "Execution complete!\n";
#endif

  for (CUdeviceptr *cuptr: buffers) {
      delete cuptr;
  }
}



}
