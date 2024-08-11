#include "vm.hpp"
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <variant>
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
  this->frames.push_back({.registers = vector<Value>(256)});
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

        this->frames.back().registers.at(rd) = {.type = ValueType::TPV_INT,
                                                .is_const = false,
                                                .value = bytes_to_int32(val)};

        break;
      }
      case Opcode::LOADF: {
        auto rd = this->next_8_bit();
        const auto val = this->next_32_bit();

        this->frames.back().registers.at(rd) = {.type = ValueType::TPV_FLOAT,
                                                .is_const = false,
                                                .value = bytes_to_float32(val)};

        break;
      }
      case Opcode::LOADS: {
        auto rd = this->next_8_bit();
        const auto idx = bytes_to_int32(this->next_32_bit());
        this->frames.back().registers.at(rd) = {
            .type = ValueType::TPV_OBJ,
            .is_const = false,
            .value = (TPV_Obj){.type = ObjType::STRING,
                               .obj = this->str_table.at(idx)}};

        break;
      }
      case Opcode::LOADNIL: {
        auto rd = this->next_8_bit();
        const auto idx = bytes_to_int32(this->next_32_bit());

        this->frames.back().registers.at(rd) = {.type = ValueType::TPV_UNIT,
                                                .is_const = false,
                                                .value = (TPV_Unit){}};

        break;
      }
      case Opcode::STORES: {
        const auto idx = bytes_to_int32(this->next_32_bit());
        const auto str = bytes_to_string(this->next_string());
        this->str_table[idx] = std::make_shared<TPV_ObjString>(
            (TPV_ObjString){.hash = static_cast<size_t>(idx), .value = str});

        break;
      }
      case Opcode::ADD: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);
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
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

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
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

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
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

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
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

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
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == ValueType::TPV_INT) {
          ref = r1;
        } else if (r1.type == ValueType::TPV_FLOAT) {
          this->frames.back().registers.at(rd) = from_raw_value(
              static_cast<TPV_FLOAT>(std::get<TPV_INT>(r1.value)));
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::NEGATE: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == ValueType::TPV_INT) {
          this->frames.back().registers.at(rd) = from_raw_value(
              static_cast<TPV_FLOAT>(-std::get<TPV_INT>(r1.value)));
        } else if (r1.type == ValueType::TPV_FLOAT) {
          this->frames.back().registers.at(rd) = from_raw_value(
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
        const auto new_pc = bytes_to_int32(this->next_32_bit());
        this->pc = new_pc;
        break;
      }
      case Opcode::JMP_IF: {
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto new_pc = bytes_to_int32(this->next_32_bit());

        if (r1.type == ValueType::TPV_INT) {
          auto val = std::get<TPV_INT>(r1.value);
          if (val)
            this->pc = new_pc;
        } else if (r1.type == ValueType::TPV_FLOAT) {
          auto val = std::get<TPV_FLOAT>(r1.value);
          if (val)
            this->pc = new_pc;
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
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

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
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

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
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

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
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

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
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

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
      case Opcode::BITAND: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto r2 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == r2.type) {
          if (r1.type == ValueType::TPV_INT) {
            ref = from_raw_value(std::get<TPV_INT>(r1.value) &
                                 std::get<TPV_INT>(r2.value));
          } else {
            this->errors.push_back({});
          }
        } else {
          this->errors.push_back({});
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
            ref = from_raw_value(std::get<TPV_INT>(r1.value) |
                                 std::get<TPV_INT>(r2.value));
          } else {
            this->errors.push_back({});
          }
        } else {
          this->errors.push_back({});
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
            ref = from_raw_value(std::get<TPV_INT>(r1.value) ^
                                 std::get<TPV_INT>(r2.value));
          } else {
            this->errors.push_back({});
          }
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::BITNOT: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == ValueType::TPV_INT) {
          ref = from_raw_value(~std::get<TPV_INT>(r1.value));
        } else {
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::BITSHL: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto imm = bytes_to_int32(this->next_32_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == ValueType::TPV_INT) {
          ref = from_raw_value(std::get<TPV_INT>(r1.value) << imm);
        } else {
          this->errors.push_back({});
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
          this->errors.push_back({});
        }

        break;
      }
      case Opcode::BITSHRA: {
        auto rd = this->next_8_bit();
        const auto r1 = this->frames.back().registers.at(this->next_8_bit());
        const auto imm = bytes_to_int32(this->next_32_bit());

        auto& ref = this->frames.back().registers.at(rd);

        if (r1.type == ValueType::TPV_INT) {
          ref = from_raw_value(std::get<TPV_INT>(r1.value) >> imm);
        } else {
          this->errors.push_back({});
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
              const auto num = std::get<TPV_INT>(r1.value);
              std::printf("%d", num);
            } else if (r1.type == ValueType::TPV_FLOAT) {
              const auto num = std::get<TPV_FLOAT>(r1.value);
              std::printf("%f", num);
            } else if (r1.type == ValueType::TPV_OBJ) {
              const auto str = std::get<TPV_Obj>(r1.value);
              const auto ptr =
                  std::get<std::shared_ptr<TPV_ObjString>>(str.obj);
              std::printf("%s", ptr->value.c_str());
            } else {
              this->errors.push_back({"Nothing in the register"});
            }

            const auto& r2 = this->frames.back().registers.at(r2_idx);
            if (r2.type == ValueType::TPV_INT) {
              const auto flag = std::get<TPV_INT>(r2.value);
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
                    from_raw_value((TPV_INT)input);
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
                    from_raw_value((TPV_FLOAT)input);
              } else {
                this->errors.push_back({"Invalid integer input"});
              }
            } else {
              this->errors.push_back({"Failed to read input"});
            }
            break;
          }
          case 3: {
            char* input = nullptr;
            size_t bufsize = 0;
            ssize_t input_size = getline(&input, &bufsize, stdin);

            if (input_size != -1) {
              if (input[input_size - 1] == '\n') {
                input[input_size - 1] = '\0';
              }

              auto str = std::string(input);
              auto idx = hash_string(str);
              auto it = this->str_table.find(idx);
              do {
                idx += 1;
                it = this->str_table.find(idx);
              } while (it != this->str_table.cend());

              this->str_table[idx] =
                  std::make_shared<TPV_ObjString>(TPV_ObjString{.value = str});

              this->frames.back().registers.at(r1_idx) = {
                  .type = ValueType::TPV_OBJ,
                  .is_const = false,
                  .value = (TPV_Obj){.type = ObjType::STRING,
                                     .obj = this->str_table.at(idx)}};
            } else {
              this->errors.push_back({"Failed to read input"});
            }

            free(input);
            break;
          }
          default:
            this->errors.push_back({"Invalid flag"});
            break;
        }

        break;
      }
      case Opcode::NEW_TABLE: {
        auto rd = this->next_8_bit();
        auto& ref = this->frames.back().registers.at(rd);

        auto idx = this->table_table.size();
        auto tbl = std::make_shared<TPV_ObjTable>(TPV_ObjTable{.hash = idx});
        this->table_table.at(idx) = tbl;

        auto obj = TPV_Obj{.type = ObjType::TABLE, .obj = tbl};
        auto val =
            Value{.type = ValueType::TPV_OBJ, .is_const = false, .value = obj};

        ref = val;
        break;
      }
      case Opcode::SET_TABLE: {
        auto rd = this->next_8_bit();
        auto r1 = this->next_8_bit();
        const auto imm = bytes_to_int32(this->next_32_bit());

        auto& ref_r1 = this->frames.back().registers.at(r1);
        auto& ref_rd = this->frames.back().registers.at(rd);

        auto obj = std::get<TPV_Obj>(ref_rd.value);
        auto tbl = std::get<std::shared_ptr<TPV_ObjTable>>(obj.obj);

        tbl->tbl[imm] = ref_rd;

        break;
      }
      case Opcode::GET_TABLE: {
        auto rd = this->next_8_bit();
        const auto imm = bytes_to_int32(this->next_32_bit());
        auto& ref_rd = this->frames.back().registers.at(rd);

        auto tbl = this->table_table[imm];

        ref_rd = Value{.type = ValueType::TPV_OBJ,
                       .is_const = false,
                       .value = TPV_Obj{.type = ObjType::TABLE, .obj = tbl}};

        break;
      }
      case Opcode::SET_GLOBAL:
      case Opcode::GET_GLOBAL:
      case Opcode::SET_CONSTANT:
      case Opcode::CALL:
      case Opcode::RETURN:
      case Opcode::CLOSURE:
      case Opcode::SET_LIST:
      case Opcode::GET_LIST:
      case Opcode::SET_ARRAY:
      case Opcode::GET_ARRAY:
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
      std::cout << "reg " << i << " : " << std::get<TPV_INT>(ref.value) << "\n";
    } else if (ref.type == ValueType::TPV_FLOAT) {
      std::cout << "reg " << i << " : " << std::get<TPV_FLOAT>(ref.value)
                << "\n";
    } else if (ref.type == ValueType::TPV_UNIT) {
      std::cout << "reg " << i << " : NIL\n";
    } else if (ref.type == ValueType::TPV_OBJ) {
      auto&& obj_ref = std::get<TPV_Obj>(ref.value);
      if (obj_ref.type == ObjType::STRING) {
        auto&& ref = std::get<std::shared_ptr<TPV_ObjString>>(obj_ref.obj);
        std::cout << "reg " << i << " : <TPV_ObjString " << ref << "> "
                  << ref->value << "\n";
      } else if (obj_ref.type == ObjType::MAP) {
        std::cout << "reg " << i << " : <TPV_ObjTable "
                  << std::get<std::shared_ptr<TPV_ObjTable>>(obj_ref.obj)
                  << ">\n";
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