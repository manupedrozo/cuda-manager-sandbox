#include "cuda_memory_manager.h" 
#include <assert.h>
#include <map>
#include "cuda_common.h"

namespace cuda_manager {

void CudaMemoryManager::allocate_kernel(int id, size_t size) {
  assert(size > 0 && "Kernel size is 0 or less");

  printf("[Memory manager] Allocated kernel id %d size %zu\n", id, size);

  MemoryKernel mem_kernel = { id, size, nullptr, nullptr };
  kernels.emplace(id, mem_kernel);
}

void CudaMemoryManager::deallocate_kernel(int id) {
    std::map<int, MemoryKernel>::iterator it;
    it = kernels.find(id);
    assert(it != kernels.end() && "Kernel does not exist");

    if(it->second.module != nullptr) {
        printf("[Memory manager] Unloaded module %p\n", it->second.module);
        CUDA_SAFE_CALL(cuModuleUnload(it->second.module));
    }

    printf("[Memory manager] Deallocated kernel id %d\n", id);
    kernels.erase(it);
}

void CudaMemoryManager::write_kernel(int id, const char *function_name, const void *data, size_t size) {
    std::map<int, MemoryKernel>::iterator it;
    it = kernels.find(id);
    assert(it != kernels.end() && "Kernel does not exist");
    MemoryKernel *mem_kernel = &it->second;

    assert(size <= mem_kernel->size && "Data size is greater than kernel size");

    printf("[Memory manager] Loading module for kernel id %d\n", id);

    // Load module and get kernel handle
    CUDA_SAFE_CALL(cuModuleLoadDataEx(&mem_kernel->module, (char *) data, 0, 0, 0));
    CUDA_SAFE_CALL(cuModuleGetFunction(&mem_kernel->kernel, mem_kernel->module, function_name));
    printf("[Memory manager] Loaded module %p\n", mem_kernel->module);
    printf("[Memory manager] Got function %p\n", mem_kernel->kernel);
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

    printf("[Memory manager] Deallocated Buffer %p\n", (void *)it->second.d_ptr);
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
    printf("[Memory manager] Copied HtoD %p to %p\n", data, (void *)mem_buffer.d_ptr);
}

void CudaMemoryManager::read_buffer(int id, void *buf, size_t size) {
    MemoryBuffer mem_buffer = get_buffer(id);
    assert(size <= mem_buffer.size && "Read size is greater than buffer size");

    printf("[Memory manager] Copied DtoH %p to %p\n", (void *)mem_buffer.d_ptr, buf);
    CUDA_SAFE_CALL(cuMemcpyDtoH(buf, mem_buffer.d_ptr, mem_buffer.size));
}

}
