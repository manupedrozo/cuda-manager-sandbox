#include "cuda_manager.h"
#include "cuda_common.h"
#include <cuda.h>
#include <vector>
#include <iostream>

CudaManager::CudaManager() : memory_manager() {
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

void CudaManager::launch_kernel(const CUfunction kernel, const std::vector<Arg *> args, const uint32_t num_blocks, const uint32_t num_threads) {
  // Set context where to launch the kernel
  CUDA_SAFE_CALL(cuCtxSetCurrent(contexts[0])); // TODO move this

  void *kernel_args[args.size()]; // Args to be passed on kernel launch
  std::vector<CudaBuffer> buffers; // Keep track of buffers 

  std::cout << "Parsing arguments...\n";
  for (int i = 0; i < args.size(); ++i) {
    Arg *arg = args[i];
    if (arg->is_buffer) {
      MemoryBuffer mem_buffer = memory_manager.get_buffer(arg->get_id());

      // Create cuda buffer and copy to device if its an input buffer
      // The buffer must be created inside the vector and then used as a reference
      // This allows us to use pointers to its members (d_ptr)
      buffers.push_back(CudaBuffer());
      CudaBuffer &cuda_buffer = buffers[buffers.size() - 1];

      cuda_buffer.h_ptr = mem_buffer.ptr;
      cuda_buffer.size  = mem_buffer.size;
      cuda_buffer.is_in = arg->is_in;
      
      CUDA_SAFE_CALL(cuMemAlloc(&cuda_buffer.d_ptr, cuda_buffer.size));

      std::cout << "Buffer arg: h_ptr = " << cuda_buffer.h_ptr << 
                             "  size = " << cuda_buffer.size << 
                             "  is_in = " << cuda_buffer.is_in << 
                             "  d_ptr = " << cuda_buffer.d_ptr << "\n";

      if (cuda_buffer.is_in) {
        std::cout << "Copied HtoD " << cuda_buffer.h_ptr << " to " << cuda_buffer.d_ptr << "\n";
        CUDA_SAFE_CALL(cuMemcpyHtoD(cuda_buffer.d_ptr, cuda_buffer.h_ptr, cuda_buffer.size));
      }

      kernel_args[i] = (void *) &cuda_buffer.d_ptr;
    }
    else {
      // Might print incorrect value since we are assuming float, but checking type is overkill
      std::cout << "Scalar arg: value = " << *(float *)arg->get_value_ptr() << 
                              " ptr = " << arg->get_value_ptr() << "\n";

      // Use address of the argument in the original array
      kernel_args[i] = args[i]->get_value_ptr();
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
  for (CudaBuffer &cuda_buffer: buffers) {
    if (!cuda_buffer.is_in) {
      std::cout << "Copied DtoH " << cuda_buffer.d_ptr << " to " << cuda_buffer.h_ptr << "\n";
      CUDA_SAFE_CALL(cuMemcpyDtoH(cuda_buffer.h_ptr, cuda_buffer.d_ptr, cuda_buffer.size));
    }
    std::cout << "Deallocated " << cuda_buffer.d_ptr << "\n";
    CUDA_SAFE_CALL(cuMemFree(cuda_buffer.d_ptr));
  }
}
