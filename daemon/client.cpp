#include <stdio.h>
#include <sys/socket.h> 
#include <sys/un.h>
#include <unistd.h>
#include <string.h>

#include "client.h"

int initialize(const char *socket_path) {
    int sock; 
    struct sockaddr_un serv_addr; 
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) { 
        printf("Socket creation error\n"); 
        return -1; 
    } 
   
    serv_addr.sun_family = AF_UNIX; 
    strcpy(serv_addr.sun_path, socket_path);
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
        printf("Connection Failed\n"); 
        return -1; 
    } 

    return sock;
}

bool send_on_socket(int fd, void *buf, size_t size) {
    size_t byte_offset = 0;

    while (byte_offset < size) {
        size_t bytes_to_send = size - byte_offset;
        ssize_t bytes_sent = send(fd, (char *) buf + byte_offset, bytes_to_send, MSG_NOSIGNAL);
        if (bytes_sent <= 0) {
            perror("send");
            return false;
        } else {
            printf("Bytes sent: %li\n", bytes_sent);
            byte_offset += bytes_sent;
        }
    }

    return true;
}

bool receive_on_socket(int fd, void *buf, size_t size) {
    size_t byte_offset = 0;

    while (byte_offset < size) {
        size_t bytes_to_read = size - byte_offset;
        ssize_t bytes_received = recv(fd, (char *) buf + byte_offset, bytes_to_read, MSG_NOSIGNAL);
        if (bytes_received <= 0) {
            perror("send");
            return false;
        } else {
            printf("Bytes received: %li\n", bytes_received);
            byte_offset += bytes_received;
        }
    }

    return true;
}

void end(int fd) {
    close(fd);
}