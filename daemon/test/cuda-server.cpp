#include "cuda_server.h"

#define SOCKET_PATH "/tmp/server-test"

using namespace cuda_mango;

int main(int argc, char const *argv[]) { 
    CudaServer cuda_server(SOCKET_PATH);
}
