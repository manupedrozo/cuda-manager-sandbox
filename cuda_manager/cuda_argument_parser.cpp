#include "cuda_argument_parser.h"
#include "cuda_common.h"
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
bool parse_arguments(const char *arguments, std::vector<Arg *> &args, int *kernel_mem_id, char **kernel_name) {
  std::cout << "Parsing arguments: " << arguments << '\n';

  // arg array index
  int i = 0;
  bool parse_correct; 

  // Kernel mem_id
  parse_correct = parse_integer(&i, arguments, kernel_mem_id);
  if (!parse_correct) {
    std::cerr << "Error: number expected (kernel mem_id)\n";
    return false;
  }

  // Kernel name
  std::string kernel_name_s = parse_next(&(++i), arguments, nullptr);
  size_t kernel_name_size = sizeof(char) * (kernel_name_s.size() + 1);
  *kernel_name = (char *) malloc(kernel_name_size);
  strncpy(*kernel_name, kernel_name_s.c_str(), kernel_name_s.size() + 1);

  // Rest of the arguments
  bool is_number;
  char c = arguments[i];
  while (c != '\0') {
    if (c == ' ') c = arguments[++i];

    std::string current_arg = parse_next(&i, arguments, &is_number);

    if (current_arg.size() > 0) { 

      // Buffer
      if (current_arg == "b") {
        current_arg.clear();

        // In/out
        bool is_in;
        parse_correct = parse_bool(&(++i), arguments, &is_in);
        if (!parse_correct) {
          std::cerr << "Error: 1|0 expected (buffer in|out)\n";
          return false;
        }

        // Size
        int size;
        parse_correct = parse_integer(&(++i), arguments, &size);
        if (!parse_correct) {
          std::cerr << "Error: number expected (buffer size)\n";
          return false;
        }

        /* Not a ptr any more
        // Ptr
        void *ptr;
        parse_correct = parse_pointer(&(++i), arguments, &ptr, true);

        Arg *arg = new BufferArg(ptr, (size_t)size, is_in);
        args.push_back(arg);

        std::cout << "Buffer: is_in = " << is_in << " size = " << size << " ptr = " << ptr << '\n';
        */

        // Id
        int id;
        parse_correct = parse_integer(&(++i), arguments, &id);
        if (!parse_correct) {
            std::cerr << "Error: number expected (buffer id)\n";
            return false;
        }

        Arg *arg = new BufferArg(id, (size_t)size, is_in);
        args.push_back(arg);

        std::cout << "Buffer: is_in = " << is_in << " size = " << size << " id = " << id << '\n';
      }


      // Number
      else if (is_number) {
        float value = std::stof(current_arg); // TODO support multiple types
        Arg *arg = new ValueArg<float>(value, sizeof(float), true);
        args.push_back(arg);
        std::cout << "Number: " << current_arg << '\n';
      }

      // String
      else {
        std::cout << "String: " << current_arg << " NOT SUPPORTED YET\n";
        return false;
      }
    }
    c = arguments[i];
  }
  
  return true;
}

std::string args_to_string(std::string kernel_name, int kernel_mem_id, std::vector<Arg *> args) {
	std::stringstream ss;
  ss << kernel_mem_id;
  ss << " " << kernel_name;

  for (Arg *arg: args) {
    if (arg->is_buffer) {
      ss << " b " << arg->is_in << ' ' << arg->size << ' ' << arg->get_id(); //<< std::hex << arg->get_value_ptr() << std::dec;
    } else {
      // Using only float since this is for testing purposes
      ss << ' ' << *(float *)arg->get_value_ptr();
    }
  }
  return ss.str();
}

void print_args(std::vector<Arg *> args) {
  int i = 0;
  for (Arg *arg: args) {
    std::cout << ++i << ") is_buffer = " << arg->is_buffer << " value = " << arg->get_value_ptr() << " size = " << arg->size << " ptr = " << arg->get_value_ptr() << " is_in = " << arg->is_in << '\n';
  }
}

}
