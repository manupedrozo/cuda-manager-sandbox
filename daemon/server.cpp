
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
#include <queue>

#include "commands.h"

#define SOCKET_PATH "/tmp/server-test"

#define BUFFER_SIZE 1024
#define NO_SOCKET -1

#define MAX_CONNECTIONS 10 // Max client connections
#define POLLFDS_SIZE MAX_CONNECTIONS + 1 // Clients + listen socket

static bool running = true;

typedef struct {
    void *buf;
    size_t size;
} message_t;

typedef struct {
    message_t *msg;
    size_t byte_offset;
} sending_message_t;

static pollfd                   pollfds[POLLFDS_SIZE]; 
static std::queue<message_t *>  message_queues[POLLFDS_SIZE]; // listen socket does not need to write, so we can use MAX_CONNECTIONS here, the difference could be confusing
static sending_message_t        messages_in_progress[POLLFDS_SIZE];

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

    printf("Bytes received: %li\n", bytes_read);

    handle_commands(buffer, bytes_read);

    cuda_mango::command_base_t *response = (cuda_mango::command_base_t *) malloc(sizeof(cuda_mango::command_base_t));
    response->cmd = cuda_mango::ACK;
    message_t *msg = (message_t *) malloc(sizeof(message_t));
    msg->buf = response;
    msg->size = sizeof(cuda_mango::command_base_t);
    message_queues[fd_idx].push(msg);
}

bool send_on_socket(int fd_idx) {
    printf("Sending data to %d\n", fd_idx);
    auto &curr_msg = messages_in_progress[fd_idx];
    auto &queue = message_queues[fd_idx];
    int fd = pollfds[fd_idx].fd;

    do {
        if (curr_msg.msg == NULL && !queue.empty()) {
            curr_msg.msg = queue.front();
            queue.pop();
        }
        if (curr_msg.msg != NULL) {
            size_t bytes_to_send = curr_msg.msg->size - curr_msg.byte_offset;
            ssize_t bytes_sent = send(fd, (char *) curr_msg.msg->buf + curr_msg.byte_offset, bytes_to_send, MSG_NOSIGNAL | MSG_DONTWAIT);
            printf("Bytes sent: %li\n", bytes_sent);
            if(bytes_sent == 0 || (bytes_sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
                printf("Can't send data right now, trying later\n");
                break;
            }
            else if(bytes_sent < 0) {
                perror("send");
                return false;
            } else {
                curr_msg.byte_offset += bytes_sent;
                if (curr_msg.byte_offset == curr_msg.msg->size) {
                    free(curr_msg.msg->buf);
                    free(curr_msg.msg);
                    curr_msg.msg = NULL;
                    curr_msg.byte_offset = 0;
                }
            }
        } 
    } while (curr_msg.msg != NULL || !queue.empty());
    return true;
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
                    if (!send_on_socket(i)) {
                        close_socket(i);
                    }
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
