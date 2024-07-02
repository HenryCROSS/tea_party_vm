#include "vm.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include "../error_code.hpp"
#include "../instructions.hpp"
#include "../utils.hpp"

using std::vector;

namespace TPV {

VM::VM()
    : bytes(),
      str_table(),
      frames(MAX_FRAME),
      flags(),
      pc(0),
      is_running(true) {
  this->frames.push_back({.stack = vector<Value>(256)});
}

uint8_t VM::next_8_bit() {
  auto byte = this->bytes.at(pc);
  pc += 1;

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

bool VM::load_bytes(const vector<uint8_t> instructions) {
  this->bytes.insert(this->bytes.cend(), instructions.cbegin(),
                     instructions.cend());

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

  this->bytes = std::vector<uint8_t>(out.cbegin(), out.cend());

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

  // TODO: parse to byte code

  // TODO: insert to this->bytes
  // this->bytes = std::vector<uint8_t>(out.cbegin(), out.cend());

  return false;
}

VM_Result VM::eval_all() {
  while (is_running && this->bytes.size() > this->pc) {
    auto byte = this->next_8_bit();

    // decoding
    auto opcode = static_cast<Opcode>(byte);

    switch (opcode) {
      case Opcode::LOADI: {
        auto rd = this->next_8_bit();
        const auto val = this->next_32_bit();

        this->frames.back().stack.at(rd) = {.type = ValueType::TPV_INT,
                                            .is_const = false,
                                            .value = bytes_to_int32(val)};

        break;
      }
      case Opcode::LOADF: {
        auto rd = this->next_8_bit();
        const auto val = this->next_32_bit();

        this->frames.back().stack.at(rd) = {.type = ValueType::TPV_FLOAT,
                                            .is_const = false,
                                            .value = bytes_to_float32(val)};

        break;
      }
      case Opcode::LOADS: {
        auto rd = this->next_8_bit();
        const auto val = this->next_32_bit();

        // TODO

        break;
      }
      case Opcode::ADD: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().stack.at(this->next_8_bit());
        const auto r2 = this->frames.back().stack.at(this->next_8_bit());

        auto& ref = this->frames.back().stack.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(std::get<TPV_INT>(r1.value) +
                                 std::get<TPV_INT>(r2.value));
          } else if (r1.type == ValueType::TPV_FLOAT) {
            ref = from_raw_value(std::get<TPV_FLOAT>(r1.value) +
                                 std::get<TPV_FLOAT>(r2.value));
          } else {
            this->errors.push_back({});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::SUB: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().stack.at(this->next_8_bit());
        const auto r2 = this->frames.back().stack.at(this->next_8_bit());

        auto& ref = this->frames.back().stack.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(std::get<TPV_INT>(r1.value) -
                                 std::get<TPV_INT>(r2.value));
          } else if (r1.type == ValueType::TPV_FLOAT) {
            ref = from_raw_value(std::get<TPV_FLOAT>(r1.value) -
                                 std::get<TPV_FLOAT>(r2.value));
          } else {
            this->errors.push_back({});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::MUL: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().stack.at(this->next_8_bit());
        const auto r2 = this->frames.back().stack.at(this->next_8_bit());

        auto& ref = this->frames.back().stack.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(std::get<TPV_INT>(r1.value) *
                                 std::get<TPV_INT>(r2.value));
          } else if (r1.type == ValueType::TPV_FLOAT) {
            ref = from_raw_value(std::get<TPV_FLOAT>(r1.value) *
                                 std::get<TPV_FLOAT>(r2.value));
          } else {
            this->errors.push_back({});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::DIV: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().stack.at(this->next_8_bit());
        const auto r2 = this->frames.back().stack.at(this->next_8_bit());

        auto& ref = this->frames.back().stack.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            if (std::get<TPV_INT>(r2.value) == 0) {
              this->errors.push_back({});
            } else {
              ref = from_raw_value(std::get<TPV_INT>(r1.value) /
                                   std::get<TPV_INT>(r2.value));
            }
          } else if (r1.type == ValueType::TPV_FLOAT) {
            if (std::get<TPV_FLOAT>(r2.value) == 0.0) {
              this->errors.push_back({});
            } else {
              ref = from_raw_value(std::get<TPV_FLOAT>(r1.value) /
                                   std::get<TPV_FLOAT>(r2.value));
            }
          } else {
            this->errors.push_back({});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::CVT_I_D: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().stack.at(this->next_8_bit());

        auto& ref = this->frames.back().stack.at(rd);

        if (r1.type == ValueType::TPV_INT) {
          ref = from_raw_value(
              static_cast<TPV_FLOAT>(std::get<TPV_FLOAT>(r1.value)));
        } else if (r1.type == ValueType::TPV_FLOAT) {
          ref = r1;
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::CVT_D_I: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().stack.at(this->next_8_bit());

        auto& ref = this->frames.back().stack.at(rd);

        if (r1.type == ValueType::TPV_INT) {
          ref = r1;
        } else if (r1.type == ValueType::TPV_FLOAT) {
          this->frames.back().stack.at(rd) = from_raw_value(
              static_cast<TPV_FLOAT>(std::get<TPV_INT>(r1.value)));
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::NEGATE: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().stack.at(this->next_8_bit());

        auto& ref = this->frames.back().stack.at(rd);

        if (r1.type == ValueType::TPV_INT) {
          this->frames.back().stack.at(rd) = from_raw_value(
              static_cast<TPV_FLOAT>(-std::get<TPV_INT>(r1.value)));
        } else if (r1.type == ValueType::TPV_FLOAT) {
          this->frames.back().stack.at(rd) = from_raw_value(
              static_cast<TPV_FLOAT>(-std::get<TPV_FLOAT>(r1.value)));
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::HLT: {
        this->is_running = false;
        break;
      }
      case Opcode::JMP: {
        auto rd = this->next_8_bit();
        const auto new_pc = bytes_to_int32(this->next_32_bit());
        this->pc = new_pc;
        break;
      }
      case Opcode::EQ: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().stack.at(this->next_8_bit());
        const auto r2 = this->frames.back().stack.at(this->next_8_bit());

        auto& ref = this->frames.back().stack.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(std::get<TPV_INT>(r1.value) ==
                                 std::get<TPV_INT>(r2.value));
          } else if (r1.type == ValueType::TPV_FLOAT) {
            ref = from_raw_value(std::get<TPV_FLOAT>(r1.value) ==
                                 std::get<TPV_FLOAT>(r2.value));
          } else {
            this->errors.push_back({});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::NEQ: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().stack.at(this->next_8_bit());
        const auto r2 = this->frames.back().stack.at(this->next_8_bit());

        auto& ref = this->frames.back().stack.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(std::get<TPV_INT>(r1.value) !=
                                 std::get<TPV_INT>(r2.value));
          } else if (r1.type == ValueType::TPV_FLOAT) {
            ref = from_raw_value(std::get<TPV_FLOAT>(r1.value) !=
                                 std::get<TPV_FLOAT>(r2.value));
          } else {
            this->errors.push_back({});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::GT: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().stack.at(this->next_8_bit());
        const auto r2 = this->frames.back().stack.at(this->next_8_bit());

        auto& ref = this->frames.back().stack.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(std::get<TPV_INT>(r1.value) >
                                 std::get<TPV_INT>(r2.value));
          } else if (r1.type == ValueType::TPV_FLOAT) {
            ref = from_raw_value(std::get<TPV_FLOAT>(r1.value) >
                                 std::get<TPV_FLOAT>(r2.value));
          } else {
            this->errors.push_back({});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::GTE: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().stack.at(this->next_8_bit());
        const auto r2 = this->frames.back().stack.at(this->next_8_bit());

        auto& ref = this->frames.back().stack.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(std::get<TPV_INT>(r1.value) >=
                                 std::get<TPV_INT>(r2.value));
          } else if (r1.type == ValueType::TPV_FLOAT) {
            ref = from_raw_value(std::get<TPV_FLOAT>(r1.value) >=
                                 std::get<TPV_FLOAT>(r2.value));
          } else {
            this->errors.push_back({});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::LT: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().stack.at(this->next_8_bit());
        const auto r2 = this->frames.back().stack.at(this->next_8_bit());

        auto& ref = this->frames.back().stack.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(std::get<TPV_INT>(r1.value) <
                                 std::get<TPV_INT>(r2.value));
          } else if (r1.type == ValueType::TPV_FLOAT) {
            ref = from_raw_value(std::get<TPV_FLOAT>(r1.value) <
                                 std::get<TPV_FLOAT>(r2.value));
          } else {
            this->errors.push_back({});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::LTE: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().stack.at(this->next_8_bit());
        const auto r2 = this->frames.back().stack.at(this->next_8_bit());

        auto& ref = this->frames.back().stack.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(std::get<TPV_INT>(r1.value) <=
                                 std::get<TPV_INT>(r2.value));
          } else if (r1.type == ValueType::TPV_FLOAT) {
            ref = from_raw_value(std::get<TPV_FLOAT>(r1.value) <=
                                 std::get<TPV_FLOAT>(r2.value));
          } else {
            this->errors.push_back({});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::BITAND:
      case Opcode::BITOR:
      case Opcode::BITXOR:
      case Opcode::BITNOT:
      case Opcode::BITSHL:
      case Opcode::BITSHRL:
      case Opcode::BITSHRA:
      case Opcode::SET_LOCAL:
      case Opcode::GET_LOCAL:
      case Opcode::SET_GLOBAL:
      case Opcode::GET_GLOBAL:
      case Opcode::SET_CONSTANT:
      case Opcode::CALL:
      case Opcode::RETURN:
      case Opcode::CLOSURE:
      case Opcode::SET_SUM:
      case Opcode::GET_SUM:
      case Opcode::SET_LIST:
      case Opcode::GET_LIST:
      case Opcode::SET_TABLE:
      case Opcode::GET_TABLE:
      case Opcode::SET_ARRAY:
      case Opcode::GET_ARRAY:
      case Opcode::IGL:
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
  for (int i = 0; i < this->frames.back().stack.size(); i++) {
    auto&& ref = this->frames.back().stack.at(i);

    if (ref.type == ValueType::TPV_INT) {
      std::cout << "reg " << i << " : " << std::get<TPV_INT>(ref.value)
                << "\n";
    } else if (ref.type == ValueType::TPV_FLOAT) {
      std::cout << "reg " << i << " : " << std::get<TPV_FLOAT>(ref.value)
                << "\n";
    }
  }
}
}  // namespace TPV