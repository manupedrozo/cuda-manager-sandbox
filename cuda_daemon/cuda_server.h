#include "commands.h"
#include "cuda_api.h"
#include "server.h"

namespace cuda_daemon {

class CudaServer {
public:
  CudaServer(const char *socket_path);
  ~CudaServer();

private:
  CudaApi cuda_api;
  Server server;

  Server::message_result_t handle_command(int id, Server::message_t msg, Server &server);

  Server::DataListenerExitCode handle_data(int id, Server::packet_t packet, Server &server);

  Server::message_result_t handle_memory_allocate_command(int id, const memory_allocate_command_t *cmd, Server &server);
  Server::message_result_t handle_memory_write_command(int id, const memory_write_command_t *cmd, Server &server);
  Server::message_result_t handle_memory_read_command(int id, const memory_read_command_t *cmd, Server &server);
  Server::message_result_t handle_launch_kernel_command(int id, const launch_kernel_command_t *cmd, Server &server);
  Server::message_result_t handle_variable_length_command(int id, const variable_length_command_t *cmd, Server &server);
};

} // namespace cuda_daemon
