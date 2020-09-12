#include <string.h>

namespace cuda_mango {
    enum command_type {
        HELLO,
        VARIABLE,
        END,
        ACK,
    };

    // All commands must be of fixed size (no pointers). 

    typedef struct {
        command_type cmd;
    } command_base_t;

    typedef struct {
        command_type cmd;
        char message[128];
    } hello_command_t;

    // For sending variable length data, specify the size to read and the server will accumulate whatever is sent next upto the size specified
    typedef struct {
        command_type cmd;
        size_t size;
    } variable_length_command_t;

    inline void init_end_command(command_base_t &cmd) {
        cmd.cmd = END;
    }

    inline void init_hello_command(hello_command_t &cmd, const char* message) {
        cmd.cmd = HELLO;
        strcpy(cmd.message, message);
    }

    inline void init_ack_command(command_base_t &cmd) {
        cmd.cmd = ACK;
    }

    inline void init_variable_length_command(variable_length_command_t &cmd, size_t size) {
        cmd.cmd = VARIABLE;
        cmd.size = size;
    }
};