#ifndef VM_H
#define VM_H

#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "../error_code.hpp"
#include "../value.hpp"

namespace TPV {
const int32_t MAX_FRAME = 2048;
const int32_t MAX_STACKS = 2048;
const int32_t MAX_REGISTERS = 256;

struct FLAGS {
  bool eq_flag = false;
  bool is_true = false;
  bool is_zero = false;
  bool is_panic = false;
};

// store each scope values
struct Frame {
  std::vector<Value> registers;
  std::vector<Value> stack;
  uint32_t pc = 0;
  TPV_Function* function = nullptr;
};

class VM {
 private:
 public:
//   std::vector<uint8_t> bytes;
  std::vector<Frame> frames;
  std::vector<TPV_Function> functions;

  std::unordered_map<size_t, TPV_INT> int32_table;
  std::unordered_map<size_t, TPV_FLOAT> float32_table;
  std::unordered_map<size_t, std::shared_ptr<TPV_ObjString>> str_table;

  // to store all values
  std::vector<std::shared_ptr<TPV_Obj>> heap;
  std::vector<Error> errors;
  FLAGS flags;
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
  void print_str_table();
};

}  // namespace TPV

#endif  // !VM_H