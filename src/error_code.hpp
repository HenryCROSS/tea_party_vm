#ifndef ERROR_CODE_HPP
#define ERROR_CODE_HPP

#include <string>

namespace TPV {
enum class VM_Result {
  RUNTIME_ERR,
  OK,
};

struct Error {
  std::string msg;
};
}

#endif  // !ERROR_CODE_HPP