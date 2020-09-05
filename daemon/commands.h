namespace cuda_mango {
    enum command_type {
        HELLO,
        END,
        ACK,
    };

    // All commands must be of fixed size (no pointers). 
    // If variable length buffers are needed we need to look into shared memory.

    typedef struct command_base {
        command_type cmd;
    } command_base_t;

    typedef struct hello_command {
        command_type cmd;
        char message[128];
    } hello_command_t;

    inline command_base_t create_end_command() {
        return {END};
    }

    inline hello_command_t create_hello_command(const char* message) {
        hello_command_t cmd;
        cmd.cmd = HELLO;
        strcpy(cmd.message, message);
        return cmd;
    }

    inline command_base_t create_ack_command() {
        return {ACK};
    }
};