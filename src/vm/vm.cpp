#include "vm.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <print>
#include "../error_code.hpp"
#include "../instructions.hpp"
#include "../parser/parser.hpp"
#include "../scanner/scanner.hpp"
#include "../utils.hpp"
#include "common.hpp"
#include "value.hpp"


using std::vector;

#define INT_TABLE 0
#define FLOAT_TABLE 1
#define STR_TABLE 2
#define TABLE_TABLE 3

namespace TPV {

VM::VM()
    : int32_table(),
      float32_table(),
      str_table(),
      frames(MAX_FRAME),
      flags(),
      is_running(true) {
  auto& current_frame = this->frames.back();
  current_frame.function = new TPV_Function();
  current_frame.function->bytes = std::vector<uint8_t>();
  current_frame.function->arity = 0;
  current_frame.function->name = "";
  current_frame.pc = 0;
  current_frame.registers = std::vector<Value>(MAX_REGISTERS);
  current_frame.stack = std::vector<Value>(MAX_STACKS);
  current_frame.stack.reserve(MAX_REGISTERS);
  current_frame.registers.reserve(MAX_STACKS);
  this->frames.push_back(current_frame);
  this->frames.back().function = current_frame.function;
  this->frames.back().pc = 0;
}

uint8_t VM::next_8_bit() {
  auto& current_frame = this->frames.back();
  if (current_frame.pc >= current_frame.function->bytes.size()) {
    throw std::out_of_range("No more bytes available");
  }
  auto byte = current_frame.function->bytes.at(current_frame.pc);
  current_frame.pc += 1;

  return byte;
}

std::vector<uint8_t> VM::next_16_bit() {
  auto bytes = {this->next_8_bit(), this->next_8_bit()};

  return bytes;
}

std::vector<uint8_t> VM::next_32_bit() {
  auto bytes = {this->next_8_bit(), this->next_8_bit(), this->next_8_bit(),
                this->next_8_bit()};

  return bytes;
}

std::vector<uint8_t> VM::next_string() {
  std::vector<uint8_t> result;
  while (true) {
    uint8_t byte = this->next_8_bit();
    if (byte == 0) {
      break;
    }
    result.push_back(byte);
  }
  return result;
}

bool VM::load_bytes(const vector<uint8_t> instructions) {
  auto& current_frame = this->frames.back();
  auto it = instructions.cbegin();
  auto end = instructions.cend();
  const auto END_FUNC = static_cast<uint8_t>(Opcode::FUNCEND);

  while (it != end) {
    auto opcode = static_cast<Opcode>(*it);
    switch (opcode) {
      case Opcode::FUNCDEF:
      case Opcode::FUNCDEF_G: {
        auto func = new TPV_Function();

        it += 1;  // skip FUNCDEF
        it += 1;  // skip r1
        it += 4;  // skip imm1

        while (it != end && *it != END_FUNC) {
          func->bytes.push_back(*it);
          it += 1;
        }

        // if not end, then error
        if (it == end && *it != END_FUNC) {
          throw std::runtime_error("FUNCDEF without FUNCEND");
        }

        this->functions.push_back(*func);
        break;
      }
      default:
        current_frame.function->bytes.push_back(*it);
        break;
    }

    // next byte
    it += 1;
  }

  return true;
}

bool VM::run_bytecode_file(std::string_view path) {
  constexpr auto read_size = std::size_t(4096);
  auto stream = std::ifstream(path.data(), std::ios::binary);
  stream.exceptions(std::ios_base::badbit);

  if (not stream) {
    throw std::ios_base::failure("file does not exist");
    // return false;
  }

  auto out = std::string();
  auto buf = std::string(read_size, '\0');
  while (stream.read(&buf[0], read_size)) {
    out.append(buf, 0, stream.gcount());
  }
  out.append(buf, 0, stream.gcount());

  load_bytes(std::vector<uint8_t>(out.cbegin(), out.cend()));

  eval_all();

  return true;
}

bool VM::run_src_file(std::string_view path) {
  constexpr auto read_size = std::size_t(4096);
  auto stream = std::ifstream(path.data());
  stream.exceptions(std::ios_base::badbit);

  if (not stream) {
    // throw std::ios_base::failure("file does not exist");
    return false;
  }

  auto out = std::string();
  auto buf = std::string(read_size, '\0');
  while (stream.read(&buf[0], read_size)) {
    out.append(buf, 0, stream.gcount());
  }
  out.append(buf, 0, stream.gcount());

  auto tokens_opt = scan_all(out);

  if (tokens_opt) {
    TPV::Parser parser{};

    TPV::Test_Fn::print_tokens(*tokens_opt);
    parser.load_tokens(*tokens_opt);
    auto result = parser.parse();
    parser.print_bytecodes();
    if (result.err_msg.empty()) {
      load_bytes(result.bytecodes);
      eval_all();
    } else {
      for (auto&& i : result.err_msg) {
        std::cout << i << "\n";
      }
    }
  } else {
    std::print(stderr, "Failed to scan file\n");
    return false;
  }

  return false;
}

VM_Result VM::eval_all() {
  while (is_running &&
         this->frames.back().function->bytes.size() > this->frames.back().pc) {
    auto byte = this->next_8_bit();

    // decoding
    auto opcode = static_cast<Opcode>(byte);

    switch (opcode) {
      case Opcode::SETI: {
        auto rd = this->next_8_bit();
        const auto val = this->next_32_bit();

        this->frames.back().registers.at(rd) = {.type = ValueType::TPV_INT,
                                                .is_const = false,
                                                .value = bytes_to_int32(val)};

        break;
      }
      case Opcode::SETF: {
        auto rd = this->next_8_bit();
        const auto val = this->next_32_bit();

        this->frames.back().registers.at(rd) = {.type = ValueType::TPV_FLOAT,
                                                .is_const = false,
                                                .value = bytes_to_float32(val)};

        break;
      }
      case Opcode::SETS: {
        auto rd = this->next_8_bit();
        const auto str = bytes_to_string(this->next_string());

        // check if str exist in the table, and find the idx
        auto idx = hash_string(str);
        auto it = this->str_table.find(idx);
        while (it != this->str_table.cend() && it->second->value != str) {
          idx += 1;
          it = this->str_table.find(idx);
        }

        // add to str_table if not exist
        if (it == this->str_table.cend()) {
          auto ptr = std::make_shared<TPV_ObjString>(
              (TPV_ObjString){.hash = (size_t)idx, .value = str});
          this->str_table[idx] = ptr;
        }

        this->frames.back().registers.at(rd) =
            from_obj_value(this->str_table.at(idx));

        break;
      }
      case Opcode::SETNIL: {
        auto rd = this->next_8_bit();

        this->frames.back().registers.at(rd) = {.type = ValueType::TPV_UNIT,
                                                .is_const = false,
                                                .value = (TPV_Unit){}};

        break;
      }
      case Opcode::STORE: {
        // return ref idx
        auto& rd = this->frames.back().registers.at(this->next_8_bit());
        // store item
        auto r1 = this->frames.back().registers.at(this->next_8_bit());
        // type of table
        auto imm = bytes_to_int32(this->next_32_bit());

        switch (imm) {
          case INT_TABLE: {
            rd = from_raw_value((int32_t)int32_table.size());
            this->int32_table[int32_table.size()] = get_int32(r1);
            break;
          }
          case FLOAT_TABLE: {
            rd = from_raw_value((int32_t)float32_table.size());
            this->float32_table[float32_table.size()] = get_float32(r1);
            break;
          }
          case STR_TABLE: {
            auto str = get_str(r1);
            auto idx = hash_string(str.value);
            auto it = this->str_table.find(idx);
            while (it != this->str_table.cend()) {
              idx += 1;
              it = this->str_table.find(idx);
            }

            str.hash = idx;
            auto ptr = std::make_shared<TPV_ObjString>(str);
            this->str_table[idx] = ptr;
            rd = from_raw_value(idx);

            break;
          }
          default: {
            this->errors.push_back(
                {.msg = std::format(
                     "Type Error: STORE operation on non-exist table type {}",
                     imm)});
            break;
          }
        }

        break;
      }
      case Opcode::LOAD: {
        // return ref
        auto& rd = this->frames.back().registers.at(this->next_8_bit());
        // access idx
        auto r1 = this->frames.back().registers.at(this->next_8_bit());
        // type of table
        auto imm = bytes_to_int32(this->next_32_bit());
        auto idx = get_int32(r1);

        switch (imm) {
          case INT_TABLE: {
            rd = from_raw_value(this->int32_table.at(idx));
            break;
          }
          case FLOAT_TABLE: {
            rd = from_raw_value(this->float32_table.at(idx));
            break;
          }
          case STR_TABLE: {
            rd = from_obj_value(this->str_table.at(idx));
            break;
          }
          default: {
            this->errors.push_back(
                {.msg = std::format(
                     "Type Error: LOAD operation on non-exist table type {}",
                     imm)});
            break;
          }
        }

        break;
      }
      case Opcode::ADD: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);
        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(get_int32(r1) + get_int32(r2));
          } else if (r1.type == ValueType::TPV_FLOAT) {
            ref = from_raw_value(get_float32(r1) + get_float32(r2));
          } else {
            this->errors.push_back(
                {.msg = std::format("Type Error: ADD operation on {} and {}",
                                    get_value_type_name(r1.type),
                                    get_value_type_name(r2.type))});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::SUB: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(get_int32(r1) - get_int32(r2));
          } else if (r1.type == ValueType::TPV_FLOAT) {
            ref = from_raw_value(get_float32(r1) - get_float32(r2));
          } else {
            this->errors.push_back(
                {.msg = std::format("Type Error: SUB operation on {} and {}",
                                    get_value_type_name(r1.type),
                                    get_value_type_name(r2.type))});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::MUL: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(get_int32(r1) * get_int32(r2));
          } else if (r1.type == ValueType::TPV_FLOAT) {
            ref = from_raw_value(get_float32(r1) * get_float32(r2));
          } else {
            this->errors.push_back(
                {.msg = std::format("Type Error: MUL operation on {} and {}",
                                    get_value_type_name(r1.type),
                                    get_value_type_name(r2.type))});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::DIV: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            if (std::get<TPV_INT>(r2.value) == 0) {
              this->errors.push_back({});
            } else {
              ref = from_raw_value(get_int32(r1) / get_int32(r2));
            }
          } else if (r1.type == ValueType::TPV_FLOAT) {
            if (std::get<TPV_FLOAT>(r2.value) == 0.0) {
              this->errors.push_back({});
            } else {
              ref = from_raw_value(get_float32(r1) / get_float32(r2));
            }
          } else {
            this->errors.push_back(
                {.msg = std::format("Type Error: DIV operation on {} and {}",
                                    get_value_type_name(r1.type),
                                    get_value_type_name(r2.type))});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::CVT_I_D: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == ValueType::TPV_INT) {
          ref = from_raw_value(static_cast<TPV_FLOAT>(get_int32(r1)));
        } else if (r1.type == ValueType::TPV_FLOAT) {
          ref = r1;
        } else {
          this->errors.push_back(
              {.msg = std::format("Type Error: CVT_I_D operation on {}",
                                  get_value_type_name(r1.type))});
        }

        break;
      }
      case Opcode::CVT_D_I: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == ValueType::TPV_INT) {
          ref = r1;
        } else if (r1.type == ValueType::TPV_FLOAT) {
          this->frames.back().registers.at(rd) =
              from_raw_value(static_cast<TPV_INT>(get_float32(r1)));
        } else {
          this->errors.push_back(
              {.msg = std::format("Type Error: CVT_D_I operation on {}",
                                  get_value_type_name(r1.type))});
        }

        break;
      }
      case Opcode::NEGATE: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == ValueType::TPV_INT) {
          this->frames.back().registers.at(rd) =
              from_raw_value(static_cast<TPV_FLOAT>(-get_int32(r1)));
        } else if (r1.type == ValueType::TPV_FLOAT) {
          this->frames.back().registers.at(rd) =
              from_raw_value(static_cast<TPV_FLOAT>(-get_float32(r1)));
        } else {
          this->errors.push_back(
              {.msg = std::format("Type Error: NEGATE operation on {}",
                                  get_value_type_name(r1.type))});
        }

        break;
      }
      case Opcode::HLT: {
        this->is_running = false;
        break;
      }
      case Opcode::JMP: {
        const auto new_pc = bytes_to_int32(this->next_32_bit());
        this->frames.back().pc = new_pc;
        break;
      }
      case Opcode::JMP_IF: {
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto new_pc = bytes_to_int32(this->next_32_bit());

        if (r1.type == ValueType::TPV_INT) {
          auto val = get_int32(r1);
          if (val)
            this->frames.back().pc = new_pc;
        } else if (r1.type == ValueType::TPV_FLOAT) {
          auto val = get_float32(r1);
          if (val)
            this->frames.back().pc = new_pc;
        } else {
          this->errors.push_back({});
        }
        break;
      }
      case Opcode::EQ: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(get_int32(r1) == get_int32(r2));
          } else if (r1.type == ValueType::TPV_FLOAT) {
            ref = from_raw_value(get_float32(r1) == get_float32(r2));
          } else {
            this->errors.push_back(
                {.msg = std::format("Type Error: EQ operation on {} and {}",
                                    get_value_type_name(r1.type),
                                    get_value_type_name(r2.type))});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::NEQ: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(get_int32(r1) != get_int32(r2));
          } else if (r1.type == ValueType::TPV_FLOAT) {
            ref = from_raw_value(get_float32(r1) != get_float32(r2));
          } else {
            this->errors.push_back(
                {.msg = std::format("Type Error: NEQ operation on {} and {}",
                                    get_value_type_name(r1.type),
                                    get_value_type_name(r2.type))});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::GT: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(get_int32(r1) > get_int32(r2));
          } else if (r1.type == ValueType::TPV_FLOAT) {
            ref = from_raw_value(get_float32(r1) > get_float32(r2));
          } else {
            this->errors.push_back(
                {.msg = std::format("Type Error: GT operation on {} and {}",
                                    get_value_type_name(r1.type),
                                    get_value_type_name(r2.type))});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::GTE: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(get_int32(r1) >= get_int32(r2));
          } else if (r1.type == ValueType::TPV_FLOAT) {
            ref = from_raw_value(get_float32(r1) >= get_float32(r2));
          } else {
            this->errors.push_back(
                {.msg = std::format("Type Error: GTE operation on {} and {}",
                                    get_value_type_name(r1.type),
                                    get_value_type_name(r2.type))});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::LT: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(get_int32(r1) < get_int32(r2));
          } else if (r1.type == ValueType::TPV_FLOAT) {
            ref = from_raw_value(get_float32(r1) < get_float32(r2));
          } else {
            this->errors.push_back(
                {.msg = std::format("Type Error: LT operation on {} and {}",
                                    get_value_type_name(r1.type),
                                    get_value_type_name(r2.type))});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::LTE: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(get_int32(r1) <= get_int32(r2));
          } else if (r1.type == ValueType::TPV_FLOAT) {
            ref = from_raw_value(get_float32(r1) <= get_float32(r2));
          } else {
            this->errors.push_back(
                {.msg = std::format("Type Error: LTE operation on {} and {}",
                                    get_value_type_name(r1.type),
                                    get_value_type_name(r2.type))});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::BITAND: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(get_int32(r1) & get_int32(r2));
          } else {
            this->errors.push_back(
                {.msg = std::format("Type Error: BITAND operation on {} and {}",
                                    get_value_type_name(r1.type),
                                    get_value_type_name(r2.type))});
          }
        } else {
          this->errors.push_back(
              {.msg = std::format("Type Error: BITAND operation on {} and {}",
                                  get_value_type_name(r1.type),
                                  get_value_type_name(r2.type))});
        }

        break;
      }
      case Opcode::BITOR: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(get_int32(r1) | get_int32(r2));
          } else {
            this->errors.push_back(
                {.msg = std::format("Type Error: BITOR operation on {} and {}",
                                    get_value_type_name(r1.type),
                                    get_value_type_name(r2.type))});
          }
        } else {
          this->errors.push_back(
              {.msg = std::format("Type Error: BITOR operation on {} and {}",
                                  get_value_type_name(r1.type),
                                  get_value_type_name(r2.type))});
        }

        break;
      }
      case Opcode::BITXOR: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(get_int32(r1) ^ get_int32(r2));
          } else {
            this->errors.push_back(
                {.msg = std::format("Type Error: BITXOR operation on {} and {}",
                                    get_value_type_name(r1.type),
                                    get_value_type_name(r2.type))});
          }
        } else {
          this->errors.push_back(
              {.msg = std::format("Type Error: BITXOR operation on {} and {}",
                                  get_value_type_name(r1.type),
                                  get_value_type_name(r2.type))});
        }

        break;
      }
      case Opcode::BITNOT: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == ValueType::TPV_INT) {
          ref = from_raw_value(~get_int32(r1));
        } else {
          this->errors.push_back(
              {.msg = std::format("Type Error: BITNOT operation on {}",
                                  get_value_type_name(r1.type))});
        }

        break;
      }
      case Opcode::BITSHL: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto imm = bytes_to_int32(this->next_32_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == ValueType::TPV_INT) {
          ref = from_raw_value(get_int32(r1) << imm);
        } else {
          this->errors.push_back(
              {.msg = std::format("Type Error: BITSHL operation on {}",
                                  get_value_type_name(r1.type))});
        }

        break;
      }
      case Opcode::BITSHRL: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto imm = bytes_to_int32(this->next_32_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == ValueType::TPV_INT) {
          auto val = std::get<TPV_INT>(r1.value);

          if (val < 0) {
            auto result =
                static_cast<std::make_unsigned_t<TPV_INT>>(val) >> imm;
            ref = from_raw_value(static_cast<TPV_INT>(result));
          } else {
            ref = from_raw_value(val >> imm);
          }
        } else {
          this->errors.push_back(
              {.msg = std::format("Type Error: BITSHRL operation on {}",
                                  get_value_type_name(r1.type))});
        }

        break;
      }
      case Opcode::BITSHRA: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto imm = bytes_to_int32(this->next_32_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == ValueType::TPV_INT) {
          ref = from_raw_value(get_int32(r1) >> imm);
        } else {
          this->errors.push_back(
              {.msg = std::format("Type Error: BITSHRA operation on {}",
                                  get_value_type_name(r1.type))});
        }

        break;
      }
      case Opcode::PUSH: {
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        this->frames.back().stack.push_back(r1);
        break;
      }
      case Opcode::POP: {
        this->frames.back().registers.at(this->next_8_bit()) =
            this->frames.back().stack.back();
        this->frames.back().stack.pop_back();
        break;
      }
      case Opcode::VMCALL: {
        const auto r1_idx = this->next_8_bit();
        const auto r2_idx = this->next_8_bit();
        const auto imm = bytes_to_int32(this->next_32_bit());

        switch (imm) {
          case 0: {
            const auto& r1 = this->frames.back().registers.at(r1_idx);
            if (r1.type == ValueType::TPV_INT) {
              const auto num = get_int32(r1);
              std::printf("%d", num);
            } else if (r1.type == ValueType::TPV_FLOAT) {
              const auto num = get_float32(r1);
              std::printf("%f", num);
            } else if (r1.type == ValueType::TPV_OBJ) {
              const auto obj = get_str_ptr(r1);
              std::printf("%s", obj->value.c_str());
            } else {
              this->errors.push_back({"Nothing in the register"});
            }

            const auto& r2 = this->frames.back().registers.at(r2_idx);
            if (r2.type == ValueType::TPV_INT) {
              const auto flag = get_int32(r2);
              if (flag == 1) {
                std::printf("\n");
              }
            } else {
              this->errors.push_back({});
            }
            break;
          }
          case 1: {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
              char* endptr;
              long int input = strtol(buffer, &endptr, 10);
              if (*endptr == '\n' || *endptr == '\0') {
                this->frames.back().registers.at(r1_idx) =
                    from_raw_value(static_cast<TPV_INT>(input));
              } else {
                this->errors.push_back({"Invalid integer input"});
              }
            } else {
              this->errors.push_back({"Failed to read input"});
            }
            break;
          }
          case 2: {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
              char* endptr;
              float input = strtof(buffer, &endptr);
              if (*endptr == '\n' || *endptr == '\0') {
                this->frames.back().registers.at(r1_idx) =
                    from_raw_value(static_cast<TPV_FLOAT>(input));
              } else {
                this->errors.push_back({"Invalid integer input"});
              }
            } else {
              this->errors.push_back({"Failed to read input"});
            }
            break;
          }
          case 3: {
            std::string input;
            std::getline(std::cin, input);
            size_t input_size = input.size();

            if (input_size != -1) {
              if (input[input_size - 1] == '\n') {
                input[input_size - 1] = '\0';
              }

              auto str = input;
              auto idx = hash_string(str);
              auto it = this->str_table.find(idx);
              while (it != this->str_table.cend() && it->second->value != str) {
                idx += 1;
                it = this->str_table.find(idx);
              }

              this->str_table[idx] = std::make_shared<TPV_ObjString>(
                  TPV_ObjString{.hash = (size_t)idx, .value = str});

              this->frames.back().registers.at(r1_idx) = {
                  .type = ValueType::TPV_OBJ,
                  .is_const = false,
                  .value = (TPV_Obj){.type = ObjType::STRING,
                                     .obj = this->str_table.at(idx)}};
            } else {
              this->errors.push_back({"Failed to read input"});
            }

            break;
          }
          default:
            this->errors.push_back({"Invalid flag"});
            break;
        }

        break;
      }
      case Opcode::CALL: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r1_value = get_int32(r1);
        const auto imm1 = bytes_to_int32(this->next_32_bit());

        auto& ref = this->frames.back().registers.at(rd);
        if (r1_value == 0) {
          auto& func = this->functions.at(imm1);
          auto new_frame = Frame{.registers = this->frames.back().registers,
                                 .stack = {},
                                 .pc = 0,
                                 .function = &func};
          this->frames.push_back(new_frame);
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::RETURN: {
        break;
      }
      case Opcode::NEW_ARRAY: {
        auto rd = this->next_8_bit();

        this->frames.back().registers.at(rd) = {
            .type = ValueType::TPV_OBJ,
            .is_const = false,
            .value = (TPV_Obj){.type = ObjType::ARRAY,
                               .obj = std::make_shared<TPV_ObjArray>(
                                   TPV_ObjArray{.values = {}})}};
        break;
      }
      case Opcode::SET_ARRAY: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);
        if (r1.type == ValueType::TPV_OBJ && r2.type == ValueType::TPV_INT) {
          auto list_ref = std::get<TPV_Obj>(r1.value);
          auto list_ptr = std::get<std::shared_ptr<TPV_ObjArray>>(list_ref.obj);
          list_ptr->values.at(std::get<TPV_INT>(r2.value)) =
              this->frames.back().registers.at(rd);
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::GET_ARRAY: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);
        if (r1.type == ValueType::TPV_OBJ && r2.type == ValueType::TPV_INT) {
          auto list_ref = std::get<TPV_Obj>(r1.value);
          auto list_ptr = std::get<std::shared_ptr<TPV_ObjArray>>(list_ref.obj);
          ref = list_ptr->values.at(std::get<TPV_INT>(r2.value));
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::RM_ARRAY: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);
        if (r1.type == ValueType::TPV_OBJ && r2.type == ValueType::TPV_INT) {
          auto list_ref = std::get<TPV_Obj>(r1.value);
          auto list_ptr = std::get<std::shared_ptr<TPV_ObjArray>>(list_ref.obj);
          list_ptr->values.erase(list_ptr->values.begin() +
                                 std::get<TPV_INT>(r2.value));
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::GET_ARRAY_LEN: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);
        if (r1.type == ValueType::TPV_OBJ) {
          auto list_ref = std::get<TPV_Obj>(r1.value);
          auto list_ptr = std::get<std::shared_ptr<TPV_ObjArray>>(list_ref.obj);
          ref = from_raw_value((TPV_INT)list_ptr->values.size());
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::IGL:
        break;
      case Opcode::NOP:
        break;
    }
  }

  return VM_Result::OK;
}

VM_Result VM::eval_one() {
  return VM_Result::OK;
}

void VM::print_regs() {
  for (int i = 0; i < this->frames.back().registers.size(); i++) {
    auto&& ref = this->frames.back().registers.at(i);

    if (ref.type == ValueType::TPV_INT) {
      std::cout << "[int] reg " << i << " : " << std::get<TPV_INT>(ref.value)
                << "\n";
    } else if (ref.type == ValueType::TPV_FLOAT) {
      std::cout << "[float] reg " << i << " : "
                << std::get<TPV_FLOAT>(ref.value) << "\n";
    } else if (ref.type == ValueType::TPV_UNIT) {
      std::cout << "[unit] reg " << i << " : NIL\n";
    } else if (ref.type == ValueType::TPV_OBJ) {
      auto&& obj_ref = std::get<TPV_Obj>(ref.value);
      if (obj_ref.type == ObjType::STRING) {
        auto&& ref = std::get<std::shared_ptr<TPV_ObjString>>(obj_ref.obj);
        std::cout << "[string] reg " << i << " : <TPV_ObjString " << ref->hash
                  << "> " << ref->value << "\n";
      }
    }
  }
}
void VM::print_str_table() {
  std::cout << "String Table Contents:" << std::endl;

  for (const auto& entry : str_table) {
    int idx = entry.first;
    const auto& str_obj = entry.second;

    if (str_obj) {
      std::cout << "Index " << idx << ": " << str_obj->value << std::endl;
    } else {
      std::cout << "Index " << idx << ": [null]" << std::endl;
    }
  }
}
}  // namespace TPV