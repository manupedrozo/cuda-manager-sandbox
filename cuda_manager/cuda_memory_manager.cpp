#include "cuda_memory_manager.h" 
#include <assert.h>
#include <map>
#include "cuda_common.h"

namespace cuda_manager {

void CudaMemoryManager::allocate_kernel(int id, size_t size) {
  assert(size > 0 && "Memory to allocate is 0 or less");
  void *ptr = malloc(size);

  printf("[Memory manager] Allocated %zu bytes at %p\n", size, ptr);

  MemoryKernel mem_kernel = { id, size, ptr };
  kernels.emplace(id, mem_kernel);
}

void CudaMemoryManager::deallocate_kernel(int id) {
    std::map<int, MemoryKernel>::iterator it;
    it = kernels.find(id);
    assert(it != kernels.end() && "Kernel does not exist");

    printf("Deallocated Kernel %p\n", it->second.ptr);
    free(it->second.ptr);
    kernels.erase(it);
}

void CudaMemoryManager::write_kernel(int id, const void *data, size_t size) {
    MemoryKernel mem_kernel = get_kernel(id);
    assert(size <= mem_kernel.size && "Data size is greater than kernel size");
    printf("[Memory manager] Writing %zu bytes at kernel id %d \n", size, id);
    printf("[Memory manager] Writing from %p to %p\n", data, mem_kernel.ptr);
    printf("[Memory manager] Kernel size: %zu, id %d, ptr %p\n", mem_kernel.size, mem_kernel.id, mem_kernel.ptr);
    memcpy(mem_kernel.ptr, data, size);
}


MemoryKernel CudaMemoryManager::get_kernel(int id) {
    std::map<int, MemoryKernel>::iterator it;

    it = kernels.find(id);
    assert(it != kernels.end() && "Kernel does not exist");

    return it->second;
}

void CudaMemoryManager::allocate_buffer(int id, size_t size) {
    assert(size > 0 && "Memory to allocate is 0 or less");

    MemoryBuffer mem_buffer;
    mem_buffer.id = id;
    mem_buffer.size = size;

    CUDA_SAFE_CALL(cuMemAlloc(&mem_buffer.d_ptr, size));

    printf("[Memory manager] Allocated %zu bytes at %p\n", size, (void *)mem_buffer.d_ptr);

    buffers.emplace(id, mem_buffer);
}

void CudaMemoryManager::deallocate_buffer(int id) {
    std::map<int, MemoryBuffer>::iterator it;
    it = buffers.find(id);
    assert(it != buffers.end() && "Buffer does not exist");

    printf("Deallocated Buffer %p\n", (void *)it->second.d_ptr);
    CUDA_SAFE_CALL(cuMemFree(it->second.d_ptr));

    buffers.erase(it);
}

MemoryBuffer CudaMemoryManager::get_buffer(int id) {
    std::map<int, MemoryBuffer>::iterator it;
    it = buffers.find(id);
    assert(it != buffers.end() && "Buffer does not exist");
    return it->second;
}

void CudaMemoryManager::write_buffer(int id, const void *data, size_t size) {
    MemoryBuffer mem_buffer = get_buffer(id);
    assert(size <= mem_buffer.size && "Data size is greater than buffer size");

    printf("[Memory manager] Writing %zu bytes at buffer id %d \n", size, id);
    printf("[Memory manager] Writing from %p to %p\n", data, (void *)mem_buffer.d_ptr);
    printf("[Memory manager] Buffer size: %zu, id %d, ptr %p\n", mem_buffer.size, mem_buffer.id, (void *)mem_buffer.d_ptr);

    CUDA_SAFE_CALL(cuMemcpyHtoD(mem_buffer.d_ptr, data, size));
    printf("Copied HtoD %p to %p\n", data, (void *)mem_buffer.d_ptr);
}

void CudaMemoryManager::read_buffer(int id, void *buf, size_t size) {
    MemoryBuffer mem_buffer = get_buffer(id);
    assert(size <= mem_buffer.size && "Read size is greater than buffer size");

    printf("Copied DtoH %p to %p\n", (void *)mem_buffer.d_ptr, buf);
    CUDA_SAFE_CALL(cuMemcpyDtoH(buf, mem_buffer.d_ptr, mem_buffer.size));
}

}
