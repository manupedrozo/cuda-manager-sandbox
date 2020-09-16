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

#include "server.h"
#include "commands.h"

#define NO_SOCKET -1

void print_pollfds(pollfd *data, size_t size) {
    printf("Pollfds\n");
    for(int i = 0; i < size; i++) {
        printf("pollfd_idx: %d, ", i);
        printf("fd: %d, ", data[i].fd);
        printf("events: %hi, ", data[i].events);
        printf("revents: %hi\n", data[i].revents);
    }
}

namespace cuda_mango {

void Server::prepare_for_data_packet(int fd_idx, size_t size) {
    receiving_data[fd_idx].waiting = true;
    receiving_data[fd_idx].msg = (message_t*) malloc(sizeof(message_t));
    receiving_data[fd_idx].msg->buf = malloc(size);
    receiving_data[fd_idx].msg->size = size;
}

void Server::handle_data_transfer_end(int fd_idx) {
    if (!data_listener) {
        printf("No data_listener set\n");
        exit(EXIT_FAILURE);
    }
    // Data listener is responsible for freeing the buffer.
    data_listener(receiving_data[fd_idx].msg->buf, receiving_data[fd_idx].msg->size);

    receiving_data[fd_idx].waiting = false;
    receiving_data[fd_idx].byte_offset = 0;

    free(receiving_data[fd_idx].msg);
    receiving_data[fd_idx].msg = nullptr;
}

void Server::reset_socket_structs(int fd_idx) {
    pollfds[fd_idx].fd = NO_SOCKET;
    pollfds[fd_idx].events = 0;
    pollfds[fd_idx].revents = 0;

    // No other way to empty the queue unless we want to dequeue over and over
    std::queue<message_t *> empty;
    message_queues[fd_idx].swap(empty);
    
    sending_messages[fd_idx].byte_offset = 0;
    if (sending_messages[fd_idx].msg != nullptr) {
        free(sending_messages[fd_idx].msg->buf);
        free(sending_messages[fd_idx].msg);
        sending_messages[fd_idx].msg = nullptr;
    }
    
    receiving_messages[fd_idx].byte_offset = 0;

    receiving_data[fd_idx].waiting = false;
    receiving_data[fd_idx].byte_offset = 0;
    if (receiving_data[fd_idx].msg != nullptr) {
        free(receiving_data[fd_idx].msg->buf);
        free(receiving_data[fd_idx].msg);
        receiving_data[fd_idx].msg = nullptr;
    }
}

void Server::close_socket(int fd_idx) {
    printf("close: Closing socket %d\n", fd_idx);
    close(pollfds[fd_idx].fd);
    if (fd_idx != listen_idx) {
        reset_socket_structs(fd_idx);
    }
}

void Server::close_sockets() {
    for(int i = 0; i < pollfds.size(); i++) {
        if(pollfds[i].fd != NO_SOCKET) {
            close_socket(i);
        }
    }
}

bool Server::accept_new_connection() {
    int server_fd = pollfds[listen_idx].fd;
    int new_socket = accept(server_fd, nullptr, nullptr);
    if (new_socket < 0) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("accept: No connection available, trying again later\n");
            return true;
        } else {
            perror("accept");
            return false;
        }
    }
    int new_socket_idx = -1;
    for(int i = 0; i < max_connections; i++) {
        if(pollfds[i].fd == NO_SOCKET) {
            new_socket_idx = i;
        }
    }
    if (new_socket_idx == -1) {
        printf("accept: Connection limit reached, rejecting connection\n");
        close(new_socket);
        return true;
    }
    pollfds[new_socket_idx].fd = new_socket;
    pollfds[new_socket_idx].events = POLLIN | POLLPRI;
    pollfds[new_socket_idx].revents = 0;
    printf("accept: New connection on %d\n", new_socket_idx);
    return true;
}

void Server::send_ack(int fd_idx) {
    cuda_mango::command_base_t *response = (cuda_mango::command_base_t *) malloc(sizeof(cuda_mango::command_base_t));
    cuda_mango::init_ack_command(*response);
    send_on_socket(fd_idx, response, sizeof(cuda_mango::command_base_t));
}

bool Server::consume_message_buffer(int fd_idx) {
    size_t buffer_start = 0;
            
    do {
        if (!msg_listener) {
            printf("No msg_listener set\n");
            exit(EXIT_FAILURE);
        }
        size_t usable_buffer_size = receiving_messages[fd_idx].byte_offset - buffer_start;
        command_result_t res = msg_listener(receiving_messages[fd_idx].buf + buffer_start, usable_buffer_size);
        if (res.exit_code == UNKNOWN_COMMAND) return false;
        else if (res.exit_code == INSUFFICIENT_DATA && usable_buffer_size == BUFFER_SIZE) {
            printf("receive: Buffer filled but a command couldn't be parsed\n");
            return false;
        }
        else if (res.exit_code == INSUFFICIENT_DATA) {
            memmove(receiving_messages[fd_idx].buf, receiving_messages[fd_idx].buf + buffer_start, usable_buffer_size);
            break;
        }
        else {
            buffer_start += res.bytes_consumed;
            if (res.expect_data > 0) {
                prepare_for_data_packet(fd_idx, res.expect_data);
            }
            send_ack(fd_idx);
        }

        // it is possible that we handle a variable_length_command, which means that whatever is left on the buffer needs to be handled as pure data
        const bool data_in_buffer = receiving_data[fd_idx].waiting && buffer_start < receiving_messages[fd_idx].byte_offset;
        if (data_in_buffer) {
            size_t data_to_transfer = receiving_messages[fd_idx].byte_offset - buffer_start;
            printf("Moving %li bytes of message buffer data to variable data buffer\n", data_to_transfer);
            void* data_buffer = (char*) receiving_data[fd_idx].msg->buf + receiving_data[fd_idx].byte_offset;
            memcpy(data_buffer, receiving_messages[fd_idx].buf + buffer_start, data_to_transfer);
            buffer_start = receiving_messages[fd_idx].byte_offset;
            receiving_data[fd_idx].byte_offset += data_to_transfer;
        }
    } while (buffer_start < receiving_messages[fd_idx].byte_offset);

    receiving_messages[fd_idx].byte_offset -= buffer_start;

    return true;
}

void Server::send_on_socket(int id, void *buf, size_t size) {
    message_t *msg = (message_t *) malloc(sizeof(message_t));
    msg->buf = buf;
    msg->size = size;
    message_queues[id].push(msg);
}

void Server::consume_data_buffer(int fd_idx) {
    size_t offset = receiving_data[fd_idx].byte_offset;
    size_t expected_size = receiving_data[fd_idx].msg->size;
    if (offset == expected_size) {
        handle_data_transfer_end(fd_idx);
        send_ack(fd_idx);
    }
}

bool Server::receive_on_socket(int fd_idx) {
    printf("receive: Receiving on socket %d\n", fd_idx);
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
            perror("receive (read)");
            return false;
        }
        else if(bytes_read == 0) {
            printf("receive: 0 bytes received, got hang up on\n");
            return false;
        }

        printf("receive: %li bytes received\n", bytes_read);

        if (waiting_for_data) {
            receiving_data[fd_idx].byte_offset += bytes_read;
            consume_data_buffer(fd_idx);
        } else {
            receiving_messages[fd_idx].byte_offset += bytes_read;
            if (!consume_message_buffer(fd_idx)) return false;
        }
    }
}

bool Server::send_on_socket(int fd_idx) {
    printf("send: Sending data to %d\n", fd_idx);
    auto &curr_msg = sending_messages[fd_idx];
    auto &queue = message_queues[fd_idx];
    int fd = pollfds[fd_idx].fd;

    do {
        if (curr_msg.msg == nullptr && !queue.empty()) {
            curr_msg.msg = queue.front();
            queue.pop();
        }
        if (curr_msg.msg != nullptr) {
            size_t bytes_to_send = curr_msg.msg->size - curr_msg.byte_offset;
            ssize_t bytes_sent = send(fd, (char *) curr_msg.msg->buf + curr_msg.byte_offset, bytes_to_send, MSG_NOSIGNAL | MSG_DONTWAIT);
            if(bytes_sent == 0 || (bytes_sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
                printf("send: Can't send data right now, trying later\n");
                break;
            }
            else if(bytes_sent < 0) {
                perror("send");
                return false;
            } else {
                printf("send: %li bytes sent\n", bytes_sent);
                curr_msg.byte_offset += bytes_sent;
                if (curr_msg.byte_offset == curr_msg.msg->size) {
                    free(curr_msg.msg->buf);
                    free(curr_msg.msg);
                    curr_msg.msg = nullptr;
                    curr_msg.byte_offset = 0;
                }
            }
        } 
    } while (curr_msg.msg != nullptr || !queue.empty());

    return true;
}

void Server::check_for_writes() {
    for(int i = 0; i < max_connections; i++) {
        if(!message_queues[i].empty() || sending_messages[i].msg != nullptr) {
            pollfds[i].events |= POLLOUT;
        } else {
            pollfds[i].events &= ~POLLOUT;
        }
    }
}

void Server::initialize_server(const char* socket_path) {
    for(auto& pollfd: pollfds) {
        pollfd.fd = NO_SOCKET;
        pollfd.events = 0;
        pollfd.revents = 0;
    }

    int server_fd = -1;
    pollfds[listen_idx].events = POLLIN | POLLPRI;

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == 0) { 
        perror("init (socket)"); 
        exit(EXIT_FAILURE); 
    } 

    int flags = fcntl(server_fd, F_GETFL);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_un address;
    address.sun_family = AF_UNIX; 
    strcpy(address.sun_path, socket_path);

    unlink(address.sun_path);
       
    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) { 
        perror("init (bind)"); 
        exit(EXIT_FAILURE); 
    } 

    if (listen(server_fd, 3) < 0)  { 
        perror("init (listen)"); 
        exit(EXIT_FAILURE); 
    } 

    pollfds[listen_idx].fd = server_fd;
}

void Server::server_loop() {
    int loop = 0;

    while(running) {
        check_for_writes();

        int events = poll(pollfds.data(), pollfds.size(), -1 /* -1 == block until events are received */); 
        if (events == -1) {
            perror("loop (poll)");
            exit(EXIT_FAILURE);
        }

        for(int i = 0, events_left = events; i < pollfds.size() && events_left > 0; i++) {
            if(pollfds[i].revents) {
                auto socket_events = pollfds[i].revents;
                if(socket_events & POLLIN) { // Ready to read
                    if(i == listen_idx) { // Listen socket, new connection available
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

void Server::end_server() {
    printf("end: Finishing up\n");
    close_sockets();
}   

Server::Server(
    const char *socket_path, 
    int max_connections
): max_connections(max_connections) {
    initialize_server(socket_path);
}

Server::~Server() {
    end_server();
}

void Server::start() {
    if (!msg_listener) {
        printf("No msg_listener set\n");
        exit(EXIT_FAILURE);
    }
    if (!data_listener) {
        printf("No data_listener set\n");
        exit(EXIT_FAILURE);
    }
    server_loop();
}

} //namespace cuda_mango