#ifndef SERVER_H
#define SERVER_H

#include <stdlib.h>
#include <queue>
#include <vector>
#include <functional>
#include <poll.h>

namespace cuda_mango {
    
class Server {

    public:
        enum MessageListenerExitCode {
            OK,
            INSUFFICIENT_DATA,
            UNKNOWN_MESSAGE,
        };

        typedef struct {
            void *buf;
            size_t size;
        } message_t;

        typedef struct {
            MessageListenerExitCode exit_code;
            size_t bytes_consumed;
            size_t expect_data;
        } message_result_t;

        typedef struct {
            message_t msg; // Message that asked for extra data to be read
            message_t extra_data; // Plain byte array
        } packet_t;

        /*
        * msg_listener_t DO NOT OWN the pointer in the received message.
        * If they need the data to exceed the scope of the function they should make their own copy.
        */
        typedef std::function<Server::message_result_t(int, message_t, Server&)> msg_listener_t;

        /* 
        * data_listener_t OWN the pointers in the received packet (buffers for msg & extra_data)
        * They are responsible for freeing the buffers after they are done with them.
        */
        typedef std::function<void(int, packet_t, Server&)> data_listener_t; 

        Server(const char *socket_path, int max_connections);
        ~Server();

        void send_on_socket(int id, message_t msg);
        void start();

        inline void stop() {
            running = false;
        }

        inline void set_message_listener(msg_listener_t msg_listener) {
            this->msg_listener = msg_listener;
        }

        inline void remove_message_listener() {
            this->msg_listener = nullptr;
        }
        
        inline void set_data_listener(data_listener_t data_listener) {
            this->data_listener = data_listener;
        }

        inline void remove_data_listener() {
            this->data_listener = nullptr;
        }

    private:
        static const int BUFFER_SIZE = 1024;

        typedef struct {
            bool in_progress;
            message_t msg;
            size_t byte_offset;
        } sending_message_t;

        typedef struct {
            char buf[BUFFER_SIZE];
            size_t byte_offset;
        } receiving_message_t;

        typedef struct {
            bool waiting;
            message_t msg;
            message_t data;
            size_t byte_offset;
        } receiving_data_t;

        msg_listener_t msg_listener;
        data_listener_t data_listener;
        const int max_connections;
        const int listen_idx = max_connections;
        bool running = false;

        std::vector<pollfd>                     pollfds = std::vector<pollfd>(max_connections + 1);                         // Sockets to poll. Listen socket + client connections.

        std::vector<std::queue<message_t>>      message_queues = std::vector<std::queue<message_t>>(max_connections);     // Messages queued to send
        std::vector<sending_message_t>          sending_messages = std::vector<sending_message_t>(max_connections);         // Message in process of being sent to the client

        std::vector<receiving_message_t>        receiving_messages = std::vector<receiving_message_t>(max_connections);     // Message in process of being received from the client
        std::vector<receiving_data_t>           receiving_data = std::vector<receiving_data_t>(max_connections);            // Unstructured data being received

        void server_loop();
        void initialize_server(const char* socket_path);
        void end_server();
        void check_for_writes();
        bool send_messages(int fd_idx);
        bool receive_messages(int fd_idx);
        bool consume_message_buffer(int fd_idx);
        void consume_data_buffer(int fd_idx);
        void send_ack(int fd_idx);
        bool accept_new_connection();
        void close_sockets();
        void close_socket(int fd_idx);
        void reset_socket_structs(int fd_idx);
        void handle_data_transfer_end(int fd_idx);
        void prepare_for_data_packet(int fd_idx, size_t size, message_t msg);
};

} // namespace cuda_mango

#endif