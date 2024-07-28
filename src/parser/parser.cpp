#include "parser.hpp"
#include <stdexcept>
#include <string>

namespace TPV {

Parser::Parser() : tokens(), offset(0), err_msg() {}

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
  while (offset < tokens.tokens.size()) {
    try {
      parse_instruction();
    } catch (const std::exception& e) {
      err_msg.push_back(e.what());
    }
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

void Parser::parse_instruction() {
  auto token = next_token();

  if (token.type != TokenType::OP) {
    err_msg.push_back("Expected opcode at position " +
                      std::to_string(token.begin));
    return;
  }

  auto opcode = std::get<OpType>(token.value).value;
  emit_byte(static_cast<uint8_t>(opcode));

  try {
    switch (opcode) {
      case Opcode::LOADI:
      case Opcode::LOADF:
      case Opcode::LOADS: {
        auto rd = std::get<RegisterType>(next_token().value).value;
        emit_byte(rd);
        auto imm = std::get<Int32Type>(next_token().value).value;
        emit_word(imm);
        break;
      }
      case Opcode::STORES: {
        auto imm = std::get<Int32Type>(next_token().value).value;
        emit_word(imm);
        auto str_token = next_token();
        if (str_token.type != TokenType::STRING) {
          throw std::runtime_error("Expected string for STORES");
        }
        auto str = std::get<StringType>(str_token.value).value;
        bytecodes.insert(bytecodes.end(), str.begin(), str.end());
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
      case Opcode::BITXOR: {
        auto rd = std::get<RegisterType>(next_token().value).value;
        auto r1 = std::get<RegisterType>(next_token().value).value;
        auto r2 = std::get<RegisterType>(next_token().value).value;
        emit_byte(rd);
        emit_byte(r1);
        emit_byte(r2);
        break;
      }
      case Opcode::BITNOT:
      case Opcode::NEGATE: {
        auto rd = std::get<RegisterType>(next_token().value).value;
        auto r1 = std::get<RegisterType>(next_token().value).value;
        emit_byte(rd);
        emit_byte(r1);
        break;
      }
      case Opcode::BITSHL:
      case Opcode::BITSHRL:
      case Opcode::BITSHRA: {
        auto rd = std::get<RegisterType>(next_token().value).value;
        auto r1 = std::get<RegisterType>(next_token().value).value;
        auto imm = std::get<Int32Type>(next_token().value).value;
        emit_byte(rd);
        emit_byte(r1);
        emit_word(imm);
        break;
      }
      case Opcode::CVT_I_D:
      case Opcode::CVT_D_I: {
        auto rd = std::get<RegisterType>(next_token().value).value;
        auto r1 = std::get<RegisterType>(next_token().value).value;
        emit_byte(rd);
        emit_byte(r1);
        break;
      }
      case Opcode::HLT: {
        // No additional operands
        break;
      }
      case Opcode::JMP: {
        auto addr = std::get<Int32Type>(next_token().value).value;
        emit_word(addr);
        break;
      }
      default:
        err_msg.push_back("Unknown opcode at position " +
                          std::to_string(token.begin));
        break;
    }
  } catch (const std::exception& e) {
    err_msg.push_back(e.what());
  }
}

}  // namespace TPV
