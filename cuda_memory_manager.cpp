#include "cuda_memory_manager.h" 
#include <assert.h>
#include <map>

int CudaMemoryManager::allocate_buffer(size_t size) {
  assert(size > 0 && "Memory to allocate is 0 or less");
  void *ptr = malloc(size);
  int id = buffer_count++;

  printf("[Memory manager] Allocated %zu bytes\n", size);

  Buffer buffer = { id, size, ptr };
  buffers.emplace(id, buffer);

  return id;
}

void CudaMemoryManager::deallocate_buffer(int id) {
  std::map<int, Buffer>::iterator it;

  it = buffers.find(id);
  assert(it != buffers.end() && "Buffer does not exist");

  free(it->second.ptr);
  buffers.erase(it);
}
