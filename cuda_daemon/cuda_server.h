#include "server.h"
#include "commands.h"
#include "cuda_manager.h"
#include "cuda_compiler.h"

namespace cuda_daemon {

class CudaServer {

  private:
  Server server;

  public:
  cuda_manager::CudaManager cuda_manager;

  // Compiler here for simplicity when launching kernels, would probably be removed in the future
  cuda_manager::CudaCompiler cuda_compiler;

  CudaServer(const char *socket_path);
  ~CudaServer();
};


}
