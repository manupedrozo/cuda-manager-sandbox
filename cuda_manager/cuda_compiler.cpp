#include "cuda_compiler.h"
#include <nvrtc.h>
#include <fstream>
#include <iostream>

namespace cuda_manager {

char *CudaCompiler::compile_to_ptx(const char *source_path) {
  std::cout << "Compiling cuda kernel file [" << source_path << "]...\n";
  // Read kernel file
  std::ifstream input_file(source_path, std::ifstream::in | std::ifstream::ate);

  if (!input_file.is_open()) {
    std::cerr << "Unable to open file\n";
    exit(1);
  }

  size_t input_size = (size_t)input_file.tellg();
  char *kernel_string = new char[input_size + 1];

  input_file.seekg(0, std::ifstream::beg);
  input_file.read(kernel_string, input_size);
  input_file.close();

  kernel_string[input_size] = '\x0';

  // Create nvrtc program for compilation
  nvrtcProgram prog;
  // TODO Check if program name (3rd param) is needed, "default_program" is used when null.
  NVRTC_SAFE_CALL(
      nvrtcCreateProgram(&prog, kernel_string, NULL, 0, NULL, NULL) 
  );

  delete[] kernel_string;

  // Compilation options
  const char *opts[] = {"--fmad=false"};
  // Compile the program
  nvrtcResult compile_result = nvrtcCompileProgram(prog, 1, opts);

  // Get compilation log
  size_t log_size;
  NVRTC_SAFE_CALL(
    nvrtcGetProgramLogSize(prog, &log_size)
  );
  char *log = new char[log_size];
  NVRTC_SAFE_CALL(nvrtcGetProgramLog(prog, log));
  std::cout << log;
  delete[] log;

  if (compile_result != NVRTC_SUCCESS) {
    exit(1);
  }
  std::cout << "Compilation successful\n";

  // Get PTX from the program
  size_t ptx_size;
  NVRTC_SAFE_CALL(nvrtcGetPTXSize(prog, &ptx_size));
  char *ptx = new char[ptx_size];
  NVRTC_SAFE_CALL(nvrtcGetPTX(prog, ptx));

  // Destroy the program
  NVRTC_SAFE_CALL(nvrtcDestroyProgram(&prog));

  return ptx;
}

void CudaCompiler::save_ptx_to_file(const char *ptx, const char *output_path) {
  std::ofstream output_file(output_path);
  
  if (!output_file) {
    std::cerr << "Unable to create file\n";
    exit(1);
  }

  output_file << ptx;
  output_file.close();
}

char *CudaCompiler::read_ptx_from_file(const char *ptx_path) {
  std::ifstream input_file(ptx_path, std::ifstream::in | std::ifstream::ate);

  if (!input_file.is_open()) {
    std::cerr << "Unable to open file\n";
    exit(1);
  }

  size_t input_size = (size_t)input_file.tellg();
  char *ptx = new char[input_size];

  input_file.seekg(0, std::ifstream::beg);
  input_file.read(ptx, input_size);
  input_file.close();

  return ptx;
}

}
