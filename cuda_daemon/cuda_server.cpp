#include "cuda_server.h"
#include "cuda_argument_parser.h"
#include "logger.h"
#include <assert.h>
#include <vector>

namespace cuda_daemon {

static Logger &logger = Logger::get_instance();

command_base_t *create_ack() {
  command_base_t *response = (command_base_t *)malloc(sizeof(command_base_t));
  init_ack_command(*response);
  return response;
}

Server::message_result_t CudaServer::handle_memory_allocate_command(int id, const memory_allocate_command_t *cmd, Server &server) {

  logger.info("Received: memory allocate command");

  // @TODO temporal while we decide if we receive the id or address in the command
  static int tmp_mem_id = 0;
  int mem_id = tmp_mem_id++;

  CudaApiExitCode err = cuda_api.allocate_memory(mem_id, cmd->size);
  if (err != CudaApiExitCode::OK) {
    return {Server::MessageListenerExitCode::OPERATION_ERROR, sizeof(memory_allocate_command_t), 0};
  }

  memory_allocate_success_command_t *res = (memory_allocate_success_command_t *)malloc(sizeof(memory_allocate_success_command_t));
  init_memory_allocate_success_command(*res, mem_id);

  server.send_on_socket(id, {res, sizeof(memory_allocate_success_command_t)});
  return {Server::MessageListenerExitCode::OK, sizeof(memory_allocate_command_t), 0};
}

Server::message_result_t CudaServer::handle_memory_write_command(int id, const memory_write_command_t *cmd, Server &server) {
  logger.info("Received: memory write command");

  server.send_on_socket(id, {create_ack(), sizeof(command_base_t)});
  return {Server::MessageListenerExitCode::OK, sizeof(memory_write_command_t), cmd->size};
}

Server::message_result_t CudaServer::handle_memory_read_command(int id, const memory_read_command_t *cmd, Server &server) {
  logger.info("Received: memory read command");

  void *buffer = malloc(cmd->size);
  CudaApiExitCode err = cuda_api.read_memory(cmd->mem_id, buffer, cmd->size);

  if (err != CudaApiExitCode::OK) {
    return {Server::MessageListenerExitCode::OPERATION_ERROR, sizeof(memory_read_command_t), 0};
  }

  server.send_on_socket(id, {buffer, cmd->size});
  return {Server::MessageListenerExitCode::OK, sizeof(memory_read_command_t), 0};
}

Server::message_result_t CudaServer::handle_launch_kernel_command(int id, const launch_kernel_command_t *cmd, Server &server) {
  logger.info("Received: launch kernel command");
  server.send_on_socket(id, {create_ack(), sizeof(command_base_t)});
  return {Server::MessageListenerExitCode::OK, sizeof(launch_kernel_command_t), cmd->size};
}

Server::message_result_t CudaServer::handle_variable_length_command(int id, const variable_length_command_t *cmd, Server &server) {
  server.send_on_socket(id, {create_ack(), sizeof(command_base_t)});
  return {Server::MessageListenerExitCode::OK, sizeof(variable_length_command_t), cmd->size};
}

Server::message_result_t CudaServer::handle_command(int id, Server::message_t msg, Server &server) {
  logger.info("Handling command");
  if (msg.size < sizeof(command_base_t)) {
    return {Server::MessageListenerExitCode::INSUFFICIENT_DATA, 0, 0}; // Need to read more data to determine a command
  }
  command_base_t *base = (command_base_t *)msg.buf;
  switch (base->cmd) {
    case MEM_ALLOC:
      if (msg.size >= sizeof(memory_allocate_command_t)) {
        return handle_memory_allocate_command(id, (memory_allocate_command_t *)msg.buf, server);
      }
      break;
    case MEM_WRITE:
      if (msg.size >= sizeof(memory_write_command_t)) {
        return handle_memory_write_command(id, (memory_write_command_t *)msg.buf, server);
      }
      break;
    case MEM_READ:
      if (msg.size >= sizeof(memory_read_command_t)) {
        return handle_memory_read_command(id, (memory_read_command_t *)msg.buf, server);
      }
      break;
    case LAUNCH_KERNEL:
      if (msg.size >= sizeof(launch_kernel_command_t)) {
        return handle_launch_kernel_command(id, (launch_kernel_command_t *)msg.buf, server);
      }
      break;
    case VARIABLE:
      if (msg.size >= sizeof(variable_length_command_t)) {
        return handle_variable_length_command(id, (variable_length_command_t *)msg.buf, server);
      }
      break;
    default:
      logger.info("Received: unknown command");
      return {Server::MessageListenerExitCode::UNKNOWN_MESSAGE, 0, 0};
      break;
  }
  return {Server::MessageListenerExitCode::INSUFFICIENT_DATA, 0, 0};
}

Server::DataListenerExitCode CudaServer::handle_data(int id, Server::packet_t packet, Server &server) {
  logger.info("Received data, size: {}", packet.extra_data.size);

  command_base_t *base = (command_base_t *)packet.msg.buf;
  switch (base->cmd) {
    case MEM_WRITE: {
      memory_write_command_t *command = (memory_write_command_t *)base;

      CudaApiExitCode err = cuda_api.write_memory(command->mem_id, packet.extra_data.buf, command->size);
      // @TODO there is no data error handling in the server yet
      assert(err == OK && "Write memory error");

      break;
    }
    case LAUNCH_KERNEL: {
      launch_kernel_command_t *command = (launch_kernel_command_t *)base;

      CudaApiExitCode err = cuda_api.launch_kernel((char *)packet.extra_data.buf, packet.extra_data.size);

      // @TODO there is no data error handling in the server yet
      assert(err == OK && "Kernel launch error");

      logger.info("Kernel launch completed");

      break;
    }
    default: {
      logger.info("Data from unsupported command: {}", (char *)packet.extra_data.buf);
      break;
    }
  }

  free(packet.extra_data.buf);
  free(packet.msg.buf);
  server.send_on_socket(id, {create_ack(), sizeof(command_base_t)});
  return Server::DataListenerExitCode::OK;
}

CudaServer::CudaServer(const char *socket_path)
    : server(
          socket_path, 10,
          [this](int id, Server::message_t msg, Server &server) { return this->handle_command(id, msg, server); },
          [this](int id, Server::packet_t packet, Server &server) { return this->handle_data(id, packet, server); }) {

  logger.info("Cuda server starting...");
  Server::InitExitCode err = server.initialize();
  if (err != Server::InitExitCode::OK) {
    logger.error("Cuda server initialization error");
    exit(EXIT_FAILURE);
  }
  server.start();
}

CudaServer::~CudaServer() {}

} // namespace cuda_daemon
