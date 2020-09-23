#include "cuda_server.h"

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
}

void handle_data(int id, Server::packet_t packet, Server &server, CudaServer *cuda_server) {
    printf("Data size: %li\n", packet.extra_data.size);
    printf("Content:\n%.*s\n", (int) packet.extra_data.size, (char*) packet.extra_data.buf);
    free(packet.extra_data.buf);
    free(packet.msg.buf);
    server.send_on_socket(id, {create_ack(), sizeof(command_base_t)});
}

CudaServer::CudaServer(const char *socket_path) : 
        server(socket_path, 10,
            [&](int id, Server::message_t msg, Server &server) { return handle_command(id, msg, server, this); },
            [&](int id, Server::packet_t packet, Server &server) { return handle_data(id, packet, server, this); }
          ), 
        cuda_manager() {

    printf("starting\n");
    this->server.initialize();
    this->server.start();
}

CudaServer::~CudaServer() {}

}
