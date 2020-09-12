
#include <unistd.h> 
#include <fcntl.h>
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

typedef struct {
    char buf[BUFFER_SIZE];
    size_t byte_offset;
} receiving_message_t;

typedef struct {
    bool waiting;
    message_t *msg;
    size_t byte_offset;
} receiving_data_t;

static pollfd                   pollfds[POLLFDS_SIZE]; 
static std::queue<message_t *>  message_queues[POLLFDS_SIZE]; // listen socket does not need to write, so we can use MAX_CONNECTIONS here, the difference could be confusing
static sending_message_t        sending_messages[POLLFDS_SIZE];
static receiving_message_t      receiving_messages[POLLFDS_SIZE];
static receiving_data_t         receiving_data[POLLFDS_SIZE];

void handle_hello_command(const cuda_mango::hello_command_t *cmd) {
    printf("%s\n", cmd->message);
}

void handle_end_command(const cuda_mango::command_base_t *cmd) {
    (void) (cmd);
    running = false;
}

void handle_variable_length_command(int fd_idx, const cuda_mango::variable_length_command_t *cmd) {
    receiving_data[fd_idx].waiting = true;
    receiving_data[fd_idx].msg = (message_t*) malloc(sizeof(message_t));
    receiving_data[fd_idx].msg->buf = malloc(cmd->size);
    receiving_data[fd_idx].msg->size = cmd->size;
}

#define INSUFFICIENT_DATA -1
#define COMMAND_OK 1
#define UNKNOWN_COMMAND 0

int handle_commands(int fd_idx, const char *buffer, size_t size) {
    if (size < sizeof(cuda_mango::command_base_t)) {
        return INSUFFICIENT_DATA; // Need to read more data to determine a command
    }
    cuda_mango::command_base_t *base = (cuda_mango::command_base_t *) buffer;
    switch (base->cmd) {
        case cuda_mango::HELLO:
            if (size == sizeof(cuda_mango::hello_command_t)) {
                handle_hello_command((cuda_mango::hello_command_t *) buffer);
                return COMMAND_OK;
            }
            return INSUFFICIENT_DATA;
            break;
        case cuda_mango::END:
            if (size == sizeof(cuda_mango::command_base_t)) {
                handle_end_command((cuda_mango::command_base_t *) buffer);
                return COMMAND_OK;
            }
            return INSUFFICIENT_DATA;
            break;
        case cuda_mango::VARIABLE:
            if (size == sizeof(cuda_mango::variable_length_command_t)) {
                handle_variable_length_command(fd_idx, (cuda_mango::variable_length_command_t *) buffer);
                return COMMAND_OK;
            }
            return INSUFFICIENT_DATA;
            break;
        default:
            printf("Unknown command\n");
            return UNKNOWN_COMMAND;
            break;
    }
}

void handle_data_transfer_end(int fd_idx) {
    receiving_data[fd_idx].waiting = false;
    receiving_data[fd_idx].byte_offset = 0;

    // Use data in some way, whatever we call here is responsible for freeing the buffer.
        printf("Variable length data: \n%s\n", (char*) receiving_data[fd_idx].msg->buf);
        free(receiving_data[fd_idx].msg->buf);
    //

    free(receiving_data[fd_idx].msg);
    receiving_data[fd_idx].msg = NULL;
}

void initialize_pollfds() {
    for (int i = 0; i < POLLFDS_SIZE; i++) {
        pollfds[i].fd = NO_SOCKET;
        pollfds[i].events = 0;
        pollfds[i].revents = 0;
    }
}

void reset_socket_structs(int fd_idx) {
    pollfds[fd_idx].fd = NO_SOCKET;
    pollfds[fd_idx].events = 0;
    pollfds[fd_idx].revents = 0;

    // No other way to empty the queue unless we want to dequeue over and over
    std::queue<message_t *> empty;
    message_queues[fd_idx].swap(empty);
    
    sending_messages[fd_idx].byte_offset = 0;
    if (sending_messages[fd_idx].msg != NULL) {
        free(sending_messages[fd_idx].msg->buf);
        free(sending_messages[fd_idx].msg);
        sending_messages[fd_idx].msg = NULL;
    }
    
    receiving_messages[fd_idx].byte_offset = 0;

    receiving_data[fd_idx].waiting = false;
    receiving_data[fd_idx].byte_offset = 0;
    if (receiving_data[fd_idx].msg != NULL) {
        free(receiving_data[fd_idx].msg->buf);
        free(receiving_data[fd_idx].msg);
        receiving_data[fd_idx].msg = NULL;
    }
}

void close_socket(int fd_idx) {
    printf("Closing socket %d\n", fd_idx);
    close(pollfds[fd_idx].fd);
    reset_socket_structs(fd_idx);
}

void close_sockets() {
    for(int i = 0; i < POLLFDS_SIZE; i++) {
        if(pollfds[i].fd != NO_SOCKET) {
            close_socket(i);
        }
    }
}

bool accept_new_connection() {
    int server_fd = pollfds[0].fd;
    int new_socket = accept(server_fd, NULL, NULL);
    if (new_socket < 0) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("accept: No connection available, trying again later\n");
            return true;
        } else {
            perror("accept");
            return false;
        }
    }
    for(int i = 1; i < POLLFDS_SIZE; i++) {
        if(pollfds[i].fd == NO_SOCKET) {
            pollfds[i].fd = new_socket;
            pollfds[i].events = POLLIN | POLLPRI;
            pollfds[i].revents = 0;
            printf("New connection on %d\n", i);
            break;
        }
    }
    return true;
}

bool receive_on_socket(int fd_idx) {
    int fd = pollfds[fd_idx].fd;

    while(true) {
        const bool waiting_for_data = receiving_data[fd_idx].waiting;
        void *buf;
        size_t size_max;

        if (waiting_for_data) {
            buf = (char*) receiving_data[fd_idx].msg->buf + receiving_data[fd_idx].byte_offset;
            size_max = receiving_data[fd_idx].msg->size - receiving_data[fd_idx].byte_offset;
        } else {
            buf = receiving_messages[fd_idx].buf + receiving_messages[fd_idx].byte_offset;
            size_max = BUFFER_SIZE - receiving_messages[fd_idx].byte_offset;
        }

        ssize_t bytes_read = recv(fd, buf, size_max, MSG_NOSIGNAL | MSG_DONTWAIT);
        if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return true;
        }
        else if (bytes_read < 0) {
            perror("read");
            return false;
        }
        else if(bytes_read == 0) {
            printf("0 bytes received, got hang up on\n");
            return false;
        }

        printf("Bytes received: %li\n", bytes_read);

        if (waiting_for_data) {
            receiving_data[fd_idx].byte_offset += bytes_read;
            size_t offset = receiving_data[fd_idx].byte_offset;
            size_t expected_size = receiving_data[fd_idx].msg->size;
            if (offset == expected_size) {
                handle_data_transfer_end(fd_idx);

                cuda_mango::command_base_t *response = (cuda_mango::command_base_t *) malloc(sizeof(cuda_mango::command_base_t));
                cuda_mango::init_ack_command(*response);
                message_t *msg = (message_t *) malloc(sizeof(message_t));
                msg->buf = response;
                msg->size = sizeof(cuda_mango::command_base_t);
                message_queues[fd_idx].push(msg);
            }
        } else {
            receiving_messages[fd_idx].byte_offset += bytes_read;

            int res = handle_commands(fd_idx, receiving_messages[fd_idx].buf, receiving_messages[fd_idx].byte_offset);
            if (res == UNKNOWN_COMMAND) return false;
            else if (res == INSUFFICIENT_DATA && receiving_messages[fd_idx].byte_offset == BUFFER_SIZE) {
                printf("Buffer filled but a command couldn't be parsed\n");
                return false;
            }
            else if (res == COMMAND_OK) {
                receiving_messages[fd_idx].byte_offset = 0;

                cuda_mango::command_base_t *response = (cuda_mango::command_base_t *) malloc(sizeof(cuda_mango::command_base_t));
                cuda_mango::init_ack_command(*response);
                message_t *msg = (message_t *) malloc(sizeof(message_t));
                msg->buf = response;
                msg->size = sizeof(cuda_mango::command_base_t);
                message_queues[fd_idx].push(msg);
            } 
        }
    }
}

bool send_on_socket(int fd_idx) {
    printf("Sending data to %d\n", fd_idx);
    auto &curr_msg = sending_messages[fd_idx];
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
            if(bytes_sent == 0 || (bytes_sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
                printf("Can't send data right now, trying later\n");
                break;
            }
            else if(bytes_sent < 0) {
                perror("send");
                return false;
            } else {
                printf("Bytes sent: %li\n", bytes_sent);
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
        if(!message_queues[i].empty()) {
            pollfds[i].events |= POLLOUT;
        } else {
            pollfds[i].events &= ~POLLOUT;
        }
    }
}

void initialize_server() {
    initialize_pollfds();
    
    int &server_fd = pollfds[0].fd;
    pollfds[0].events = POLLIN | POLLPRI;
    struct sockaddr_un address; 
       
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == 0) { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 

    int flags = fcntl(server_fd, F_GETFL);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    address.sun_family = AF_UNIX; 
    strcpy(address.sun_path, SOCKET_PATH);

    unlink(address.sun_path);
       
    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 

    if (listen(server_fd, 3) < 0)  { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 
}

void server_loop() {
    int loop = 0;

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
                        if(!accept_new_connection()) {
                            printf("(loop %d) Accept error, closing socket\n", loop);
                            close_sockets();
                            exit(EXIT_FAILURE);
                        }
                    } else {
                        if (!receive_on_socket(i)) {
                            printf("(loop %d) Receive error, closing socket\n", loop);
                            close_socket(i);
                            events_left--;
                            continue;
                        }
                    }
                }
                if(socket_events & POLLOUT) { // Ready to write
                    if (!send_on_socket(i)) {
                        printf("(loop %d) Send error, closing socket\n", loop);
                        close_socket(i);
                        events_left--;
                        continue;
                    }
                }
                if(socket_events & POLLPRI) { // Exceptional condition (very rare)
                    printf("(loop %d) Exceptional condition on idx %d\n", loop, i);
                    close_socket(i);
                    events_left--;
                    continue;
                }
                if(socket_events & POLLERR) { // Error / read end of pipe is closed
                    printf("(loop %d) Error on idx %d\n", loop, i); // errno?
                    close_socket(i);
                    events_left--;
                    continue;
                }
                if(socket_events & POLLHUP) { // Other end closed connection, some data may be left to read
                    printf("(loop %d) Got hang up on %d\n", loop, i);
                    close_socket(i);
                    events_left--;
                    continue;
                }
                if(socket_events & POLLNVAL) { // Invalid request, fd not open
                    printf("(loop %d) Socket %d at index %d is closed\n", loop, pollfds[i].fd, i);
                    reset_socket_structs(i);
                    events_left--;
                    continue;
                }
                pollfds[i].revents = 0;
                events_left--;
            }
        }
        loop++;
    }     
}

void end_server() {
    printf("Finishing up\n");
    close_sockets();
}   

int main(int argc, char const *argv[]) 
{ 
    initialize_server();

    server_loop();

    end_server();
    
    return 0; 
} 
