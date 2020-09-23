#ifndef COMMANDS_H
#define COMMANDS_H

#include <string.h>

namespace cuda_mango {
    enum command_type {
        HELLO,
        VARIABLE,
        MEM_ALLOC,
        MEM_RELEASE,
        MEM_WRITE,
        MEM_READ,
        LAUNCH_KERNEL,
        END,
        ACK,

        //responses (should we separate them?)
        ALLOC_MEM_SUCCESS,
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

    typedef struct {
        command_type cmd;
        size_t size;
    } memory_allocate_command_t;

    typedef struct {
        command_type cmd;
        int mem_id;
    } memory_release_command_t;

    typedef struct {
        command_type cmd;
        int mem_id;
        size_t size;
    } memory_write_command_t;

    typedef struct {
        command_type cmd;
        int mem_id;
        size_t size;
    } memory_read_command_t;

    typedef struct {
        command_type cmd;
        size_t size;
    } launch_kernel_command_t;

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

    inline void init_memory_allocate_command(memory_allocate_command_t &cmd, size_t size) {
        cmd.cmd = MEM_ALLOC;
        cmd.size = size;
    }

    inline void init_memory_write_command(memory_write_command_t &cmd, int mem_id, size_t size) {
        cmd.cmd = MEM_WRITE;
        cmd.mem_id = mem_id;
        cmd.size = size;
    }

    inline void init_memory_read_command(memory_read_command_t &cmd, int mem_id, size_t size) {
        cmd.cmd = MEM_READ;
        cmd.mem_id = mem_id;
        cmd.size = size;
    }

    inline void init_memory_release_command(memory_release_command_t &cmd, int mem_id) {
        cmd.cmd = MEM_RELEASE;
        cmd.mem_id = mem_id;
    }

    inline void init_launch_kernel_command(launch_kernel_command_t &cmd, int size) {
        cmd.cmd = LAUNCH_KERNEL;
        cmd.size = size;
    }

    typedef struct {
        command_type cmd;
        int mem_id;
    } memory_allocate_success_command_t;

    inline void init_memory_allocate_success_command(memory_allocate_success_command_t &cmd, int mem_id) {
        cmd.cmd = ALLOC_MEM_SUCCESS;
        cmd.mem_id = mem_id;
    }
}

#endif
