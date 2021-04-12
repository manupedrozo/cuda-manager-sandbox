#include <cstddef>

namespace cuda_compiler {

/*! \brief A class for cuda kernel compilation.
 */
class CudaCompiler {
public:
  CudaCompiler() {}
  ~CudaCompiler() {}
  void compile_to_ptx(const char *source_path, char **ptx, size_t *ptx_size = nullptr);
  void save_ptx_to_file(const char *ptx, const char *output_path);
  char *read_ptx_from_file(const char *ptx_path);
};

}
