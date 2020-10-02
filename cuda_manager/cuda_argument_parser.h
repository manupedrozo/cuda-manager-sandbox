#include "cuda_common.h"
#include <vector>
#include <string>


/*! \brief Parse arguments from a string.
 * The following syntax is required: {kernel_path arguments}
 * arguments:
 *    - Buffer: {'b' is_in size ptr} "b {0|1} 2048 0x....."
 *    - Scalar: {value} "23"
 * example: "saxpy 2.5 b 1 16384 0x55c6403af910 b 1 4000 0x55c640bba8f0 b 0 4000 0x55c6403c1ee0 4096"
 * \note Allocates memory for kernel_path
 * \return true if successful
 */
bool parse_arguments(const char *arguments, std::vector<Arg *> &args, char **kernel_path);

std::string args_to_string(std::string kernel_path, std::vector<Arg *> args);
void print_args(std::vector<Arg *> args);
