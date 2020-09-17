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
#include <queue>

#include "server.h"

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

void Server::close_socket(int fd_idx) {
    if (fd_idx != listen_idx) sockets[fd_idx] = nullptr;
    else close(pollfds[fd_idx].fd);

    pollfds[fd_idx].fd = NO_SOCKET;
    pollfds[fd_idx].events = 0;
    pollfds[fd_idx].revents = 0;
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
            break;
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
    
    sockets[new_socket_idx] = std::make_unique<Server::Socket>(new_socket);
    sockets[new_socket_idx]->set_message_listener([this, new_socket_idx] (message_t msg) { return this->msg_listener(new_socket_idx, msg, *this); });
    sockets[new_socket_idx]->set_data_listener([this, new_socket_idx] (packet_t packet) { return this->data_listener(new_socket_idx, packet, *this); });

    printf("accept: New connection on %d (fd = %d)\n", new_socket_idx, new_socket);
    return true;
}

void Server::send_on_socket(int id, message_t msg) {
    sockets[id]->queue_message(msg);
}

void Server::check_for_writes() {
    for(int i = 0; i < max_connections; i++) {
        if (sockets[i] && sockets[i]->wants_to_write()) {
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
                        if (!sockets[i]->receive_messages()) {
                            printf("(loop %d) Receive error, closing socket\n", loop);
                            close_socket(i);
                            events_left--;
                            continue;
                        }
                    }
                }
                if(socket_events & POLLOUT) { // Ready to write
                    if (!sockets[i]->send_messages()) {
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
                    close_socket(i);
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
    running = false;
    end_server();
}

void Server::start() {
    if (!running) running = true;
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

Server::Socket::Socket(int fd): fd(fd) {}   

Server::Socket::~Socket() {
    printf("Destroying socket\n");

    for(int i = 0; i < message_queue.size(); i++) {
        message_t m = message_queue.front();
        free(m.buf);
        message_queue.pop();
    }
    
    if (sending_message.in_progress) {
        free(sending_message.msg.buf);
    }
    
    if (receiving_data.waiting) {
        free(receiving_data.data.buf);
    }

    close(fd);
}

bool Server::Socket::wants_to_write() {
    return !message_queue.empty() || sending_message.in_progress;
}

bool Server::Socket::send_messages() {
    printf("send: Sending data to %d\n", fd);

    do {
        if (!sending_message.in_progress && !message_queue.empty()) {
            sending_message.msg = message_queue.front();
            message_queue.pop();
            sending_message.in_progress = true;
        }
        if (sending_message.in_progress) {
            size_t bytes_to_send = sending_message.msg.size - sending_message.byte_offset;
            ssize_t bytes_sent = send(fd, (char *) sending_message.msg.buf + sending_message.byte_offset, bytes_to_send, MSG_NOSIGNAL | MSG_DONTWAIT);
            if(bytes_sent == 0 || (bytes_sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
                printf("send: Can't send data right now, trying later\n");
                break;
            }
            else if(bytes_sent < 0) {
                perror("send");
                return false;
            } else {
                printf("send: %li bytes sent\n", bytes_sent);
                sending_message.byte_offset += bytes_sent;
                if (sending_message.byte_offset == sending_message.msg.size) {
                    free(sending_message.msg.buf);
                    sending_message.in_progress = false;
                    sending_message.byte_offset = 0;
                }
            }
        } 
    } while (sending_message.in_progress || !message_queue.empty());

    return true;
}

bool Server::Socket::receive_messages() {
    printf("receive: Receiving on socket %d\n", fd);

    while(true) {
        const bool waiting_for_data = receiving_data.waiting;
        void *buf;
        size_t size_max;

        if (waiting_for_data) {
            buf = (char*) receiving_data.data.buf + receiving_data.byte_offset;
            size_max = receiving_data.data.size - receiving_data.byte_offset;
        } else {
            buf = receiving_message.buf + receiving_message.byte_offset;
            size_max = BUFFER_SIZE - receiving_message.byte_offset;
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
            receiving_data.byte_offset += bytes_read;
            consume_data_buffer();
        } else {
            receiving_message.byte_offset += bytes_read;
            if (!consume_message_buffer()) return false;
        }
    }
}

void Server::Socket::consume_data_buffer() {
    size_t offset = receiving_data.byte_offset;
    size_t expected_size = receiving_data.data.size;
    if (offset == expected_size) {
        handle_data_transfer_end();
    }
}

bool Server::Socket::consume_message_buffer() {
    size_t buffer_start = 0;
            
    do {
        if (!msg_listener) {
            printf("No msg_listener set\n");
            exit(EXIT_FAILURE);
        }
        size_t usable_buffer_size = receiving_message.byte_offset - buffer_start;
        message_result_t res = msg_listener({receiving_message.buf + buffer_start, usable_buffer_size});
        if (res.exit_code == UNKNOWN_MESSAGE) return false;
        else if (res.exit_code == INSUFFICIENT_DATA && usable_buffer_size == BUFFER_SIZE) {
            printf("receive: Buffer filled but a command couldn't be parsed\n");
            return false;
        }
        else if (res.exit_code == INSUFFICIENT_DATA) {
            memmove(receiving_message.buf, receiving_message.buf + buffer_start, usable_buffer_size);
            break;
        }
        else {
            if (res.expect_data > 0) {
                prepare_for_data_packet(res.expect_data, {receiving_message.buf + buffer_start, res.bytes_consumed});
            }
            buffer_start += res.bytes_consumed;
        }

        // it is possible that we handle a variable_length_command, which means that whatever is left on the buffer needs to be handled as pure data
        const bool data_in_buffer = receiving_data.waiting && buffer_start < receiving_message.byte_offset;
        if (data_in_buffer) {
            size_t data_to_transfer = receiving_message.byte_offset - buffer_start;
            printf("Moving %li bytes of message buffer data to variable data buffer\n", data_to_transfer);
            void* data_buffer = (char*) receiving_data.data.buf + receiving_data.byte_offset;
            memcpy(data_buffer, receiving_message.buf + buffer_start, data_to_transfer);
            buffer_start = receiving_message.byte_offset;
            receiving_data.byte_offset += data_to_transfer;
        }
    } while (buffer_start < receiving_message.byte_offset);

    receiving_message.byte_offset -= buffer_start;

    return true;
}

void Server::Socket::prepare_for_data_packet(size_t size, message_t msg) {
    receiving_data.waiting = true;
    receiving_data.data = { malloc(size), size };
    void *msg_buf_copy = malloc(msg.size);
    memcpy(msg_buf_copy, msg.buf, msg.size);
    receiving_data.msg = { msg_buf_copy, msg.size };
}

void Server::Socket::handle_data_transfer_end() {
    if (!data_listener) {
        printf("No data_listener set\n");
        exit(EXIT_FAILURE);
    }
    packet_t packet;
    packet.msg = receiving_data.msg;
    packet.extra_data = { receiving_data.data.buf, receiving_data.data.size };
    data_listener(packet);

    receiving_data.waiting = false;
    receiving_data.byte_offset = 0;
}

} //namespace cuda_mango