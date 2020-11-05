#ifndef CUDA_API_H
#define CUDA_API_H

#include "cuda_manager.h"

enum CudaApiExitCode {
  OK,
  ERROR
};

class CudaApi {
private:
  cuda_manager::CudaManager cuda_manager;
  cuda_manager::CudaMemoryManager memory_manager;

public:
  CudaApi();
  ~CudaApi();

  CudaApiExitCode allocate_memory(int buffer_id, size_t size);
  CudaApiExitCode deallocate_memory(int buffer_id);
  CudaApiExitCode write_memory(int buffer_id, const void *data, size_t size);
  CudaApiExitCode read_memory(int buffer_id, void *dest_buffer, size_t size);

  CudaApiExitCode allocate_kernel(int kernel_id, size_t size);
  CudaApiExitCode deallocate_kernel(int kernel_id);
  CudaApiExitCode write_kernel(int kernel_id, const void *data, size_t size);
  CudaApiExitCode launch_kernel(const char *args, size_t size);
};

#endif
