#include <map>

struct Buffer {
  int id;
  size_t size;
  void *ptr;
};

class CudaMemoryManager {
private:
  std::map<int, Buffer> buffers;
  int buffer_count = 0;


public:

  CudaMemoryManager() {}
  ~CudaMemoryManager() {}

  int allocate_buffer(size_t size);
  void deallocate_buffer(int id);
};
