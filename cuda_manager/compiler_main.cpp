#include <iostream>
#include "cuda_compiler.h"

int main(int argc, char **argv) {
    if(argc < 2) {
        printf("[Cuda compiler] Error, bad arguments\n");
        printf("Arguments: <kernel_path> <(opt)output_path>\n");
        exit(1);
    }

    std::string kernel_path = argv[1];
    std::string output_path;
    if(argc > 2) {
        output_path = argv[2];
    } else {
        size_t from = kernel_path.find_last_of("/") + 1;
        size_t to = kernel_path.find_last_of(".") - from;
        output_path = kernel_path.substr(from, to);
    }
    
    std::cout << "[Cuda compiler] Compiling: " << kernel_path << " to " << output_path << std::endl;

    // Initialize cuda compiler
    cuda_manager::CudaCompiler cuda_compiler;

    // Compile the file to a PTX
    char *ptx;
    cuda_compiler.compile_to_ptx(kernel_path.c_str(), &ptx);

    // Save the PTX file
    cuda_compiler.save_ptx_to_file(ptx, output_path.c_str());

    std::cout << "[Cuda compiler] Compilation complete" << std::endl;

    delete[] ptx;
}
