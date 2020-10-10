#ifndef CUDA_CLIENT_H
#define CUDA_CLIENT_H

#include <stdlib.h>

namespace cuda_daemon {

class CudaClient {

public:
  enum ExitCode {
    OK,           // successful operation
    ERROR,        // generic error in the request
    SEVERE_ERROR, // error that leaves the client unusable
  };

  CudaClient(const char *socket_path);

  ~CudaClient();

  CudaClient::ExitCode memory_allocate(size_t size, int *mem_id);

  CudaClient::ExitCode memory_release(int mem_id);

  CudaClient::ExitCode memory_write(int id, void *buf, size_t size);

  CudaClient::ExitCode memory_read(int id, void *buf, size_t size);

  CudaClient::ExitCode launch_kernel(char *arg_string, size_t size);

private:
  int socket_fd;

  void close_socket();
};

} // namespace cuda_daemon

#endif
