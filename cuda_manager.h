#include <cuda.h>
#include <vector>

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

struct Arg {
  float value; // TODO add support for other scalars
  void *base_ptr;
  size_t size;
  bool is_buffer;
  bool is_in;
};

// Struct to keep track of buffers
struct CudaBuffer {
  void *h_ptr; // Ptr to host memory
  CUdeviceptr d_ptr; // Ptr to device memory
  size_t size;
  bool is_in; // Input or output buffer
};

/*! \brief A class that manages devices, contexts and launches kernels.
 */
class CudaManager {
public:
  CudaManager();
  ~CudaManager();
  void launch_kernel(const CUfunction kernel, const std::vector<Arg> args, const uint32_t num_blocks, const uint32_t num_threads);

  uint32_t device_count = 0;
  // (Device, Context) pairs
  CUdevice *devices; 
  CUcontext *contexts;
};
