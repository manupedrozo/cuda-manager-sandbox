#ifndef CUDA_MEMORY_MANAGER_H
#define CUDA_MEMORY_MANAGER_H
#include <map>
#include <string.h>


namespace cuda_manager {

// @TODO Properly handle errors, e.g when buffer doesnt exist
struct MemoryBuffer {
  int id;
  size_t size;
  void *ptr;
};

class CudaMemoryManager {
private:
  // Separating kernels from buffers to allow for overlapping ids
  std::map<int, MemoryBuffer> kernels;
  std::map<int, MemoryBuffer> buffers;
  int next_id = 0;

public:
  CudaMemoryManager() {}
  ~CudaMemoryManager() {}

  void allocate_buffer(int id, size_t size, bool is_kernel, void **result_ptr = nullptr);
  int allocate_buffer(size_t size, bool is_kernel, void **result_ptr = nullptr);
  void deallocate_buffer(int id, bool is_kernel);
  MemoryBuffer get_buffer(int id, bool is_kernel);
  void write_buffer(int id, const void *data, size_t size, bool is_kernel);
  void read_buffer(int id, void *buf, size_t size, bool is_kernel);
};

}

#endif
