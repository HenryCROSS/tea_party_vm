#ifndef VM_H
#define VM_H

#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "../common.hpp"
#include "../error_code.hpp"
#include "../value.hpp"

namespace TPV {
const int32_t MAX_FRAME = 2048;

struct FLAGS {
  bool eq_flag = false;
  bool is_true = false;
  bool is_zero = false;
  bool is_panic = false;
};

// store each scope values
struct Frame {
  std::vector<Value> stack;
};

class VM {
 private:
 public:
  std::vector<uint8_t> bytes;
  std::vector<Frame> frames;
  std::unordered_map<int32_t, std::shared_ptr<TPV_ObjString>> str_table;
  std::vector<Error> errors;
  FLAGS flags;
  uint32_t pc = 0;
  uint32_t code_begin = 0;
  uint32_t code_size = 0;
  uint32_t data_begin = 0;
  uint32_t data_size = 0;
  bool is_running;

  uint8_t next_8_bit();
  std::vector<uint8_t> next_16_bit();
  std::vector<uint8_t> next_32_bit();
  std::vector<uint8_t> next_string();

 public:
  VM();
  ~VM() = default;

  bool load_bytes(const std::vector<uint8_t> bytes);
  bool run_bytecode_file(std::string_view path);
  bool run_src_file(std::string_view path);
  VM_Result eval_all();
  VM_Result eval_one();

  // For Debugging
  void print_regs();
};

}  // namespace TPV

#endif  // !VM_H