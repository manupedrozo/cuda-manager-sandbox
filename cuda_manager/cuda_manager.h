#ifndef CUDA_MANAGER_H
#define CUDA_MANAGER_H

#include "cuda_common.h"
#include "cuda_memory_manager.h"
#include <cuda.h>
#include <vector>

// Dimensions for grid and blocks
struct CudaDims {
  uint32_t x;
  uint32_t y;
  uint32_t z;
};

// Resource specific arguments for kernel launch
struct CudaResourceArgs {
    int device_id;
    CudaDims grid_dim;
    CudaDims block_dim;
};

namespace cuda_manager {

/*! \brief A class that manages devices, contexts and launches kernels.
 */
class CudaManager {
public:
  CudaMemoryManager memory_manager;

  uint32_t device_count = 0;
  // (Device, Context) pairs
  CUdevice *devices; 
  CUcontext *contexts;


  CudaManager();
  ~CudaManager();
  
  // Load kernel from a ptx and function name
  void launch_kernel_from_ptx(const char *ptx, const char* function_name, CudaResourceArgs &r_args, const char *args, int arg_count);

  // Careful! this function will launch a kernel in the current context, if you are not manually managing contexts, do not use this function directly
  void launch_kernel(const CUfunction kernel, CudaResourceArgs &r_args, const char *args, int arg_count);
};

}

#endif
