#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <string.h>
#include <errno.h>

#include "server.h"
#include "logger.h"

#define NO_SOCKET -1

namespace cuda_mango {

static Logger &logger = Logger::get_instance();

void Server::close_socket(int fd_idx) {
    logger.debug("close_socket: Closing socket {}", fd_idx);
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

Server::AcceptConnectionExitCode Server::accept_new_connection() {
    int server_fd = pollfds[listen_idx].fd;
    int new_socket = accept(server_fd, nullptr, nullptr);
    if (new_socket < 0) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            logger.debug("accept: No connection available, trying again later");
            return AcceptConnectionExitCode::OK;
        } else {
            logger.error("accept: {}", strerror(errno));
            return AcceptConnectionExitCode::ERROR;
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
        logger.error("accept: Connection limit reached, rejecting connection");
        close(new_socket);
        return AcceptConnectionExitCode::OK;
    }
    pollfds[new_socket_idx].fd = new_socket;
    pollfds[new_socket_idx].events = POLLIN | POLLPRI;
    pollfds[new_socket_idx].revents = 0;
    
    auto socket_msg_listener = [this, new_socket_idx] (message_t msg) { return this->msg_listener(new_socket_idx, msg, *this); };
    auto socket_data_listener = [this, new_socket_idx] (packet_t packet) { return this->data_listener(new_socket_idx, packet, *this); };
    sockets[new_socket_idx] = std::make_unique<Server::Socket>(new_socket, socket_msg_listener, socket_data_listener);

    logger.info("accept: New connection on {} (fd = {})", new_socket_idx, new_socket);
    return AcceptConnectionExitCode::OK;
}

void Server::send_on_socket(int id, message_t msg) {
    sockets[id]->queue_message(msg);
}

void Server::check_for_writes() {
    for(int i = 0; i < max_connections; i++) {
        if (sockets[i] && sockets[i]->wants_to_write()) {
            logger.debug("check_for_writes: Incoming data on socket {}", i);
            pollfds[i].events |= POLLOUT;
        } else {
            pollfds[i].events &= ~POLLOUT;
        }
    }
}

Server::InitExitCode Server::initialize() {
    logger.info("initialize: Initializing server");

    for(auto& pollfd: pollfds) {
        pollfd.fd = NO_SOCKET;
        pollfd.events = 0;
        pollfd.revents = 0;
    }

    int server_fd = -1;
    pollfds[listen_idx].events = POLLIN | POLLPRI;

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == 0) { 
        logger.critical("initialize (socket): {}", strerror(errno)); 
        return InitExitCode::ERROR;
    } 

    int flags = fcntl(server_fd, F_GETFL);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_un address;
    address.sun_family = AF_UNIX; 
    strcpy(address.sun_path, socket_path.c_str());

    unlink(address.sun_path);
       
    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) { 
        logger.critical("initialize (bind): {}", strerror(errno)); 
        return InitExitCode::ERROR;
    } 

    if (listen(server_fd, 3) < 0)  { 
        logger.critical("initialize (listen): {}", strerror(errno)); 
        return InitExitCode::ERROR;
    } 

    pollfds[listen_idx].fd = server_fd;

    initialized = true;
    return InitExitCode::OK;
}

Server::StartExitCode Server::server_loop() {
    unsigned int loop = 0;

    while(running) {
        check_for_writes();

        int events = poll(pollfds.data(), pollfds.size(), -1 /* -1 == block until events are received */); 
        if (events == -1) {
            logger.critical("server_loop (poll): {}", strerror(errno));
            return StartExitCode::ERROR;
        }

        for(int i = 0, events_left = events; i < pollfds.size() && events_left > 0; i++) {
            if(pollfds[i].revents) {
                auto socket_events = pollfds[i].revents;
                if(socket_events & POLLIN) { // Ready to read
                    if(i == listen_idx) { // Listen socket, new connection available
                        switch (accept_new_connection()) {
                            case Server::AcceptConnectionExitCode::ERROR:
                                logger.critical("server_loop ({}): Accept error, ending server", loop);
                                close_sockets();
                                return StartExitCode::ERROR;
                                break;
                            case Server::AcceptConnectionExitCode::OK:
                                break;
                        }
                    } else {
                        switch (sockets[i]->receive_messages()) {
                            case Socket::ReceiveMessagesExitCode::ERROR:
                                logger.error("server_loop ({}): Receive error", loop);
                                close_socket(i);
                                events_left--;
                                continue;
                                break;
                            case Socket::ReceiveMessagesExitCode::HANG_UP:
                                logger.info("server_loop ({}): Client closed connection on socket {}", loop, i);
                                close_socket(i);
                                events_left--;
                                continue;
                                break;
                            case Socket::ReceiveMessagesExitCode::OK:
                                break;
                        }
                    }
                }
                if(socket_events & POLLOUT) { // Ready to write
                    switch(sockets[i]->send_messages()) {
                        case Socket::SendMessagesExitCode::ERROR:
                            logger.error("server_loop ({}): Send error", loop);
                            close_socket(i);
                            events_left--;
                            continue;
                        case Socket::SendMessagesExitCode::OK:
                            break;
                    }
                }
                if(socket_events & POLLPRI) { // Exceptional condition (very rare)
                    logger.error("server_loop ({}): Exceptional condition on idx {}", loop, i);
                    close_socket(i);
                    events_left--;
                    continue;
                }
                if(socket_events & POLLERR) { // Error / read end of pipe is closed
                    logger.error("server_loop ({}): Error on idx {}", loop, i); // errno?
                    close_socket(i);
                    events_left--;
                    continue;
                }
                if(socket_events & POLLHUP) { // Other end closed connection, some data may be left to read
                    logger.error("server_loop ({}): Got hang up on {}", loop, i);
                    close_socket(i);
                    events_left--;
                    continue;
                }
                if(socket_events & POLLNVAL) { // Invalid request, fd not open
                    logger.error("server_loop ({}): Socket {} at index {} is closed", loop, pollfds[i].fd, i);
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
    return StartExitCode::OK;
}

void Server::end_server() {
    close_sockets();
}   

Server::Server(
    std::string socket_path, 
    int max_connections,
    msg_listener_t message_listener,
    data_listener_t data_listener
): socket_path(socket_path), max_connections(max_connections), msg_listener(message_listener), data_listener(data_listener) {
    logger.info("Creating server on [{}] with a maximum of {} connections", socket_path, max_connections);
}

Server::~Server() {
    logger.info("Ending server");
    running = false;
    end_server();
}

Server::StartExitCode Server::start() {
    if (!initialized) {
        logger.error("start: Server not initialized");
        return StartExitCode::ERROR;
    }
    logger.info("Starting server");
    if (!running) running = true;
    return server_loop();
}

Server::Socket::Socket(
    int fd, 
    socket_msg_listener_t msg_listener, 
    socket_data_listener_t data_listener
): fd(fd), msg_listener(msg_listener), data_listener(data_listener) {
    sending_message.byte_offset = 0;
    sending_message.msg.buf = nullptr;
    sending_message.msg.size = 0;
    sending_message.in_progress = false;

    receiving_message.byte_offset = 0;

    receiving_data.byte_offset = 0;
    receiving_data.data.buf = nullptr;
    receiving_data.data.size = 0;
    receiving_data.msg.buf = nullptr;
    receiving_data.msg.size = 0;
    receiving_data.waiting = false;
}   

Server::Socket::~Socket() {
    logger.debug("Destroying socket");

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

Server::Socket::SendMessagesExitCode Server::Socket::send_messages() {
    logger.debug("send: Sending data to {}", fd);

    do {
        if (!sending_message.in_progress && !message_queue.empty()) {
            sending_message.msg = message_queue.front();
            message_queue.pop();
            sending_message.in_progress = true;
        }
        if (sending_message.in_progress) {
            size_t bytes_to_send = sending_message.msg.size - sending_message.byte_offset;
            ssize_t bytes_sent = send(fd, (char *) sending_message.msg.buf + sending_message.byte_offset, bytes_to_send, MSG_NOSIGNAL | MSG_DONTWAIT);
            logger.debug("send: OFFSET {}", sending_message.byte_offset);
            if(bytes_sent == 0 || (bytes_sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
                logger.debug("send: Can't send data right now, trying later");
                break;
            }
            else if(bytes_sent < 0) {
                logger.error("send: {}", strerror(errno));
                return SendMessagesExitCode::ERROR;
            } else {
                logger.trace("send: {} bytes sent", bytes_sent);
                sending_message.byte_offset += bytes_sent;
                if (sending_message.byte_offset == sending_message.msg.size) {
                    free(sending_message.msg.buf);
                    sending_message.in_progress = false;
                    sending_message.byte_offset = 0;
                }
            }
        } 
    } while (sending_message.in_progress || !message_queue.empty());

    return SendMessagesExitCode::OK;
}

Server::Socket::ReceiveMessagesExitCode Server::Socket::receive_messages() {
    logger.debug("receive: Receiving data on socket {}", fd);

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
            return ReceiveMessagesExitCode::OK;
        }
        else if (bytes_read < 0) {
            logger.error("receive (read): {}", strerror(errno));
            return ReceiveMessagesExitCode::ERROR;
        }
        else if(bytes_read == 0) {
            logger.debug("receive: 0 bytes received, got hang up on");
            return ReceiveMessagesExitCode::HANG_UP;
        }

        logger.trace("receive: {} bytes received", bytes_read);

        if (waiting_for_data) {
            receiving_data.byte_offset += bytes_read;
            return consume_data_buffer();
        } else {
            receiving_message.byte_offset += bytes_read;
            return consume_message_buffer();
        }
    }
}

Server::Socket::ReceiveMessagesExitCode Server::Socket::consume_data_buffer() {
    size_t offset = receiving_data.byte_offset;
    size_t expected_size = receiving_data.data.size;
    if (offset == expected_size) {
        packet_t packet;
        packet.msg = receiving_data.msg;
        packet.extra_data = { receiving_data.data.buf, receiving_data.data.size };
        data_listener(packet);

        receiving_data.waiting = false;
        receiving_data.byte_offset = 0;
    }
    return ReceiveMessagesExitCode::OK;
}

Server::Socket::ReceiveMessagesExitCode Server::Socket::consume_message_buffer() {
    size_t buffer_start = 0;

    bool more_data_needed = false;
    do {
        size_t usable_buffer_size = receiving_message.byte_offset - buffer_start;
        message_result_t res = msg_listener({receiving_message.buf + buffer_start, usable_buffer_size});
        switch(res.exit_code) {
            case MessageListenerExitCode::UNKNOWN_MESSAGE:
                return ReceiveMessagesExitCode::ERROR;
                break;
            case MessageListenerExitCode::INSUFFICIENT_DATA:
                if (usable_buffer_size == BUFFER_SIZE) {
                    logger.error("consume_message_buffer: Buffer filled but a command couldn't be parsed");
                    return ReceiveMessagesExitCode::ERROR;
                } else {
                    memmove(receiving_message.buf, receiving_message.buf + buffer_start, usable_buffer_size);
                    more_data_needed = true;
                }
                break;
            case MessageListenerExitCode::OK:
                if (res.expect_data > 0) {
                    receiving_data.waiting = true;
                    receiving_data.data = { malloc(res.expect_data), res.expect_data };
                    void *msg_buf_copy = malloc(res.bytes_consumed);
                    memcpy(msg_buf_copy, receiving_message.buf + buffer_start, res.bytes_consumed);
                    receiving_data.msg = { msg_buf_copy, res.bytes_consumed };
                }
                buffer_start += res.bytes_consumed;
                break;
        }
        // it is possible that we handle a variable_length_command, which means that whatever is left on the buffer needs to be handled as pure data
        const bool data_in_buffer = receiving_data.waiting && buffer_start < receiving_message.byte_offset;
        if (data_in_buffer) {
            size_t data_to_transfer = receiving_message.byte_offset - buffer_start;
            logger.debug("consume_message_buffer: Moving {} bytes of message buffer data to variable data buffer", data_to_transfer);
            void* data_buffer = (char*) receiving_data.data.buf + receiving_data.byte_offset;
            memcpy(data_buffer, receiving_message.buf + buffer_start, data_to_transfer);
            buffer_start = receiving_message.byte_offset;
            receiving_data.byte_offset += data_to_transfer;
        }
    } while (buffer_start < receiving_message.byte_offset && !more_data_needed);

    receiving_message.byte_offset -= buffer_start;

    return ReceiveMessagesExitCode::OK;
}

} //namespace cuda_mango
