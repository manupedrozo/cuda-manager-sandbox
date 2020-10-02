#include "cuda_common.h"
#include "cuda_memory_manager.h"
#include <cuda.h>
#include <vector>

// Struct to keep track of buffers
struct CudaBuffer {
  void *h_ptr; // Ptr to host memory
  CUdeviceptr d_ptr; // Ptr to device memory
  size_t size;
  bool is_in; // Input or output buffer
};

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
  void launch_kernel_from_ptx(const char *ptx, const char* function_name, const std::vector<Arg *> args, const uint32_t num_blocks, const uint32_t num_threads);

  // Careful! this function will launch a kernel in the current context, if you are not manually managing contexts, do not use this function directly
  void launch_kernel(const CUfunction kernel, const std::vector<Arg *> args, const uint32_t num_blocks, const uint32_t num_threads); 
};
