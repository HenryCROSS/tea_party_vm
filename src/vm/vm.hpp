#ifndef VM_H
#define VM_H

#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "../error_code.hpp"
#include "../common.hpp"
#include "../value.hpp"

namespace TPV {

struct FLAGS {
  bool eq_flag;
  bool is_true;
  bool is_zero;
}; 

// store each scope values
struct Frame{
  std::vector<Value> constants;
};

class VM {
 private:
 public:
  std::vector<TPV_INT> registers_i;
  std::vector<TPV_FLOAT> registers_d;
  std::vector<uint8_t> bytes;
  // std::unordered_map<std::string, T> table;
  std::vector<Value> stack;
  std::unordered_map<int32_t, TPV_ObjString> str_table;
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