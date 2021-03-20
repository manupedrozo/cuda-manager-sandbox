#include "kernel_arguments.h"
#include "cuda_memory_manager.h"
#include <vector>
#include <string>

namespace cuda_manager {

// TODO NOT CURRENTLY WORKING, SCALAR ARGS NOT SUPPORTED
/*! \brief Parse arguments from a string.
 * The following syntax is required: {kernel_mem_id kernel_name arguments}
 * arguments:
 *    - Buffer: {'b' is_in id} "b {0|1} 5"
 *    - Scalar: {value} "23"
 * example: "0 saxpy 2.5 b 1 1 b 1 2 b 0 3 4096"
 * \note Allocates memory for kernel_name and parsed_args
 * \return true if successful
 */
bool parse_arguments(const char *arguments, char **parsed_args, int *arg_count, int *kernel_mem_id, char **kernel_name);

std::string args_to_string(std::string kernel_name, int kernel_mem_id, std::vector<void *> args);
void print_args(std::vector<void *> args);

}
