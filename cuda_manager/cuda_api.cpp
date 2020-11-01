#include "cuda_api.h"
#include "cuda_argument_parser.h"

CudaApi::CudaApi():cuda_manager() {}

CudaApi::~CudaApi() {}

// TODO 
// - figure out if we are using id or address for memory operations
// - error codes

CudaApiExitCode CudaApi::allocate_memory(int mem_id, size_t size) {
  memory_manager.allocate_buffer(mem_id, size);
  return OK;
}

CudaApiExitCode CudaApi::release_memory(int mem_id) {
  memory_manager.deallocate_buffer(mem_id);
  return OK;
}

CudaApiExitCode CudaApi::write_memory(int mem_id, const void *data, size_t size) {
  memory_manager.write_buffer(mem_id, data, size);
  return OK;
}

CudaApiExitCode CudaApi::read_memory(int mem_id, void *dest_buffer, size_t size) {
  memory_manager.read_buffer(mem_id, dest_buffer, size);
  return OK;
}


// @TODO arguments may need change
// kernel_mem_id wont be in args any more but as a function arg
CudaApiExitCode CudaApi::launch_kernel(const char *args, size_t size) {
  char *kernel_name;

  int kernel_mem_id;
  int args_count;
  char *parsed_args;
  bool err = cuda_manager::parse_arguments(args, &parsed_args, &args_count, &kernel_mem_id, &kernel_name, &memory_manager);

  if (!err) return ERROR; // Arg parsing error

  // Get ptx using kernel_mem_id
  char *ptx = (char *) memory_manager.get_buffer(kernel_mem_id).ptr;

  // Launch kernel
  // @TODO change fixed blocks/threads
  cuda_manager.launch_kernel_from_ptx(ptx, kernel_name, parsed_args, args_count, 32, 128);

  // Free stuff
  delete[] kernel_name;
  delete[] ptx;
  free(parsed_args);

  return OK;
}




