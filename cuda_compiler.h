#include "cuda_common.h"
#include <nvrtc.h>


/*! \brief A class for cuda kernel compilation.
 */
class CudaCompiler {
public:
  CudaCompiler() {}
  ~CudaCompiler() {}
  char *compile_to_ptx(const char *source_path);
  void save_ptx_to_file(const char *ptx, const char *output_path);
  char *read_ptx_from_file(const char *ptx_path);
};
