#include "cuda_argument_parser.h"
#include "kernel_arguments.h"
#include <vector>
#include <iostream>
#include <sstream>
#include <string.h>

namespace cuda_manager {

std::string parse_next(int *position, const char *arguments, bool *is_number) {
  int i = *position;

  std::string current_arg;
  char c = arguments[i];
  bool _is_number = true;
  while (c != ' ' && c != '\0') {
    // TODO Improve float parsing
    if (is_number && (!isdigit(c) && c != '.')) _is_number = false;

    current_arg.push_back(c);
    c = arguments[++i];
  }

  if (is_number != nullptr) *is_number = _is_number;
  *position = i;
  return current_arg;
}

bool parse_pointer(int *position, const char *arguments, void **result, bool full) {
  // full means we also parse "0x"
  int i = *position;
  char c = arguments[i];
  if (full) {
    char n = arguments[++i];
    if (c != '0' || n != 'x') {
      *position = i;
      return false;
    }
    ++i;
  }

  std::string hex = parse_next(&i, arguments, nullptr);

  // Not checking well formed hex string
  *result = (void *)std::stoul(hex, nullptr, 16);
  *position = i;
  return true;
}

bool parse_bool(int *position, const char *arguments, bool *result) {
  int i = *position;

  char c = arguments[i];
  char n = arguments[++i];

  *position = i;

  if (!isdigit(c) && (n != ' ' || n != '\0')) {
    return false;
  }

  *result = std::atoi(&c);
  return true;
}

bool parse_integer(int *position, const char *arguments, int *result) {
  int i = *position;

  std::string current_arg;
  char c = arguments[i];
  bool is_number = true;
  while (c != ' ' && c != '\0') {
    if (!isdigit(c)) {
      is_number = false;
      break;
    }
    current_arg.push_back(c);
    c = arguments[++i];
  }

  *position = i;

  if (current_arg.size() > 0 && is_number) {
    *result = std::stoi(current_arg);
    return true;
  }

  return false;
}

// @TODO receive size to not depend on correctly terminated strings
bool parse_arguments(const char *arguments, char **parsed_args, int *arg_count, int *kernel_id, char **function_name) {
  std::cout << "Parsing arguments: " << arguments << '\n';

  // Allocate memory for arguments
  size_t args_total_size = 1024 * 8;
  char *args = (char *) malloc(args_total_size);
  int args_count = 0;
  size_t args_size = 0;

  // Save base ptr
  *parsed_args = args;

  int i = 0; // arguments index
  bool parse_correct; 
  bool is_number;

  // Kernel mem_id
  parse_correct = parse_integer(&i, arguments, kernel_id);
  if (!parse_correct) {
    std::cerr << "Error: number expected (kernel mem_id)\n";
    return false;
  }

  std::cout << "Kernel id: " << kernel_id << '\n';

  // Kernel name
  std::string function_name_s = parse_next(&(++i), arguments, nullptr);
  size_t function_name_size = sizeof(char) * (function_name_s.size() + 1);
  *function_name = (char *) malloc(function_name_size);
  strncpy(*function_name, function_name_s.c_str(), function_name_s.size() + 1);

  std::cout << "Function name: " << function_name_s << '\n';

  // Rest of the arguments
  char c = arguments[i];
  while (c != '\0') {
    if (c == ' ') c = arguments[++i];

    std::string current_arg = parse_next(&i, arguments, &is_number);

    if (current_arg.size() > 0) { 

      // Buffer
      if (current_arg == "b") {
        std::cout << "Parsing buffer..." << '\n';

        current_arg.clear();

        // In/out
        bool is_in;
        parse_correct = parse_bool(&(++i), arguments, &is_in);
        if (!parse_correct) {
          std::cerr << "Error: 1|0 expected (buffer in|out)\n";
          return false;
        }

        // Id
        int id;
        parse_correct = parse_integer(&(++i), arguments, &id);
        if (!parse_correct) {
            std::cerr << "Error: number expected (buffer id)\n";
            return false;
        }

        size_t new_args_size = args_size + sizeof(BufferArg);
        if(args_total_size <= args_size) {
          std::cerr << "Ran out of memory for arguments\n";
          return false;
        }

        BufferArg *arg = (BufferArg *)args;
        *arg = {BUFFER, id, is_in};
        ++args_count;
        args_size = new_args_size;
        args += sizeof(BufferArg);

        std::cout << "Buffer: is_in = " << is_in << " id = " << id << '\n';
      }


      // Number
      else if (is_number) {
        float value = std::stof(current_arg); // TODO support multiple types

        size_t new_args_size = args_size + sizeof(ValueArg);
        if(args_total_size <= args_size) {
          std::cerr << "Ran out of memory for arguments\n";
          return false;
        }

        ValueArg *arg = (ValueArg *)args;
        *arg = {VALUE, value};
        ++args_count;
        args_size += new_args_size;
        args += sizeof(ValueArg);

        std::cout << "Number: " << current_arg << '\n';
      }

      // String
      else {
        std::cerr << "String: " << current_arg << " NOT SUPPORTED YET\n";
        return false;
      }
    }
    c = arguments[i];
  }

  *arg_count = args_count;
  
  return true;
}

// Receiving a void * vector here since we only use this for testing and makes it easy for the test.
std::string args_to_string(std::string function_name, int kernel_id, std::vector<void *> args) {
  std::stringstream ss;
  ss << kernel_id;
  ss << " " << function_name;

  for (void *arg: args) {
    Arg *base = (Arg *) arg; 
    switch (base->type) {
      case BUFFER: 
      {
        BufferArg *buffer_arg = (BufferArg *) arg; 
        ss << " b " << buffer_arg->is_in << ' ' << buffer_arg->id; //<< std::hex << buffer_arg->ptr << std::dec;
        break;
      }
      case VALUE:
      {
        ValueArg *value_arg = (ValueArg *) arg;
        ss << ' ' << value_arg->value;
        break;
      }
    }
  }
  return ss.str();
}

void print_args(std::vector<void *> args) {
  int i = 0;
  std::cout << "print_args not implemented\n"; 
  /*
  for (Arg *arg: args) {
    std::cout << ++i << ") is_buffer = " << arg->is_buffer << " value = " << arg->get_value_ptr() << " size = " << arg->size << " ptr = " << arg->get_value_ptr() << " is_in = " << arg->is_in << '\n';
  }
  */
}

}
