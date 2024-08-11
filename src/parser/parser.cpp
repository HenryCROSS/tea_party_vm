#include "parser.hpp"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>

namespace TPV {

Parser::Parser() : tokens(), labels(), instructions(), offset(0), err_msg() {}

Parser::~Parser() {}

void Parser::load_tokens(const Tokens& src) {
  auto tokens = src.tokens;
  for (auto&& token : tokens) {
    this->tokens.tokens.push_back(token);
  }
}

Token Parser::next_token() {
  if (offset >= tokens.tokens.size()) {
    throw std::out_of_range("No more tokens available");
  }
  auto token = tokens.tokens.at(offset);
  offset += 1;
  return token;
}

Parser_Result Parser::parse() {
  try {
    first_pass();
    second_pass();
  } catch (const std::exception& e) {
    err_msg.push_back(e.what());
  }

  if (this->err_msg.size()) {
    return {.bytecodes{}, .err_msg{err_msg}};
  } else {
    return {.bytecodes{bytecodes}, .err_msg{}};
  }
}

void Parser::emit_byte(uint8_t byte) {
  bytecodes.push_back(byte);
}

void Parser::emit_word(uint32_t word) {
  bytecodes.push_back((word >> 24) & 0xFF);
  bytecodes.push_back((word >> 16) & 0xFF);
  bytecodes.push_back((word >> 8) & 0xFF);
  bytecodes.push_back(word & 0xFF);
}

void Parser::first_pass() {
  uint32_t bytes_offset = 0;
  while (offset < tokens.tokens.size()) {
    auto token = next_token();

    if (token.type == TokenType::OP) {
      Instruction instr;
      instr.op_val = std::get<OpType>(token.value);

      switch (instr.op_val.value) {
        case Opcode::LOADI:
        case Opcode::LOADF:
        case Opcode::LOADS: {
          instr.rd = std::get<RegisterType>(next_token().value);
          bytes_offset += 2;
          auto value_token = next_token();
          if (std::holds_alternative<Int32Type>(value_token.value)) {
            instr.int_val = std::get<Int32Type>(value_token.value);
            bytes_offset += 4;
          } else if (std::holds_alternative<Float32Type>(value_token.value)) {
            instr.float_val = std::get<Float32Type>(value_token.value);
            bytes_offset += 4;
          } else {
            err_msg.push_back("Type Error at position " +
                              std::to_string(token.begin));
          }
          break;
        }
        case Opcode::LOADNIL: {
          instr.rd = std::get<RegisterType>(next_token().value);
          bytes_offset += 2;
          break;
        }
        case Opcode::STORES: {
          bytes_offset += 1;
          auto token = next_token();
          if (std::holds_alternative<Int32Type>(token.value)) {
            instr.int_val = std::get<Int32Type>(token.value);
            bytes_offset += 4;
          } else {
            err_msg.push_back("Type Error at position " +
                              std::to_string(token.begin));
          }

          auto str_token = next_token();
          if (str_token.type != TokenType::STRING) {
            throw std::runtime_error("Expected string for STORES");
          } else {
            instr.str_val = std::get<StringType>(str_token.value);
            bytes_offset += instr.str_val->value.size();
          }
          break;
        }
        case Opcode::ADD:
        case Opcode::SUB:
        case Opcode::MUL:
        case Opcode::DIV:
        case Opcode::EQ:
        case Opcode::NEQ:
        case Opcode::GT:
        case Opcode::GTE:
        case Opcode::LT:
        case Opcode::LTE:
        case Opcode::BITAND:
        case Opcode::BITOR:
        case Opcode::BITXOR: {
          instr.rd = std::get<RegisterType>(next_token().value);
          instr.r1 = std::get<RegisterType>(next_token().value);
          instr.r2 = std::get<RegisterType>(next_token().value);
          bytes_offset += 4;
          break;
        }
        case Opcode::BITNOT:
        case Opcode::NEGATE: {
          instr.rd = std::get<RegisterType>(next_token().value);
          instr.r1 = std::get<RegisterType>(next_token().value);
          bytes_offset += 3;
          break;
        }
        case Opcode::BITSHL:
        case Opcode::BITSHRL:
        case Opcode::BITSHRA: {
          instr.rd = std::get<RegisterType>(next_token().value);
          instr.r1 = std::get<RegisterType>(next_token().value);
          instr.int_val = std::get<Int32Type>(next_token().value);
          bytes_offset += 7;
          break;
        }
        case Opcode::CVT_I_D:
        case Opcode::CVT_D_I: {
          instr.rd = std::get<RegisterType>(next_token().value);
          instr.r1 = std::get<RegisterType>(next_token().value);
          bytes_offset += 3;
          break;
        }
        case Opcode::HLT:
          break;
        case Opcode::JMP: {
          bytes_offset += 1;
          auto value_token = next_token();
          if (std::holds_alternative<Int32Type>(value_token.value)) {
            instr.int_val = std::get<Int32Type>(value_token.value);
            bytes_offset += 4;
          } else if (std::holds_alternative<LabelRefType>(value_token.value)) {
            instr.label_ref = std::get<LabelRefType>(value_token.value);
            bytes_offset += 4;
          } else {
            err_msg.push_back("Type Error at position " +
                              std::to_string(token.begin));
          }
          break;
        }
        case Opcode::JMP_IF: {
          bytes_offset += 1;
          instr.r1 = std::get<RegisterType>(next_token().value);
          bytes_offset += 1;
          auto value_token = next_token();
          if (std::holds_alternative<Int32Type>(value_token.value)) {
            instr.int_val = std::get<Int32Type>(value_token.value);
            bytes_offset += 4;
          } else if (std::holds_alternative<LabelRefType>(value_token.value)) {
            instr.label_ref = std::get<LabelRefType>(value_token.value);
            bytes_offset += 4;
          } else {
            err_msg.push_back("Type Error at position " +
                              std::to_string(token.begin));
          }
          break;
        }
        case Opcode::VMCALL: {
          bytes_offset += 1;
          instr.r1 = std::get<RegisterType>(next_token().value);
          instr.r2 = std::get<RegisterType>(next_token().value);
          bytes_offset += 2;
          auto value_token = next_token();
          if (std::holds_alternative<Int32Type>(value_token.value)) {
            instr.int_val = std::get<Int32Type>(value_token.value);
            bytes_offset += 4;
          } else {
            err_msg.push_back("Type Error at position " +
                              std::to_string(token.begin));
          }
          break;
        }
        default:
          err_msg.push_back("Unknown opcode at position " +
                            std::to_string(token.begin));
          break;
      }

      instructions.push_back(instr);
    } else if (token.type == TokenType::LABEL) {
      auto label = std::get<LabelType>(token.value).label;
      labels[label] = bytes_offset;
    } else {
      err_msg.push_back("Unexpected token at position " +
                        std::to_string(token.begin));
    }
  }
}

void Parser::second_pass() {
  for (const auto& instr : instructions) {
    emit_byte(static_cast<uint8_t>(instr.op_val.value));

    switch (instr.op_val.value) {
      case Opcode::LOADI: {
        emit_byte(instr.rd->value);
        emit_word(instr.int_val->value);
        break;
      }
      case Opcode::LOADF: {
        emit_byte(instr.rd->value);
        uint32_t value = 0;
        std::memcpy(&value, &instr.float_val->value, sizeof(uint32_t));
        emit_word(value);
        break;
      }
      case Opcode::LOADS: {
        emit_byte(instr.rd->value);
        emit_word(instr.int_val->value);
        break;
      }
      case Opcode::LOADNIL: {
        emit_byte(instr.rd->value);
        break;
      }
      case Opcode::STORES: {
        emit_word(instr.int_val->value);
        for (const auto& ch : instr.str_val->value) {
          emit_byte(static_cast<uint8_t>(ch));
        }
        emit_byte(0);  // Null-terminator
        break;
      }
      case Opcode::ADD:
      case Opcode::SUB:
      case Opcode::MUL:
      case Opcode::DIV:
      case Opcode::EQ:
      case Opcode::NEQ:
      case Opcode::GT:
      case Opcode::GTE:
      case Opcode::LT:
      case Opcode::LTE:
      case Opcode::BITAND:
      case Opcode::BITOR:
      case Opcode::BITXOR:
        emit_byte(instr.rd->value);
        emit_byte(instr.r1->value);
        emit_byte(instr.r2->value);
        break;
      case Opcode::BITNOT:
      case Opcode::NEGATE:
        emit_byte(instr.rd->value);
        emit_byte(instr.r1->value);
        break;
      case Opcode::BITSHL:
      case Opcode::BITSHRL:
      case Opcode::BITSHRA:
        emit_byte(instr.rd->value);
        emit_byte(instr.r1->value);
        emit_word(instr.int_val->value);
        break;
      case Opcode::CVT_I_D:
      case Opcode::CVT_D_I:
        emit_byte(instr.rd->value);
        emit_byte(instr.r1->value);
        break;
      case Opcode::HLT:
        // No additional operands
        break;
      case Opcode::JMP: {
        if (instr.label_ref.has_value()) {
          auto label_ref = instr.label_ref->label;
          auto label_pos = labels.find(label_ref);
          if (label_pos != labels.end()) {
            emit_word(label_pos->second);
          } else {
            err_msg.push_back("Undefined label: " + label_ref);
          }
        } else {
          emit_word(instr.int_val->value);
        }
        break;
      }
      case Opcode::JMP_IF: {
        emit_byte(instr.r1->value);

        if (instr.label_ref.has_value()) {
          auto label_ref = instr.label_ref->label;
          auto label_pos = labels.find(label_ref);
          if (label_pos != labels.end()) {
            emit_word(label_pos->second);
          } else {
            err_msg.push_back("Undefined label: " + label_ref);
          }
        } else {
          emit_word(instr.int_val->value);
        }
        break;
      }
      case Opcode::VMCALL: {
        emit_byte(instr.r1->value);
        emit_byte(instr.r2->value);
        emit_word(instr.int_val->value);
        break;
      }
      default:
        err_msg.push_back("Unknown opcode");
        break;
    }
  }
}

void Parser::print_bytecodes() const {
  for (size_t i = 0; i < bytecodes.size(); ++i) {
    printf("%08b ", bytecodes[i]);
  }
  printf("\n");
}

}  // namespace TPV
