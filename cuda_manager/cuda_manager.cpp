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
}

CudaManager::~CudaManager() {
  std::cout << "Destructing CUDA Manager...\n";
  for (int i = 0; i < device_count; ++i) {
    CUDA_SAFE_CALL(cuCtxDestroy(contexts[i]));
  }
}


void CudaManager::launch_kernel_from_ptx(const char *ptx, const char* function_name, const char *args, int arg_count, const uint32_t num_blocks, const uint32_t num_threads) {
  // Set context where to launch the kernel
  CUDA_SAFE_CALL(cuCtxSetCurrent(contexts[0])); // TODO just using one context for now

  // Load module in current context and get kernel handle
  CUfunction kernel;
  CUmodule module;
  CUDA_SAFE_CALL(cuModuleLoadDataEx(&module, ptx, 0, 0, 0));
  CUDA_SAFE_CALL(cuModuleGetFunction(&kernel, module, function_name));

  // Launch kernel in current context
  launch_kernel(kernel, args, arg_count, num_blocks, num_threads);

  // Unload module
  CUDA_SAFE_CALL(cuModuleUnload(module));
}


void CudaManager::launch_kernel(const CUfunction kernel, const char *args, int arg_count, const uint32_t num_blocks, const uint32_t num_threads) {
  void *kernel_args[arg_count]; // Args to be passed on kernel launch
  std::vector<CudaBuffer*> buffers; // Keep track of buffers 

  std::cout << "Parsing arguments... [" << arg_count << "]\n";

  const char *current_arg = args;
  for (int i = 0; i < arg_count; ++i) {
    Arg *base = (Arg *) current_arg;

    switch (base->type) {
      case BUFFER: 
      {
        BufferArg *arg = (BufferArg *) base; 
        current_arg += sizeof(BufferArg);

        std::cout << "Buffer arg[1/2]: id = " << arg->id << "  is_in = " << arg->is_in << "\n";

        // Create cuda buffer and copy to device if its an input buffer
        // The buffer must be created inside the vector and then used as a reference
        // This allows us to use pointers to its members (d_ptr)

        CudaBuffer *cuda_buffer = new CudaBuffer();
        buffers.push_back(cuda_buffer);


        // Get memory buffer by id
        MemoryBuffer memory_buffer = memory_manager.get_buffer(arg->id, false);

        cuda_buffer->h_ptr = memory_buffer.ptr;
        cuda_buffer->size  = memory_buffer.size;
        cuda_buffer->is_in = arg->is_in;

        CUDA_SAFE_CALL(cuMemAlloc(&cuda_buffer->d_ptr, cuda_buffer->size));

        std::cout << "Buffer arg[2/2]: h_ptr = " << cuda_buffer->h_ptr << 
          "  size = "  << cuda_buffer->size << 
          "  d_ptr = " << (void *)&cuda_buffer->d_ptr << "\n";

        if (cuda_buffer->is_in) {
          CUDA_SAFE_CALL(cuMemcpyHtoD(cuda_buffer->d_ptr, cuda_buffer->h_ptr, cuda_buffer->size));
          std::cout << "Copied HtoD " << cuda_buffer->h_ptr << " to " << cuda_buffer->d_ptr << "\n";
        }

        kernel_args[i] = (void *) &cuda_buffer->d_ptr;
        break;
      }
      case VALUE:
      {
        ValueArg *arg = (ValueArg *) base;
        current_arg += sizeof(ValueArg);

        // float only for now
        std::cout << "Scalar arg: value = " << arg->value << "\n";

        // Use address of the argument in the original array
        // Might need to allocate memory for the value so arguments can be freed right away
        kernel_args[i] = &arg->value;

        break;
      }
    }
  }

  std::cout << "Executing...\n";
  // Execute
  CUDA_SAFE_CALL(
      cuLaunchKernel(kernel, 
        num_blocks, 1, 1, // grid dim 
        num_threads, 1, 1, // block dim
        0, NULL, // shared mem, stream
        kernel_args, 0) // args, extras
      );

  // Synchronize
  CUDA_SAFE_CALL(cuCtxSynchronize());

  std::cout << "Execution complete!\n";

  // Copy back to host and free host/device memory
  for (CudaBuffer *cuda_buffer: buffers) {
    if (!cuda_buffer->is_in) {
      std::cout << "Copied DtoH " << cuda_buffer->d_ptr << " to " << cuda_buffer->h_ptr << "\n";
      CUDA_SAFE_CALL(cuMemcpyDtoH(cuda_buffer->h_ptr, cuda_buffer->d_ptr, cuda_buffer->size));
    }
    std::cout << "Deallocated " << cuda_buffer->d_ptr << "\n";
    CUDA_SAFE_CALL(cuMemFree(cuda_buffer->d_ptr));
    free(cuda_buffer);
  }
}



}
