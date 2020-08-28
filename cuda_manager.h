#include <cuda.h>
#include <nvrtc.h>

#define NVRTC_SAFE_CALL(x)                                        \
  do {                                                            \
    nvrtcResult result = x;                                       \
    if (result != NVRTC_SUCCESS) {                                \
      std::cerr << "error: " #x " failed with error "             \
                << nvrtcGetErrorString(result) << '\n';           \
      exit(1);                                                    \
    }                                                             \
  } while(0)
#define CUDA_SAFE_CALL(x)                                         \
  do {                                                            \
    CUresult result = x;                                          \
    if (result != CUDA_SUCCESS) {                                 \
      const char *msg;                                            \
      cuGetErrorName(result, &msg);                               \
      std::cerr << "error: " #x " failed with error "             \
                << msg << '\n';                                   \
      exit(1);                                                    \
    }                                                             \
  } while(0)


/*! \brief A class that manages contexts and kernel compilation.
 */
class CudaManager {
public:
  CudaManager();
  ~CudaManager();
  void compile_to_ptx(const char *source_path, char **result_ptx);

  // A context and its device, only one pair for now
  CUcontext context;
  CUdevice device; 
};
