#include <map>
#include <string.h>

// @TODO Properly handle errors, e.g when buffer doesnt exist

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
  void write_buffer(int id, void *data, size_t size);
  void read_buffer(int id, void *buf, size_t size);
};
