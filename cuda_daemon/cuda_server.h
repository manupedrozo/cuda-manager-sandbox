#include "server.h"
#include "commands.h"
#include "cuda_manager.h"

namespace cuda_daemon {

class CudaServer {

  private:
  Server server;

  public:
  cuda_manager::CudaManager cuda_manager;

  CudaServer(const char *socket_path);
  ~CudaServer();
};


}
