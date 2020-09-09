
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <poll.h>
#include <sys/un.h>
#include <stdlib.h> 
#include <string.h> 
#include <errno.h>
#include <iostream>
#include <vector>

#include "commands.h"

#define SOCKET_PATH "/tmp/server-test"

#define BUFFER_SIZE 1024
#define NO_SOCKET -1

#define MAX_CONNECTIONS 10 // Max client connections
#define POLLFDS_SIZE MAX_CONNECTIONS + 1 // Clients + listen socket

static bool running = true;

typedef struct {
    void* buf;
    size_t size;
} message_t;

static pollfd pollfds[POLLFDS_SIZE]; 
static std::vector<message_t> message_queues[POLLFDS_SIZE]; // listen socket does not need to write, so we can use MAX_CONNECTIONS here, the difference could be confusing

void handle_hello_command(const cuda_mango::hello_command_t *cmd) {
    printf("%s\n", cmd->message);
}

void handle_end_command(const cuda_mango::command_base_t *cmd) {
    (void) (cmd);
    running = false;
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

void initialize_pollfds() {
    for (int i = 0; i < POLLFDS_SIZE; i++) {
        pollfds[i].fd = NO_SOCKET;
        pollfds[i].events = 0;
        pollfds[i].revents = 0;
    }
}

void close_socket(int fd_idx) {
    close(pollfds[fd_idx].fd);
    pollfds[fd_idx].fd = NO_SOCKET;
    pollfds[fd_idx].events = 0;
    pollfds[fd_idx].revents = 0;
}

void close_sockets() {
    for(int i = 0; i < POLLFDS_SIZE; i++) {
        if(pollfds[i].fd != NO_SOCKET) {
            close_socket(i);
        }
    }
}

void accept_new_connection() {
    int server_fd = pollfds[0].fd;
    int new_socket = accept(server_fd, NULL, NULL);
    if (new_socket < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    for(int i = 1; i < POLLFDS_SIZE; i++) {
        if(pollfds[i].fd == NO_SOCKET) {
            pollfds[i].fd = new_socket;
            pollfds[i].events = POLLIN | POLLPRI;
            pollfds[i].revents = 0;
            break;
        }
    }
}

void receive_on_socket(int fd_idx) {
    int fd = pollfds[fd_idx].fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(fd, &buffer, BUFFER_SIZE, 0);

    if (bytes_read < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    else if(bytes_read == 0) return;

    printf("Bytes read:%li\n", bytes_read);

    handle_commands(buffer, bytes_read);

    cuda_mango::command_base_t *response = (cuda_mango::command_base_t *) malloc(sizeof(cuda_mango::command_base_t));
    response->cmd = cuda_mango::ACK;
    message_queues[fd_idx].push_back({response, sizeof(cuda_mango::command_base_t)});
}

void send_on_socket(int fd_idx) {
    printf("Sending data to %d\n", fd_idx);
    int fd = pollfds[fd_idx].fd;
    auto &queue = message_queues[fd_idx];
    for(auto &msg : queue) {
        if (send(fd, msg.buf, msg.size, MSG_NOSIGNAL) < 0) {
            if (errno == EPIPE) {
                printf("Socket disconnected\n"); // What do we do here?
                exit(EXIT_FAILURE);
            } else {
                perror("send");
                exit(EXIT_FAILURE);
            }
        } else {
            free(msg.buf);
            printf("Ack sent\n");
        }
    }
    queue.clear();
}

void check_for_writes() {
    for(int i = 0; i < POLLFDS_SIZE; i++) {
        if(message_queues[i].size() > 0) {
            pollfds[i].events |= POLLOUT;
        } else {
            pollfds[i].events &= ~POLLOUT;
        }
    }
}

int main(int argc, char const *argv[]) 
{ 
    initialize_pollfds();
    
    int &server_fd = pollfds[0].fd;
    pollfds[0].events = POLLIN | POLLPRI;
    struct sockaddr_un address; 
       
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

    int loop = 0;

    char continue_buf[8];
    while(running) {
        check_for_writes();
        int events = poll(pollfds, POLLFDS_SIZE, -1 /* -1 == block until events are received */); 

        if (events == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        for(int i = 0, events_left = events; i < POLLFDS_SIZE && events_left > 0; i++) {
            if(pollfds[i].revents) {
                auto socket_events = pollfds[i].revents;
                if(socket_events & POLLIN) { // Ready to read
                    if(i == 0) { // Listen socket, new connection available
                        accept_new_connection();
                    } else {
                        receive_on_socket(i);
                    }
                }
                if(socket_events & POLLOUT) { // Ready to write
                    send_on_socket(i);
                }
                if(socket_events & POLLPRI) { // Exceptional condition (very rare)
                    printf("Exceptional condition on idx %d\n", i);
                }
                if(socket_events & POLLERR) { // Error / read end of pipe is closed
                    printf("Error on idx %d\n", i); // errno?
                }
                if(socket_events & POLLHUP) { // Other end closed connection, some data may be left to read
                    printf("Got hang up on %d\n", i);
                    close_socket(i);
                }
                if(socket_events & POLLNVAL) { // Invalid request, fd not open
                    printf("Socket %d at index %d is closed\n", pollfds[i].fd, i);
                    exit(EXIT_FAILURE);
                }
                pollfds[i].revents = 0;
                events_left--;
            }
        }
    }     

    printf("Finishing up\n");
    close_sockets();

    return 0; 
} 
