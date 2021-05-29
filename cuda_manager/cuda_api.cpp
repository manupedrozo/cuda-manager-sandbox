#include "cuda_api.h"
#include "cuda_memory_manager.h"
#include "cuda_argument_parser.h"
#include <assert.h>

CudaApi::CudaApi():cuda_manager() {}

CudaApi::~CudaApi() {}

// TODO 
// - error codes

CudaApiExitCode CudaApi::allocate_memory(int buffer_id, size_t size) {
  cuda_manager.memory_manager.allocate_buffer(buffer_id, size);
  return OK;
}

CudaApiExitCode CudaApi::deallocate_memory(int buffer_id) {
  cuda_manager.memory_manager.deallocate_buffer(buffer_id);
  return OK;
}

CudaApiExitCode CudaApi::write_memory(int buffer_id, const void *data, size_t size) {
  cuda_manager.memory_manager.write_buffer(buffer_id, data, size);
  return OK;
}

CudaApiExitCode CudaApi::read_memory(int buffer_id, void *dest_buffer, size_t size) {
  cuda_manager.memory_manager.read_buffer(buffer_id, dest_buffer, size);
  return OK;
}

CudaApiExitCode CudaApi::allocate_kernel(int kernel_id, size_t size) {
  cuda_manager.memory_manager.allocate_kernel(kernel_id, size);
  return OK;
}

CudaApiExitCode CudaApi::deallocate_kernel(int kernel_id) {
  cuda_manager.memory_manager.deallocate_kernel(kernel_id);
  return OK;
}

CudaApiExitCode CudaApi::write_kernel(int kernel_id, const char *function_name, const void *data, size_t size) {
  cuda_manager.memory_manager.write_kernel(kernel_id, function_name, data, size);
  return OK;
}

CudaApiExitCode CudaApi::launch_kernel(int kernel_id, CudaResourceArgs r_args, const char *args, int arg_count) {
  // Get writtenl kernel using kernel_id
  cuda_manager::MemoryKernel mem_kernel = cuda_manager.memory_manager.get_kernel(kernel_id);
  assert(mem_kernel.kernel != nullptr && "Kernel isn't loaded, perform kernel_write() before launching");

  // Launch kernel
#ifndef NDEBUG
  std::cout << "Launching kernel " << kernel_id << "\n";
  std::cout << "Resources: device_id: " 
            << r_args.device_id << ",  Grid(" 
            << r_args.grid_dim.x << ", " 
            << r_args.grid_dim.y << ", " 
            << r_args.grid_dim.z << "),  Block(" 
            << r_args.block_dim.x << ", "
            << r_args.block_dim.y << ", "  
            << r_args.block_dim.z << ")\n"; 
  std::cout << "Number of arguments: " << arg_count << "\n";
#endif

  cuda_manager.launch_kernel(mem_kernel.kernel, r_args, args, arg_count);

  return OK;
}

