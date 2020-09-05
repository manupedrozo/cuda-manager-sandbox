
// Client side C/C++ program to demonstrate Socket programming 
#include <stdio.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h> 

#include "commands.h"

#define SOCKET_PATH "/tmp/server-test"
   
int main(int argc, char const *argv[]) 
{ 
    int sock = 0, valread; 
    struct sockaddr_un serv_addr; 
    char *hello = "Hello from client"; 
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        return EXIT_FAILURE; 
    } 
   
    serv_addr.sun_family = AF_UNIX; 
    strcpy(serv_addr.sun_path, SOCKET_PATH);
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("\nConnection Failed \n"); 
        return EXIT_FAILURE; 
    } 

    cuda_mango::hello_command_t hello_cmd = cuda_mango::create_hello_command(hello);

    cuda_mango::command_base_t res;

    send(sock, &hello_cmd, sizeof(hello_cmd), 0); 
    printf("Hello message sent\n"); 
    valread = read(sock, &res, sizeof(res)); 
    if (res.cmd == cuda_mango::ACK) {
        printf("Server ack received\n");
    } else {
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS; 
} 
