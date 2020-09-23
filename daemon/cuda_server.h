#include "server.h"
#include "commands.h"
#include "cuda_manager.h"

namespace cuda_mango {

class CudaServer {
private:
    Server server;

public:
    CudaManager cuda_manager;

    CudaServer(const char *socket_path);
    ~CudaServer();
};


}
