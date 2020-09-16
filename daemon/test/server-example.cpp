#include "server.h"

#define SOCKET_PATH "/tmp/server-test"

using namespace cuda_mango;

Server::command_result_t handle_hello_command(const hello_command_t *cmd) {
    printf("%s\n", cmd->message);
    return {Server::OK, sizeof(hello_command_t), 0};
}

Server::command_result_t handle_end_command(const command_base_t *cmd) {
    (void) (cmd);
    printf("This should stop the server\n");
    return {Server::OK, sizeof(command_base_t), 0};
}

Server::command_result_t handle_variable_length_command(const variable_length_command_t *cmd) {
    return {Server::OK, sizeof(variable_length_command_t), cmd->size};
}

Server::command_result_t handle_command(const void *buffer, size_t size) {
    if (size < sizeof(command_base_t)) {
        return {Server::INSUFFICIENT_DATA, 0, 0}; // Need to read more data to determine a command
    }
    command_base_t *base = (command_base_t *) buffer;
    switch (base->cmd) {
        case HELLO:
            if (size >= sizeof(hello_command_t)) {
                return handle_hello_command((hello_command_t *) buffer);
            }
            return {Server::INSUFFICIENT_DATA, 0, 0};
            break;
        case END:
            if (size >= sizeof(command_base_t)) {
                return handle_end_command((command_base_t *) buffer);
            }
            return {Server::INSUFFICIENT_DATA, 0, 0};
            break;
        case VARIABLE:
            if (size >= sizeof(variable_length_command_t)) {
                return handle_variable_length_command((variable_length_command_t *) buffer);
            }
            return {Server::INSUFFICIENT_DATA, 0, 0};
            break;
        default:
            printf("handle: Unknown command\n");
            return {Server::UNKNOWN_COMMAND, 0, 0};
            break;
    }
}

void data_listener(const void *buf, size_t size) {
    printf("Data size: %li\n", size);
    printf("Content:\n%.*s\n", (int) size, (char*) buf);
}

int main(int argc, char const *argv[]) { 
    Server server(SOCKET_PATH, 10);

    server.set_message_listener(handle_command);
    server.set_data_listener(data_listener);

    printf("Server initialized, starting loop...\n");

    server.start();
    
    return 0; 
} 