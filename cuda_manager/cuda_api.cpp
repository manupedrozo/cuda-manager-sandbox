#include "cuda_api.h"
#include "cuda_argument_parser.h"

CudaApi::CudaApi():cuda_manager() {}

CudaApi::~CudaApi() {}

// TODO 
// - figure out if we are using id or address for memory operations
// - error codes

CudaApiExitCode CudaApi::allocate_memory(int buffer_id, size_t size) {
  cuda_manager.memory_manager.allocate_buffer(buffer_id, size, false);
  return OK;
}

CudaApiExitCode CudaApi::deallocate_memory(int buffer_id) {
  cuda_manager.memory_manager.deallocate_buffer(buffer_id, false);
  return OK;
}

CudaApiExitCode CudaApi::write_memory(int buffer_id, const void *data, size_t size) {
  cuda_manager.memory_manager.write_buffer(buffer_id, data, size, false);
  return OK;
}

CudaApiExitCode CudaApi::read_memory(int buffer_id, void *dest_buffer, size_t size) {
  cuda_manager.memory_manager.read_buffer(buffer_id, dest_buffer, size, false);
  return OK;
}

CudaApiExitCode CudaApi::allocate_kernel(int kernel_id, size_t size) {
  cuda_manager.memory_manager.allocate_buffer(kernel_id, size, true);
  return OK;
}

CudaApiExitCode CudaApi::deallocate_kernel(int kernel_id) {
  cuda_manager.memory_manager.deallocate_buffer(kernel_id, true);
  return OK;
}

CudaApiExitCode CudaApi::write_kernel(int kernel_id, const void *data, size_t size) {
  cuda_manager.memory_manager.write_buffer(kernel_id, data, size, true);
  return OK;
}

CudaApiExitCode CudaApi::launch_kernel(int kernel_id, const char *function_name, const char *args, int arg_count) {
  // Get ptx using kernel_id
  char *ptx = (char *) cuda_manager.memory_manager.get_buffer(kernel_id, true).ptr;

  // Launch kernel
  // @TODO change fixed blocks/threads
  //std::cout << "Launching kernel from ptx: \n" << ptx << "\n";
  std::cout << "Function name " << function_name << "\n";
  std::cout << "Number of arguments: " << arg_count << "\n";
  cuda_manager.launch_kernel_from_ptx(ptx, function_name, args, arg_count, 32, 128);

  return OK;
}

// @TODO arguments may need change
// kernel_id wont be in args any more but as a function arg
CudaApiExitCode CudaApi::launch_kernel_string_args(const char *args, size_t size) {
  char *function_name;

  int kernel_id;
  int args_count;
  char *parsed_args;
  bool err = cuda_manager::parse_arguments(args, &parsed_args, &args_count, &kernel_id, &function_name);

  if (!err) return ERROR; // Arg parsing error

  // Get ptx using kernel_id
  char *ptx = (char *) cuda_manager.memory_manager.get_buffer(kernel_id, true).ptr;

  // Launch kernel
  // @TODO change fixed blocks/threads
  //std::cout << "Launching kernel from ptx: \n" << ptx << "\n";
  std::cout << "Function name " << function_name << "\n";
  std::cout << "Number of arguments: " << args_count << "\n";
  cuda_manager.launch_kernel_from_ptx(ptx, function_name, parsed_args, args_count, 32, 128);

  // Free stuff
  //delete[] ptx; not deleting ptx, user takes care of deleting kernel when done
  delete[] function_name;
  free(parsed_args);

  return OK;
}




