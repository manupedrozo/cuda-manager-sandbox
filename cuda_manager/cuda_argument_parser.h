#include "kernel_arguments.h"
#include "cuda_memory_manager.h"
#include <vector>
#include <string>

namespace cuda_manager {

/*! \brief Parse arguments from a string.
 * The following syntax is required: {kernel_mem_id kernel_name arguments}
 * arguments:
 *    - Buffer: {'b' is_in size ptr} "b {0|1} 2048 0x....."
 *    - Scalar: {value} "23"
 * example: "1 saxpy 2.5 b 1 16384 0x55c6403af910 b 1 4000 0x55c640bba8f0 b 0 4000 0x55c6403c1ee0 4096"
 * \note Allocates memory for kernel_name and parsed_args
 * \return true if successful
 */
bool parse_arguments(const char *arguments, char **parsed_args, int *arg_count, int *kernel_mem_id, char **kernel_name, CudaMemoryManager *memory_manager);

std::string args_to_string(std::string kernel_name, int kernel_mem_id, std::vector<void *> args);
void print_args(std::vector<void *> args);

}
