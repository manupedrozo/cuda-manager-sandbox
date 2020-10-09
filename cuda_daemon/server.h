#ifndef SERVER_H
#define SERVER_H

#include <stdlib.h>
#include <queue>
#include <vector>
#include <functional>
#include <poll.h>
#include <memory>

namespace cuda_daemon {
    
class Server {

    public:
        enum class MessageListenerExitCode {
            OK,                     // Success
            OPERATION_ERROR,        // Error on the cuda api
            INSUFFICIENT_DATA,      // More data needs to be read in order to parse a message
            UNKNOWN_MESSAGE,        // Message received could not be recognized
        };

        enum class InitExitCode {
            OK,
            ERROR,
        };

        enum class StartExitCode {
            OK,
            ERROR,
        };

        typedef struct {
            void *buf;
            size_t size;
        } message_t;

        typedef struct {
            MessageListenerExitCode exit_code;  // Result of message listener operation
            size_t bytes_consumed;              // Amount of bytes consumed from the received buffer
            size_t expect_data;                 // Amount of bytes that are expected to arrive following the parsed message
        } message_result_t;

        typedef struct {
            message_t msg;          // Message that asked for extra data to be read
            message_t extra_data;   // Plain byte array
        } packet_t;

        /*
        * msg_listener_t DO NOT OWN the pointer in the received message.
        * If they need the data to exceed the scope of the function they should make their own copy.
        * 
        * \param int Id of the socket where the message was received.
        * \param message_t Message received.
        * \param Server& Reference to the server.
        */
        typedef std::function<Server::message_result_t(int, message_t, Server&)> msg_listener_t;

        /* 
        * data_listener_t OWN the pointers in the received packet (buffers for msg & extra_data)
        * They are responsible for freeing the buffers after they are done with them.
        * 
        * \param int Id of the socket where the packet was received.
        * \param packet_t Packet received, containing the mesage that asked for extra data and the extra data itself.
        * \param Server& Reference to the server.
        */
        typedef std::function<void(int, packet_t, Server&)> data_listener_t; 

        Server(std::string socket_path, int max_connections, msg_listener_t msg_listener, data_listener_t data_listener);
        ~Server();

        InitExitCode initialize();

        /*
        * \brief Send the given message to the socket with the given id.
        */
        void send_on_socket(int id, message_t msg);

        /*
        * \brief Start the server loop, listening for incoming connections.
        */
        StartExitCode start();

        /*
        * \brief Stop server loop.
        */
        inline void stop() {
            running = false;
        }

    private:
        class Socket {
            public:
                enum class SendMessagesExitCode {
                    OK,
                    ERROR,
                };

                enum class ReceiveMessagesExitCode {
                    OK,
                    HANG_UP,
                    ERROR,
                };

                typedef std::function<Server::message_result_t(message_t)> socket_msg_listener_t;
                typedef std::function<void(packet_t)> socket_data_listener_t;

                Socket(int fd, socket_msg_listener_t msg_listener, socket_data_listener_t data_listener);
                ~Socket();

                bool wants_to_write();
                SendMessagesExitCode send_messages();
                ReceiveMessagesExitCode receive_messages();
                
                inline void queue_message(message_t msg) {
                    message_queue.push(msg);
                }

            private:
                static const int BUFFER_SIZE = 1024; // Fixed size of the receiving message buffer. Should be at least equal to the maximum size of the expected structured messages.

                typedef struct {
                    bool in_progress;       // Whether a message is currently being sent.
                    message_t msg;          // Message to send.
                    size_t byte_offset;     // Position to read on the buffer.
                } sending_message_t;

                typedef struct {
                    char buf[BUFFER_SIZE];  // Buffer to receive incoming raw data from the socket. 
                                            // Fixed size is needed as the amount of data is unknown and the data we read has to be limited

                    size_t byte_offset;     // Position to write on the buffer.
                } receiving_message_t;

                typedef struct {
                    bool waiting;           // Whether there is data currently being received.
                    message_t msg;          // Message that asked for extra data to be retrieved.
                    message_t data;         // Extra data being received.
                    size_t byte_offset;     // Position to write on the data buffer.
                } receiving_data_t;

                const int fd;

                std::queue<message_t> message_queue = std::queue<message_t>();  // Messages queued to send
                sending_message_t sending_message;                              // Message in process of being sent to the client

                receiving_message_t receiving_message;                          // Message in process of being received from the client
                receiving_data_t receiving_data;                                // Unstructured data being received

                const socket_msg_listener_t msg_listener;
                const socket_data_listener_t data_listener;

                ReceiveMessagesExitCode consume_message_buffer();
                ReceiveMessagesExitCode consume_data_buffer();
        };

        enum class AcceptConnectionExitCode {
            OK,
            ERROR,
        };

        const int max_connections;
        const int listen_idx = max_connections;
        const msg_listener_t msg_listener;
        const data_listener_t data_listener;
        const std::string socket_path;

        bool running = false;
        bool initialized = false;

        std::vector<pollfd> pollfds = std::vector<pollfd>(max_connections + 1); // Sockets to poll. Listen socket + client connections.

        std::vector<std::unique_ptr<Server::Socket>> sockets = std::vector<std::unique_ptr<Server::Socket>>(max_connections);        

        StartExitCode server_loop();
        void end_server();
        void check_for_writes();
        AcceptConnectionExitCode accept_new_connection();
        void close_sockets();
        void close_socket(int fd_idx);
};

} // namespace cuda_daemon

#endif
