#include <stdio.h> 
#include <fstream>
#include <sstream>

#include "commands.h"
#include "socket_client.h"

#define SOCKET_PATH "/tmp/server-test"

#define MESSAGE_AMOUNT 20

#define TEST_FILE_PATH "lorem.txt"
   
std::string slurp(std::ifstream& in) {
    std::ostringstream sstr;
    sstr << in.rdbuf();
    return sstr.str();
}

int main(int argc, char const *argv[]) { 
    int sock = initialize(SOCKET_PATH);
    
    if (sock == -1) {
        exit(EXIT_FAILURE);
    }

    cuda_mango::command_base_t res;

    cuda_mango::hello_command_t hello_cmd;
    cuda_mango::init_hello_command(hello_cmd, "Hello from client");

    printf("Trying to send %d hello messages\n", MESSAGE_AMOUNT);

    char buf[sizeof(hello_cmd) * MESSAGE_AMOUNT];

    size_t offset = 0;
    for(int i = 0; i < MESSAGE_AMOUNT; i++) {
        memcpy(buf + i * sizeof(hello_cmd), &hello_cmd, sizeof(hello_cmd));
    }
    
    if(!send_on_socket(sock, &buf, sizeof(hello_cmd) * MESSAGE_AMOUNT)) {
        return EXIT_FAILURE;
    } 

    printf("Hello messages sent\n"); 

    for(int i = 0; i < MESSAGE_AMOUNT; i++) {
        if(!receive_on_socket(sock, &res, sizeof(res))) {
            return EXIT_FAILURE;
        }

        if (res.cmd == cuda_mango::ACK) {
            printf("Server ack received\n");
        } else {
            return EXIT_FAILURE;
        }
    }

    printf("Trying to send variable length message and data at the same time\n");

    auto file_stream = std::ifstream(TEST_FILE_PATH);
    auto lorem = slurp(file_stream);

    cuda_mango::variable_length_command_t cmd;
    cuda_mango::init_variable_length_command(cmd, lorem.size());

    printf("File size %li\n", lorem.size());

    size_t variable_len_buffer_size = sizeof(cmd) + lorem.size();
    char var_buffer[variable_len_buffer_size];
    memcpy(var_buffer, &cmd, sizeof(cmd));
    memcpy(var_buffer + sizeof(cmd), lorem.c_str(), lorem.size());

    if(!send_on_socket(sock, var_buffer, variable_len_buffer_size)) {
        return EXIT_FAILURE;
    }

    //variable_length_command ACK
    if(!receive_on_socket(sock, &res, sizeof(res))) {
        return EXIT_FAILURE;
    }

    if (res.cmd == cuda_mango::ACK) {
        printf("Server ack received\n");
    } else {
        return EXIT_FAILURE;
    }

    //data received ACK
    if(!receive_on_socket(sock, &res, sizeof(res))) {
        return EXIT_FAILURE;
    }

    if (res.cmd == cuda_mango::ACK) {
        printf("Server ack received\n");
    } else {
        return EXIT_FAILURE;
    }
    
    end(sock);
    
    return EXIT_SUCCESS; 
} 
