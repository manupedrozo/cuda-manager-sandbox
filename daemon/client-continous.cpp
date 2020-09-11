
// Client side C/C++ program to demonstrate Socket programming 
#include <stdio.h> 
#include <iostream>

#include "commands.h"
#include "client.h"

#define SOCKET_PATH "/tmp/server-test"

int main(int argc, char const *argv[]) { 
    int sock = initialize(SOCKET_PATH);
    
    if (sock == -1) {
        exit(EXIT_FAILURE);
    }

    while (true) {
        printf("Type your message\n");
        char msg_buf[128];

        std::cin >> msg_buf;

        printf("Message: %s\n", msg_buf);
        cuda_mango::hello_command_t hello_cmd = cuda_mango::create_hello_command(msg_buf);

        cuda_mango::command_base_t res;

        if(!send_on_socket(sock, &hello_cmd, sizeof(hello_cmd))) {
            return EXIT_FAILURE;
        }
        
        printf("Hello message sent\n"); 
        
        if(!receive_on_socket(sock, &res, sizeof(res))) {
            return EXIT_FAILURE;
        }
    }

    end(sock);
    
    return EXIT_SUCCESS; 
} 
