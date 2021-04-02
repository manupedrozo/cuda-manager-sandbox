#ifndef CUDA_MEMORY_MANAGER_H
#define CUDA_MEMORY_MANAGER_H
#include <map>
#include <string.h>
#include <cuda.h>


namespace cuda_manager {

// @TODO Properly handle errors, e.g when buffer doesnt exist

struct MemoryBuffer {
  int id;
  size_t size;
  CUdeviceptr d_ptr; // Ptr to device memory
};

struct MemoryKernel {
  int id;
  size_t size;
  void *ptr; // Ptr to host memory
};

class CudaMemoryManager {
private:
  // Separating kernels from buffers to allow for overlapping ids
  std::map<int, MemoryKernel> kernels;
  std::map<int, MemoryBuffer> buffers;

public:
  CudaMemoryManager() {}
  ~CudaMemoryManager() {}

  void allocate_kernel(int id, size_t size);
  void deallocate_kernel(int id);
  void write_kernel(int id, const void *data, size_t size);
  MemoryKernel get_kernel(int id);

  void allocate_buffer(int id, size_t size);
  void deallocate_buffer(int id);
  MemoryBuffer get_buffer(int id);
  void write_buffer(int id, const void *data, size_t size);
  void read_buffer(int id, void *buf, size_t size);
};

}

#endif
