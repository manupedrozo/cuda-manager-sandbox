
#include <stdio.h> 
#include <string>
#include <fstream>
#include <sstream>

#include "commands.h"
#include "client.h"

#define SOCKET_PATH "/tmp/server-test"

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

    auto file_stream = std::ifstream(TEST_FILE_PATH);
    auto lorem = slurp(file_stream);

    printf("%s\n", lorem.c_str());

    cuda_mango::variable_length_command_t cmd;
    cuda_mango::init_variable_length_command(cmd, lorem.size());

    cuda_mango::command_base_t res;

    if(!send_on_socket(sock, &cmd, sizeof(cmd))) {
        return EXIT_FAILURE;
    } 

    printf("Variable length command sent\n"); 

    if(!receive_on_socket(sock, &res, sizeof(res))) {
        return EXIT_FAILURE;
    }

    if (res.cmd == cuda_mango::ACK) {
        printf("Server ack received\n");
    } else {
        return EXIT_FAILURE;
    }

    for(int i = 0; i < lorem.size(); i += 128) {
        if(!send_on_socket(sock, (char*) lorem.c_str() + i, std::min(128UL, lorem.size() - i))) {
            return EXIT_FAILURE;
        }
    }

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