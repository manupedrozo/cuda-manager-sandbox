#include "server.h"
#include "commands.h"
#include "cuda_api.h"

namespace cuda_daemon {

class CudaServer {

  private:
  Server server;

  public:
  CudaApi cuda_api;

  CudaServer(const char *socket_path);
  ~CudaServer();
};


}
