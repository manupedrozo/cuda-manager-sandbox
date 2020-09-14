#include "commands.h"
#include "socket_client.h"
#include "cuda_client.h"

#define CHECK_OPEN_SOCKET if(socket_fd == -1) return SEVERE_ERROR;
#define TRY_OR_CLOSE(x)         \
    if(!x) {                    \
        close_socket();         \
        return SEVERE_ERROR;    \
    }

namespace cuda_mango {

    CudaClient::CudaClient(const char *socket_path) {
        socket_fd = initialize(socket_path);
    }

    CudaClient::~CudaClient() {
        close_socket();
    }

    void CudaClient::close_socket() {
        if (socket_fd == -1) return;

        end(socket_fd);
        socket_fd = -1;
    }


    CudaClient::ExitCode CudaClient::memory_allocate(size_t size, int &mem_id) { 
        CHECK_OPEN_SOCKET

        memory_allocate_command_t cmd;
        init_memory_allocate_command(cmd, size);
        TRY_OR_CLOSE(send_on_socket(socket_fd, &cmd, sizeof(cmd)))

        memory_allocate_success_command_t res;
        TRY_OR_CLOSE(receive_on_socket(socket_fd, &res, sizeof(res)))

        if (res.cmd != ALLOC_MEM_SUCCESS) {
            return ERROR;
        }

        mem_id = res.mem_id;

        return OK;
    }

    CudaClient::ExitCode CudaClient::memory_release(int mem_id) {
        CHECK_OPEN_SOCKET

        memory_release_command_t cmd;
        init_memory_release_command(cmd, mem_id);
        TRY_OR_CLOSE(send_on_socket(socket_fd, &cmd, sizeof(cmd)))

        return OK;
    }

    CudaClient::ExitCode CudaClient::memory_write(int id, void *buf, size_t size) {
        CHECK_OPEN_SOCKET

        memory_write_command_t cmd;
        init_memory_write_command(cmd, id, size);

        TRY_OR_CLOSE(send_on_socket(socket_fd, &cmd, sizeof(cmd)))

        TRY_OR_CLOSE(send_on_socket(socket_fd, buf, size))

        return OK;
    }

    CudaClient::ExitCode CudaClient::memory_read(int id, void *buf, size_t size) {
        CHECK_OPEN_SOCKET

        memory_read_command_t cmd;
        init_memory_read_command(cmd, id, size);

        TRY_OR_CLOSE(send_on_socket(socket_fd, &cmd, sizeof(cmd)))

        TRY_OR_CLOSE(receive_on_socket(socket_fd, buf, size))

        return OK;
    }
    
}