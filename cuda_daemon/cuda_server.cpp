#include "cuda_server.h"
#include "cuda_argument_parser.h"
#include "logger.h"
#include <assert.h>
#include <vector>

namespace cuda_daemon {

  static Logger &logger = Logger::get_instance();

  command_base_t *create_ack() {
    command_base_t *response = (command_base_t *) malloc(sizeof(command_base_t));
    init_ack_command(*response);
    return response;
  }

  Server::message_result_t handle_memory_allocate_command(int id, const memory_allocate_command_t *cmd, Server &server, CudaServer *cuda_server) {

    logger.info("Received: memory allocate command");

    int mem_id = cuda_server->cuda_manager.memory_manager.allocate_buffer(cmd->size);

    memory_allocate_success_command_t *res = (memory_allocate_success_command_t *) malloc(sizeof(memory_allocate_success_command_t));
    init_memory_allocate_success_command(*res, mem_id);

    server.send_on_socket(id, {res, sizeof(memory_allocate_success_command_t)});
    return {Server::MessageListenerExitCode::OK, sizeof(memory_allocate_command_t), 0};
  }

  Server::message_result_t handle_memory_write_command(int id, const memory_write_command_t *cmd, Server &server, CudaServer *cuda_server) {
    logger.info("Received: memory write command");

    server.send_on_socket(id, {create_ack(), sizeof(command_base_t)});
    return {Server::MessageListenerExitCode::OK, sizeof(memory_write_command_t), cmd->size};
  }

  Server::message_result_t handle_memory_read_command(int id, const memory_read_command_t *cmd, Server &server, CudaServer *cuda_server) {
    logger.info("Received: memory read command");

    void *buffer = malloc(cmd->size);
    cuda_server->cuda_manager.memory_manager.read_buffer(cmd->mem_id, buffer, cmd->size);
    server.send_on_socket(id, {buffer, cmd->size});
    return {Server::MessageListenerExitCode::OK, sizeof(memory_read_command_t), 0};
  }

  Server::message_result_t handle_launch_kernel_command(int id, const launch_kernel_command_t *cmd, Server &server) {
    logger.info("Received: launch kernel command");
    server.send_on_socket(id, {create_ack(), sizeof(command_base_t)});
    return {Server::MessageListenerExitCode::OK, sizeof(launch_kernel_command_t), cmd->size};
  }

  Server::message_result_t handle_variable_length_command(int id, const variable_length_command_t *cmd, Server &server) {
    server.send_on_socket(id, {create_ack(), sizeof(command_base_t)});
    return {Server::MessageListenerExitCode::OK, sizeof(variable_length_command_t), cmd->size};
  }

  Server::message_result_t handle_command(int id, Server::message_t msg, Server &server, CudaServer *cuda_server) {
    logger.info("Handling command");
    if (msg.size < sizeof(command_base_t)) {
      return {Server::MessageListenerExitCode::INSUFFICIENT_DATA, 0, 0}; // Need to read more data to determine a command
    }
    command_base_t *base = (command_base_t *) msg.buf;
    switch (base->cmd) {
      case MEM_ALLOC:
        if (msg.size >= sizeof(memory_allocate_command_t)) {
          return handle_memory_allocate_command(id, (memory_allocate_command_t *)  msg.buf, server, cuda_server);
        }
        break;
      case MEM_WRITE:
        if (msg.size >= sizeof(memory_write_command_t)) {
          return handle_memory_write_command(id, (memory_write_command_t *)  msg.buf, server, cuda_server);
        }
        break;
      case MEM_READ:
        if (msg.size >= sizeof(memory_read_command_t)) {
          return handle_memory_read_command(id, (memory_read_command_t *)  msg.buf, server, cuda_server);
        }
        break;
      case LAUNCH_KERNEL:
        if (msg.size >= sizeof(launch_kernel_command_t)) {
          return handle_launch_kernel_command(id, (launch_kernel_command_t *)  msg.buf, server);
        }
        break;
      case VARIABLE:
        if (msg.size >= sizeof(variable_length_command_t)) {
          return handle_variable_length_command(id, (variable_length_command_t *)  msg.buf, server);
        }
        break;
      default:
        logger.info("Received: unknown command");
        return {Server::MessageListenerExitCode::UNKNOWN_MESSAGE, 0, 0};
        break;
    }
    return {Server::MessageListenerExitCode::INSUFFICIENT_DATA, 0, 0};
  }

  void handle_data(int id, Server::packet_t packet, Server &server, CudaServer *cuda_server) {
    logger.info("Received data, size: {}", packet.extra_data.size);

    command_base_t *base = (command_base_t *) packet.msg.buf;
    switch (base->cmd) {
      case MEM_WRITE: {
        memory_write_command_t *command = (memory_write_command_t *) base;

        cuda_server->cuda_manager.memory_manager.write_buffer(command->mem_id, packet.extra_data.buf, command->size);
        break;
      }
      case LAUNCH_KERNEL: {
        launch_kernel_command_t *command = (launch_kernel_command_t *) base;
        
        std::vector<cuda_manager::Arg *> parsed_args;
        char *kernel_path;
        bool err = cuda_manager::parse_arguments((char *) packet.extra_data.buf, parsed_args, &kernel_path);

        // @TODO there is no data error handling in the server yet
        assert(err && "Argument parsing error");

        // @TODO We are assuming that the kernel ptx file is accessible by path and that the kernel_path is the same as function name inside the ptx file
        char *ptx = cuda_server->cuda_compiler.read_ptx_from_file(kernel_path);

        cuda_server->cuda_manager.launch_kernel_from_ptx(ptx, kernel_path, parsed_args, 32, 128);

        // Free stuff
        delete[] kernel_path;
        delete[] ptx;
        for (cuda_manager::Arg *arg : parsed_args) { free(arg); }

        logger.info("Kernel launch completed");

        break;
      }
      default: {
        logger.info("Data from unsupported command: {}", (char*) packet.extra_data.buf);
        break;
      }
    }

    free(packet.extra_data.buf);
    free(packet.msg.buf);
    server.send_on_socket(id, {create_ack(), sizeof(command_base_t)});
  }

  CudaServer::CudaServer(const char *socket_path) : 
    server(socket_path, 10,
        [&](int id, Server::message_t msg, Server &server) { return handle_command(id, msg, server, this); },
        [&](int id, Server::packet_t packet, Server &server) { return handle_data(id, packet, server, this); }
        ), 
    cuda_manager(),
    cuda_compiler() {

      logger.info("Cuda server starting...");
      Server::InitExitCode err = this->server.initialize();
      if (err != Server::InitExitCode::OK) {
        logger.error("Cuda server initialization error");
        exit(EXIT_FAILURE);
      }
      this->server.start();
    }

  CudaServer::~CudaServer() {}

}