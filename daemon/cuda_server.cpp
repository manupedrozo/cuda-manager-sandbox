#include "cuda_server.h"
#include "cuda_argument_parser.h"
#include <assert.h>
#include <vector>

namespace cuda_mango {

  command_base_t *create_ack() {
    command_base_t *response = (command_base_t *) malloc(sizeof(command_base_t));
    init_ack_command(*response);
    return response;
  }

  Server::message_result_t handle_memory_allocate_command(int id, const memory_allocate_command_t *cmd, Server &server, CudaServer *cuda_server) {

    printf("Received: memory allocate command\n");

    int mem_id = cuda_server->cuda_manager.memory_manager.allocate_buffer(cmd->size);

    memory_allocate_success_command_t *res = (memory_allocate_success_command_t *) malloc(sizeof(memory_allocate_success_command_t));
    init_memory_allocate_success_command(*res, mem_id);

    server.send_on_socket(id, {res, sizeof(memory_allocate_success_command_t)});
    return {Server::MessageListenerExitCode::OK, sizeof(memory_allocate_command_t), 0};
  }

  Server::message_result_t handle_memory_write_command(int id, const memory_write_command_t *cmd, Server &server, CudaServer *cuda_server) {
    printf("Received: memory write command\n");

    server.send_on_socket(id, {create_ack(), sizeof(command_base_t)});
    return {Server::MessageListenerExitCode::OK, sizeof(memory_write_command_t), cmd->size};
  }

  Server::message_result_t handle_memory_read_command(int id, const memory_read_command_t *cmd, Server &server, CudaServer *cuda_server) {
    printf("Received: memory read command\n");

    void *buffer = malloc(cmd->size);
    cuda_server->cuda_manager.memory_manager.read_buffer(cmd->mem_id, buffer, cmd->size);
    server.send_on_socket(id, {buffer, cmd->size});
    return {Server::MessageListenerExitCode::OK, sizeof(memory_read_command_t), 0};
  }

  Server::message_result_t handle_launch_kernel_command(int id, const launch_kernel_command_t *cmd, Server &server) {
    printf("Received: launch kernel command\n");
    server.send_on_socket(id, {create_ack(), sizeof(command_base_t)});
    return {Server::MessageListenerExitCode::OK, sizeof(launch_kernel_command_t), cmd->size};
  }

  Server::message_result_t handle_variable_length_command(int id, const variable_length_command_t *cmd, Server &server) {
    server.send_on_socket(id, {create_ack(), sizeof(command_base_t)});
    return {Server::MessageListenerExitCode::OK, sizeof(variable_length_command_t), cmd->size};
  }

  Server::message_result_t handle_command(int id, Server::message_t msg, Server &server, CudaServer *cuda_server) {
    printf("Handling command\n");
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
        printf("handle: Unknown command\n");
        return {Server::MessageListenerExitCode::UNKNOWN_MESSAGE, 0, 0};
        break;
    }
    return {Server::MessageListenerExitCode::INSUFFICIENT_DATA, 0, 0};
  }

  void handle_data(int id, Server::packet_t packet, Server &server, CudaServer *cuda_server) {
    printf("Received data, size: %zu\n", packet.extra_data.size);

    command_base_t *base = (command_base_t *) packet.msg.buf;
    switch (base->cmd) {
      case MEM_WRITE: {
        memory_write_command_t *command = (memory_write_command_t *) base;

        cuda_server->cuda_manager.memory_manager.write_buffer(command->mem_id, packet.extra_data.buf, command->size);
        break;
      }
      case LAUNCH_KERNEL: {
        launch_kernel_command_t *command = (launch_kernel_command_t *) base;
        
        std::vector<Arg *> parsed_args;
        char *kernel_path;
        bool err = parse_arguments((char *) packet.extra_data.buf, parsed_args, &kernel_path);

        // @TODO there is data error handling in the server yet
        assert(err && "Argument parsing error");

        // @TODO this needs a clean up, we are doing all the work here now for simplicity
        // We probably wont have the compiler here
        // We are assuming that the kernel ptx file is accessible by path
        char *ptx = cuda_server->cuda_compiler.read_ptx_from_file(kernel_path);

        CUmodule module;
        CUfunction kernel;
        CUDA_SAFE_CALL(cuModuleLoadDataEx(&module, ptx, 0, 0, 0));
        CUDA_SAFE_CALL(cuModuleGetFunction(&kernel, module, kernel_path));

        delete[] kernel_path;
        delete[] ptx;

        cuda_server->cuda_manager.launch_kernel(kernel, parsed_args, 32, 128);

        for (Arg *arg : parsed_args) { free(arg); }
        CUDA_SAFE_CALL(cuModuleUnload(module));

        printf("Kernel launch completed\n");

        break;
      }
      default: {
        printf("Content:\n%.*s\n", (int) packet.extra_data.size, (char*) packet.extra_data.buf);
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

      printf("starting\n");
      this->server.initialize();
      this->server.start();
    }

  CudaServer::~CudaServer() {}

}
