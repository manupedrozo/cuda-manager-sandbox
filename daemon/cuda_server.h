#include "server.h"
#include "commands.h"
#include "cuda_memory_manager.h"

namespace cuda_mango {

class CudaServer {
private:
    Server server;

public:

    //Adding memory manager here for now, we have to decide its owner later
    CudaMemoryManager memory_manager;

    CudaServer(const char *socket_path);
    ~CudaServer();
};




}
