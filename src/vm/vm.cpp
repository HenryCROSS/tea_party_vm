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
    : registers_i(256),
      registers_d(256),
      bytes(),
      str_table(),
      stack(),
      flags(),
      pc(0),
      is_running(true) {}

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
      case Opcode::LOAD_I: {
        auto rd = this->next_8_bit();
        const auto val = this->next_32_bit();

        this->registers_i.at(rd) = bytes_to_int32(val);

        break;
      }
      case Opcode::LOAD_D: {
        auto rd = this->next_8_bit();
        const auto val = this->next_32_bit();

        this->registers_d.at(rd) = bytes_to_float32(val);

        break;
      }
      case Opcode::ADD_I: {
        auto rd = this->next_8_bit();
        const auto r1 = this->registers_i.at(this->next_8_bit());
        const auto r2 = this->registers_i.at(this->next_8_bit());

        this->registers_i.at(rd) = r1 + r2;

        break;
      }
      case Opcode::SUB_I: {
        auto rd = this->next_8_bit();
        const auto r1 = this->registers_i.at(this->next_8_bit());
        const auto r2 = this->registers_i.at(this->next_8_bit());

        this->registers_i.at(rd) = r1 - r2;

        break;
      }
      case Opcode::MUL_I: {
        auto rd = this->next_8_bit();
        const auto r1 = this->registers_i.at(this->next_8_bit());
        const auto r2 = this->registers_i.at(this->next_8_bit());

        this->registers_i.at(rd) = r1 * r2;

        break;
      }
      case Opcode::DIV_I: {
        auto rd = this->next_8_bit();
        const auto r1 = this->registers_i.at(this->next_8_bit());
        const auto r2 = this->registers_i.at(this->next_8_bit());

        this->registers_i.at(rd) = r1 / r2;

        break;
      }
      case Opcode::ADD_D: {
        auto rd = this->next_8_bit();
        const auto fr1 = this->registers_d.at(this->next_8_bit());
        const auto fr2 = this->registers_d.at(this->next_8_bit());

        this->registers_d.at(rd) = fr1 + fr2;

        break;
      }
      case Opcode::SUB_D: {
        auto rd = this->next_8_bit();
        const auto fr1 = this->registers_d.at(this->next_8_bit());
        const auto fr2 = this->registers_d.at(this->next_8_bit());

        this->registers_d.at(rd) = fr1 - fr2;

        break;
      }
      case Opcode::MUL_D: {
        auto rd = this->next_8_bit();
        const auto fr1 = this->registers_d.at(this->next_8_bit());
        const auto fr2 = this->registers_d.at(this->next_8_bit());

        this->registers_d.at(rd) = fr1 * fr2;

        break;
      }
      case Opcode::DIV_D: {
        auto rd = this->next_8_bit();
        const auto fr1 = this->registers_d.at(this->next_8_bit());
        const auto fr2 = this->registers_d.at(this->next_8_bit());

        this->registers_d.at(rd) = fr1 / fr2;

        break;
      }
      case Opcode::CVT_I_D: {
        auto rd = this->next_8_bit();
        const auto r1 = this->registers_i.at(this->next_8_bit());

        this->registers_d.at(rd) = r1;

        break;
      }
      case Opcode::CVT_D_I: {
        auto rd = this->next_8_bit();
        const auto fr1 = this->registers_d.at(this->next_8_bit());

        this->registers_i.at(rd) = fr1;

        break;
      }
      case Opcode::NEGATE_I: {
        auto rd = this->next_8_bit();
        const auto r1 = this->registers_i.at(this->next_8_bit());

        this->registers_i.at(rd) = -r1;

        break;
      }
      case Opcode::NEGATE_D: {
        auto rd = this->next_8_bit();
        const auto fr1 = this->registers_d.at(this->next_8_bit());

        this->registers_d.at(rd) = -fr1;

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
      case Opcode::EQ_I: {
        auto rd = this->next_8_bit();
        const auto r1 = this->registers_i.at(this->next_8_bit());
        const auto r2 = this->registers_i.at(this->next_8_bit());

        this->registers_i.at(rd) = r1 == r2;

        break;
      }
      case Opcode::NEQ_I: {
        auto rd = this->next_8_bit();
        const auto r1 = this->registers_i.at(this->next_8_bit());
        const auto r2 = this->registers_i.at(this->next_8_bit());

        this->registers_i.at(rd) = r1 != r2;

        break;
      }
      case Opcode::GT_I: {
        auto rd = this->next_8_bit();
        const auto r1 = this->registers_i.at(this->next_8_bit());
        const auto r2 = this->registers_i.at(this->next_8_bit());

        this->registers_i.at(rd) = r1 > r2;

        break;
      }
      case Opcode::GTE_I: {
        auto rd = this->next_8_bit();
        const auto r1 = this->registers_i.at(this->next_8_bit());
        const auto r2 = this->registers_i.at(this->next_8_bit());

        this->registers_i.at(rd) = r1 >= r2;

        break;
      }
      case Opcode::LT_I: {
        auto rd = this->next_8_bit();
        const auto r1 = this->registers_i.at(this->next_8_bit());
        const auto r2 = this->registers_i.at(this->next_8_bit());

        this->registers_i.at(rd) = r1 < r2;

        break;
      }
      case Opcode::LTE_I: {
        auto rd = this->next_8_bit();
        const auto r1 = this->registers_i.at(this->next_8_bit());
        const auto r2 = this->registers_i.at(this->next_8_bit());

        this->registers_i.at(rd) = r1 <= r2;

        break;
      }
      case Opcode::EQ_D: {
        auto rd = this->next_8_bit();
        const auto r1 = this->registers_d.at(this->next_8_bit());
        const auto r2 = this->registers_d.at(this->next_8_bit());

        this->registers_d.at(rd) = r1 == r2;

        break;
      }
      case Opcode::NEQ_D: {
        auto rd = this->next_8_bit();
        const auto r1 = this->registers_d.at(this->next_8_bit());
        const auto r2 = this->registers_d.at(this->next_8_bit());

        this->registers_d.at(rd) = r1 != r2;

        break;
      }
      case Opcode::GT_D: {
        auto rd = this->next_8_bit();
        const auto r1 = this->registers_d.at(this->next_8_bit());
        const auto r2 = this->registers_d.at(this->next_8_bit());

        this->registers_d.at(rd) = r1 > r2;

        break;
      }
      case Opcode::GTE_D: {
        auto rd = this->next_8_bit();
        const auto r1 = this->registers_d.at(this->next_8_bit());
        const auto r2 = this->registers_d.at(this->next_8_bit());

        this->registers_d.at(rd) = r1 >= r2;

        break;
      }
      case Opcode::LT_D: {
        auto rd = this->next_8_bit();
        const auto r1 = this->registers_d.at(this->next_8_bit());
        const auto r2 = this->registers_d.at(this->next_8_bit());

        this->registers_d.at(rd) = r1 < r2;

        break;
      }
      case Opcode::LTE_D: {
        auto rd = this->next_8_bit();
        const auto r1 = this->registers_d.at(this->next_8_bit());
        const auto r2 = this->registers_d.at(this->next_8_bit());

        this->registers_d.at(rd) = r1 <= r2;

        break;
      }
      case Opcode::CALL:
      case Opcode::RETURN:
      case Opcode::CLOSURE:
      case Opcode::SET_GLOBAL:
      case Opcode::SET_CONSTANT:
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
  for (int i = 0; i < this->registers_i.size(); i++) {
    std::cout << "reg_i " << i << " : " << this->registers_i.at(i) << "\n";
  }

  for (int i = 0; i < this->registers_d.size(); i++) {
    std::cout << "reg_d " << i << " : " << this->registers_d.at(i) << "\n";
  }
}
}  // namespace TPV