
// Server side C/C++ program to demonstrate Socket programming 
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <sys/un.h>
#include <stdlib.h> 
#include <string.h> 

#include "commands.h"

#define SOCKET_PATH "/tmp/server-test"

#define BUFFER_SIZE 1024

void handle_hello_command(const cuda_mango::hello_command_t *cmd) {
    printf("%s\n", cmd->message);
}

void handle_end_command(const cuda_mango::command_base_t *cmd) {
    (void) (cmd);
    exit(EXIT_SUCCESS);
}

void handle_commands(const char *buffer, ssize_t size) {
    size_t offset = 0;
    while(offset < size) {
        cuda_mango::command_base_t *base = (cuda_mango::command_base_t *)(buffer + offset);
        switch (base->cmd) {
            case cuda_mango::HELLO:
                handle_hello_command((cuda_mango::hello_command_t *)(buffer + offset));
                offset += sizeof(cuda_mango::hello_command_t);
                break;
            case cuda_mango::END:
                handle_end_command((cuda_mango::command_base_t *)(buffer + offset));
                offset += sizeof(cuda_mango::command_base_t);
                break;
            default:
                printf("Unknown command\n");
                break;
        }
    }
}

int main(int argc, char const *argv[]) 
{ 
    int server_fd, new_socket, valread; 
    struct sockaddr_un address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
    char *hello = "Hello from server"; 
       
    // Creating socket file descriptor 
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 

    address.sun_family = AF_UNIX; 
    strcpy(address.sun_path, SOCKET_PATH);

    unlink(address.sun_path);
       
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 

    if (listen(server_fd, 3) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 

    char buffer[BUFFER_SIZE];

    int loop = 0;

    while(1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) 
        { 
            perror("accept"); 
            exit(EXIT_FAILURE); 
        } 
        printf("loop %d\n", loop++);
        ssize_t bytes_read = read(new_socket, &buffer, BUFFER_SIZE);
        if (bytes_read < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        handle_commands(buffer, bytes_read);

        cuda_mango::command_base_t response = cuda_mango::create_ack_command();
        if (send(new_socket, &response, sizeof(response), 0) < 0) {
            perror("send");
            exit(EXIT_FAILURE);
        } 
        printf("Hello message sent\n");
    }

    printf("Here!");
     
    return 0; 
} 
