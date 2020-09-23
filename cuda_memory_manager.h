#include <map>

struct MemoryBuffer {
  int id;
  size_t size;
  void *ptr;
};

class CudaMemoryManager {
private:
  std::map<int, MemoryBuffer> buffers;
  int buffer_count = 0;


public:

  CudaMemoryManager() {}
  ~CudaMemoryManager() {}

  int allocate_buffer(size_t size, void **result_ptr = nullptr);
  void deallocate_buffer(int id);
  MemoryBuffer get_buffer(int id);
};
