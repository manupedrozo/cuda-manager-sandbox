#include "cuda_memory_manager.h" 
#include <assert.h>
#include <map>

namespace cuda_manager {

void CudaMemoryManager::allocate_buffer(int id, size_t size, bool is_kernel, void **result_ptr) {
  assert(size > 0 && "Memory to allocate is 0 or less");
  void *ptr = malloc(size);

  next_id = id + 1;

  printf("[Memory manager] Allocated %zu bytes at %p\n", size, ptr);

  MemoryBuffer mem_buffer = { id, size, ptr };
  if (is_kernel)
    kernels.emplace(id, mem_buffer);
  else
    buffers.emplace(id, mem_buffer);


  if (result_ptr != nullptr) *result_ptr = ptr;
}

int CudaMemoryManager::allocate_buffer(size_t size, bool is_kernel, void **result_ptr) {
  assert(size > 0 && "Memory to allocate is 0 or less");
  void *ptr = malloc(size);
  int id = next_id++;

  printf("[Memory manager] Allocated %zu bytes\n", size);

  MemoryBuffer mem_buffer = { id, size, ptr };

  if (is_kernel)
    kernels.emplace(id, mem_buffer);
  else
    buffers.emplace(id, mem_buffer);

  if (result_ptr != nullptr) *result_ptr = ptr;

  return id;
}

void CudaMemoryManager::deallocate_buffer(int id, bool is_kernel) {
  std::map<int, MemoryBuffer>::iterator it;

  if (is_kernel) {
    it = kernels.find(id);
    assert(it != kernels.end() && "Kernel does not exist");
    free(it->second.ptr);
    kernels.erase(it);
  } else {
    it = buffers.find(id);
    assert(it != buffers.end() && "Buffer does not exist");
    free(it->second.ptr);
    buffers.erase(it);
  }
}

MemoryBuffer CudaMemoryManager::get_buffer(int id, bool is_kernel) {
  std::map<int, MemoryBuffer>::iterator it;

  if (is_kernel) {
    it = kernels.find(id);
    assert(it != kernels.end() && "Kernel does not exist");
  } else {
    it = buffers.find(id);
    assert(it != buffers.end() && "Buffer does not exist");
  }

  return it->second;
}

void CudaMemoryManager::write_buffer(int id, const void *data, size_t size, bool is_kernel) {
  MemoryBuffer mem_buffer = get_buffer(id, is_kernel);
  assert(size <= mem_buffer.size && "Data size is greater than buffer size");

  printf("[Memory manager] Writing at buffer id %d \n", id);
  printf("[Memory manager] Writing %zu bytes from %p to %p\n", size, data, mem_buffer.ptr);
  printf("[Memory manager] Buffer size: %zu, id %d, ptr %p\n", mem_buffer.size, mem_buffer.id, mem_buffer.ptr);
  memcpy(mem_buffer.ptr, data, size);
}

void CudaMemoryManager::read_buffer(int id, void *buf, size_t size, bool is_kernel) {
  MemoryBuffer mem_buffer = get_buffer(id, is_kernel);

  assert(size <= mem_buffer.size && "Read size is greater than buffer size");

  memcpy(buf, mem_buffer.ptr, size);
}

}
