#ifndef SERVER_H
#define SERVER_H

#include <stdlib.h>
#include <queue>
#include <vector>
#include <poll.h>
#include "commands.h"

namespace cuda_mango {
    class Server {
        public:
            Server(const char *socket_path, int max_connections);
            ~Server();
            void start();

        private:
            static const int BUFFER_SIZE = 1024;

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

            std::vector<pollfd>                     pollfds = std::vector<pollfd>(max_connections + 1);                             // Sockets to poll. Listen socket + client connections.

            std::vector<std::queue<message_t *>>    message_queues = std::vector<std::queue<message_t *>>(max_connections);     // Messages queued to send
            std::vector<sending_message_t>          sending_messages = std::vector<sending_message_t>(max_connections);         // Message in process of being sent to the client

            std::vector<receiving_message_t>        receiving_messages = std::vector<receiving_message_t>(max_connections);     // Message in process of being received from the client
            std::vector<receiving_data_t>           receiving_data = std::vector<receiving_data_t>(max_connections);            // Unstructured data being received

            const char *socket_path;
            const int max_connections;
            const int listen_idx = max_connections;
            bool running = true;

            void server_loop();
            void initialize_server();
            void end_server();
            void check_for_writes();
            bool send_on_socket(int fd_idx);
            bool receive_on_socket(int fd_idx);
            bool consume_message_buffer(int fd_idx);
            void consume_data_buffer(int fd_idx);
            void send_ack(int fd_idx);
            bool accept_new_connection();
            void close_sockets();
            void close_socket(int fd_idx);
            void reset_socket_structs(int fd_idx);
            void handle_data_transfer_end(int fd_idx);
            int handle_command(int fd_idx, const char *buffer, size_t size);
            void handle_variable_length_command(int fd_idx, const variable_length_command_t *cmd);
            void handle_end_command(const command_base_t *cmd);
            void handle_hello_command(const hello_command_t *cmd);
    };
}

#endif