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

public:
  CudaApi();
  ~CudaApi();

  CudaApiExitCode allocate_memory(int buffer_id, size_t size);
  CudaApiExitCode deallocate_memory(int buffer_id);
  CudaApiExitCode write_memory(int buffer_id, const void *data, size_t size);
  CudaApiExitCode read_memory(int buffer_id, void *dest_buffer, size_t size);

  CudaApiExitCode allocate_kernel(int kernel_id, size_t size);
  CudaApiExitCode deallocate_kernel(int kernel_id);
  CudaApiExitCode write_kernel(int kernel_id, const char *function_name, const void *data, size_t size);
  
  /*
   * \param kernel_id 
   * \param function_name name of the function to run in the kernel file
   * \param resource_args device id and grid and block dimensions
   * \param args kernel_arguments array of structs
   * \param arg_count number of arguments in the arguments array
   */
  CudaApiExitCode launch_kernel(int kernel_id, CudaResourceArgs resource_args, const char *args, int arg_count);
};

#endif
