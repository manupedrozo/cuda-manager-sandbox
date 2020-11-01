#include "cuda_memory_manager.h" 
#include <assert.h>
#include <map>

namespace cuda_manager {

void CudaMemoryManager::allocate_buffer(int id, size_t size, void **result_ptr) {
  assert(size > 0 && "Memory to allocate is 0 or less");
  void *ptr = malloc(size);

  buffer_count = id + 1;

  printf("[Memory manager] Allocated %zu bytes\n", size);

  MemoryBuffer buffer = { id, size, ptr };
  buffers.emplace(id, buffer);

  if (result_ptr != nullptr) *result_ptr = ptr;
}

int CudaMemoryManager::allocate_buffer(size_t size, void **result_ptr) {
  assert(size > 0 && "Memory to allocate is 0 or less");
  void *ptr = malloc(size);
  int id = buffer_count++;

  printf("[Memory manager] Allocated %zu bytes\n", size);

  MemoryBuffer buffer = { id, size, ptr };
  buffers.emplace(id, buffer);

  if (result_ptr != nullptr) *result_ptr = ptr;

  return id;
}

void CudaMemoryManager::deallocate_buffer(int id) {
  std::map<int, MemoryBuffer>::iterator it;

  it = buffers.find(id);
  assert(it != buffers.end() && "Buffer does not exist");

  free(it->second.ptr);
  buffers.erase(it);
}

MemoryBuffer CudaMemoryManager::get_buffer(int id) {
  std::map<int, MemoryBuffer>::iterator it;
  it = buffers.find(id);
  assert(it != buffers.end() && "Buffer does not exist");

  return it->second;
}

void CudaMemoryManager::write_buffer(int id, const void *data, size_t size) {
  MemoryBuffer buffer = get_buffer(id);

  assert(size <= buffer.size && "Data size is greater than buffer size");

  memcpy(buffer.ptr, data, size);
}

void CudaMemoryManager::read_buffer(int id, void *buf, size_t size) {
  MemoryBuffer buffer = get_buffer(id);

  assert(size <= buffer.size && "Read size is greater than buffer size");

  memcpy(buf, buffer.ptr, size);
}

}
