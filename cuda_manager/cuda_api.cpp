#include "cuda_api.h"
#include "cuda_argument_parser.h"

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

CudaApiExitCode CudaApi::write_kernel(int kernel_id, const void *data, size_t size) {
  cuda_manager.memory_manager.write_kernel(kernel_id, data, size);
  return OK;
}

CudaApiExitCode CudaApi::launch_kernel(int kernel_id, const char *function_name, CudaResourceArgs r_args, const char *args, int arg_count) {
  // Get ptx using kernel_id
  char *ptx = (char *) cuda_manager.memory_manager.get_kernel(kernel_id).ptr;

  // Launch kernel
  //std::cout << "Launching kernel from ptx: \n" << ptx << "\n";
  std::cout << "Function name " << function_name << "\n";
  std::cout << "Resources: device_id:" 
            << r_args.device_id << ",  Grid(" 
            << r_args.grid_dim.x << ", " 
            << r_args.grid_dim.y << ", " 
            << r_args.grid_dim.z << "),  Block(" 
            << r_args.block_dim.x << ", "
            << r_args.block_dim.y << ", "  
            << r_args.block_dim.z << ")\n"; 
  std::cout << "Number of arguments: " << arg_count << "\n";
  cuda_manager.launch_kernel_from_ptx(ptx, function_name, r_args, args, arg_count);

  return OK;
}

