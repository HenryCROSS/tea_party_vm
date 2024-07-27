#include "scanner.hpp"
#include <cctype>
#include <charconv>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

static bool starts_with(std::string_view str, std::string_view prefix) {
  size_t pos = str.find_first_of(" \t\n\r");
  if (pos == std::string_view::npos) {
    pos = str.size();
  }
  return str.substr(0, pos) == prefix;
}

namespace TPV {

std::unordered_map<std::string, Opcode> opcode_map = {
    {"LOADI", Opcode::LOADI},
    {"LOADF", Opcode::LOADF},
    {"LOADS", Opcode::LOADS},
    {"STORES", Opcode::STORES},
    {"ADD", Opcode::ADD},
    {"SUB", Opcode::SUB},
    {"MUL", Opcode::MUL},
    {"DIV", Opcode::DIV},
    {"CVT_I_D", Opcode::CVT_I_D},
    {"CVT_D_I", Opcode::CVT_D_I},
    {"NEGATE", Opcode::NEGATE},
    {"HLT", Opcode::HLT},
    {"JMP", Opcode::JMP},
    {"EQ", Opcode::EQ},
    {"NEQ", Opcode::NEQ},
    {"GT", Opcode::GT},
    {"GTE", Opcode::GTE},
    {"LT", Opcode::LT},
    {"LTE", Opcode::LTE},
    {"BITAND", Opcode::BITAND},
    {"BITOR", Opcode::BITOR},
    {"BITXOR", Opcode::BITXOR},
    {"BITNOT", Opcode::BITNOT},
    {"BITSHL", Opcode::BITSHL},
    {"BITSHRL", Opcode::BITSHRL},
    {"BITSHRA", Opcode::BITSHRA},
    {"SET_LOCAL", Opcode::SET_LOCAL},
    {"GET_LOCAL", Opcode::GET_LOCAL},
    {"SET_GLOBAL", Opcode::SET_GLOBAL},
    {"GET_GLOBAL", Opcode::GET_GLOBAL},
    {"SET_CONSTANT", Opcode::SET_CONSTANT},
    {"CALL", Opcode::CALL},
    {"RETURN", Opcode::RETURN},
    {"CLOSURE", Opcode::CLOSURE},
    {"SET_SUM", Opcode::SET_SUM},
    {"GET_SUM", Opcode::GET_SUM},
    {"SET_LIST", Opcode::SET_LIST},
    {"GET_LIST", Opcode::GET_LIST},
    {"SET_TABLE", Opcode::SET_TABLE},
    {"GET_TABLE", Opcode::GET_TABLE},
    {"SET_ARRAY", Opcode::SET_ARRAY},
    {"GET_ARRAY", Opcode::GET_ARRAY},
    {"IGL", Opcode::IGL},
    {"NOP", Opcode::NOP}};

std::optional<Token> scan_opcode(std::string_view str,
                                 uint32_t start_pos,
                                 uint32_t absolute_pos,
                                 uint32_t line) {
  if (!std::isupper(str[0])) {
    return std::nullopt;
  }

  for (const auto& [key, value] : opcode_map) {
    if (starts_with(str, key)) {
      return Token{
          absolute_pos, absolute_pos + static_cast<uint32_t>(key.length()),
          start_pos,    start_pos + static_cast<uint32_t>(key.length()),
          line,         TokenType::OP,
          OpType{value}};
    }
  }

  size_t pos = str.find_first_of(" \t\n\r");
  if (pos == std::string_view::npos) {
    pos = str.size();
  }

  std::string unknown_op(str.substr(0, pos));
  std::string error_msg = "Unknown opcode: " + unknown_op;
  return Token{absolute_pos,
               absolute_pos + static_cast<uint32_t>(pos),
               start_pos,
               start_pos + static_cast<uint32_t>(pos),
               line,
               TokenType::ERR,
               ErrType{error_msg}};
}

std::optional<Token> scan_register(std::string_view str,
                                   uint32_t start_pos,
                                   uint32_t absolute_pos,
                                   uint32_t line) {
  if (str.size() >= 2 && str[0] == 'r' && std::isdigit(str[1])) {
    size_t pos = 1;
    while (pos < str.size() && std::isdigit(str[pos])) {
      pos++;
    }
    int reg_value = 0;
    auto result = std::from_chars(str.data() + 1, str.data() + pos, reg_value);
    if (result.ec == std::errc{} && reg_value >= 0 && reg_value <= 255) {
      return Token{absolute_pos,
                   absolute_pos + static_cast<uint32_t>(pos),
                   start_pos,
                   start_pos + static_cast<uint32_t>(pos),
                   line,
                   TokenType::REGISTER,
                   RegisterType{static_cast<uint8_t>(reg_value)}};
    }
  }
  return std::nullopt;
}

std::optional<Token> scan_int(std::string_view str,
                              uint32_t start_pos,
                              uint32_t absolute_pos,
                              uint32_t line) {
  size_t pos = 0;
  if (str[pos] == '-' || str[pos] == '+') {
    pos++;
  }
  while (pos < str.size() && std::isdigit(str[pos])) {
    pos++;
  }
  if (pos > 0 && (pos > 1 || std::isdigit(str[0]))) {
    int32_t int_value = 0;
    auto result = std::from_chars(str.data(), str.data() + pos, int_value);
    if (result.ec == std::errc{}) {
      return Token{absolute_pos,
                   absolute_pos + static_cast<uint32_t>(pos),
                   start_pos,
                   start_pos + static_cast<uint32_t>(pos),
                   line,
                   TokenType::INT32,
                   Int32Type{int_value}};
    }
  }
  return std::nullopt;
}

std::optional<Token> scan_float(std::string_view str,
                                uint32_t start_pos,
                                uint32_t absolute_pos,
                                uint32_t line) {
  size_t pos = 0;
  bool has_dot = false;
  if (str[pos] == '-' || str[pos] == '+') {
    pos++;
  }
  while (pos < str.size() &&
         (std::isdigit(str[pos]) || (str[pos] == '.' && !has_dot))) {
    if (str[pos] == '.') {
      has_dot = true;
    }
    pos++;
  }
  if (has_dot && pos > 1) {
    float float_value = 0.0f;
    auto result = std::from_chars(str.data(), str.data() + pos, float_value);
    if (result.ec == std::errc{}) {
      return Token{absolute_pos,
                   absolute_pos + static_cast<uint32_t>(pos),
                   start_pos,
                   start_pos + static_cast<uint32_t>(pos),
                   line,
                   TokenType::FLOAT32,
                   Float32Type{float_value}};
    }
  }
  return std::nullopt;
}

std::optional<Token> scan_string(std::string_view str,
                                 uint32_t start_pos,
                                 uint32_t absolute_pos,
                                 uint32_t line) {
  if (str.size() < 2 || str[0] != '"') {
    return std::nullopt;
  }

  size_t end_pos = str.find('"', 1);
  if (end_pos == std::string_view::npos) {
    std::string error_msg = "Unterminated string literal";
    return Token{absolute_pos,
                 absolute_pos + static_cast<uint32_t>(str.size()),
                 start_pos,
                 start_pos + static_cast<uint32_t>(str.size()),
                 line,
                 TokenType::ERR,
                 ErrType{error_msg}};
  }

  std::string value(str.substr(1, end_pos - 1));
  return Token{absolute_pos,
               absolute_pos + static_cast<uint32_t>(end_pos + 1),
               start_pos,
               start_pos + static_cast<uint32_t>(end_pos + 1),
               line,
               TokenType::STRING,
               StringType{StringFormat::ASCII, value}};
}

std::optional<Tokens> scan_all(std::string_view str) {
  std::vector<Token> token_list;
  std::string_view remaining = str;
  uint32_t current_pos = 0;
  uint32_t absolute_pos = 0;  // 绝对位置
  uint32_t line = 1;

  while (!remaining.empty()) {
    // Handle new lines for Windows (\r\n) and Unix (\n)
    if (remaining[0] == '\n') {
      remaining.remove_prefix(1);
      current_pos = 0;
      absolute_pos++;
      line++;
      continue;
    } else if (remaining.size() > 1 && remaining[0] == '\r' &&
               remaining[1] == '\n') {
      remaining.remove_prefix(2);
      current_pos = 0;
      absolute_pos += 2;
      line++;
      continue;
    }

    // Skip whitespace and commas
    if (remaining[0] == ' ' || remaining[0] == '\t' || remaining[0] == ',') {
      remaining.remove_prefix(1);
      current_pos++;
      absolute_pos++;
      continue;
    }

    if (remaining.empty())
      break;

    // Try to scan each type of token
    if (auto token = scan_opcode(remaining, current_pos, absolute_pos, line)) {
      token_list.push_back(*token);
      auto token_length = token->end - token->begin;
      remaining.remove_prefix(token_length);
      current_pos += token_length;
      absolute_pos += token_length;
    } else if (auto token =
                   scan_register(remaining, current_pos, absolute_pos, line)) {
      token_list.push_back(*token);
      auto token_length = token->end - token->begin;
      remaining.remove_prefix(token_length);
      current_pos += token_length;
      absolute_pos += token_length;
    } else if (auto token =
                   scan_int(remaining, current_pos, absolute_pos, line)) {
      token_list.push_back(*token);
      auto token_length = token->end - token->begin;
      remaining.remove_prefix(token_length);
      current_pos += token_length;
      absolute_pos += token_length;
    } else if (auto token =
                   scan_float(remaining, current_pos, absolute_pos, line)) {
      token_list.push_back(*token);
      auto token_length = token->end - token->begin;
      remaining.remove_prefix(token_length);
      current_pos += token_length;
      absolute_pos += token_length;
    } else if (auto token =
                   scan_string(remaining, current_pos, absolute_pos, line)) {
      token_list.push_back(*token);
      auto token_length = token->end - token->begin;
      remaining.remove_prefix(token_length);
      current_pos += token_length;
      absolute_pos += token_length;
    } else {
      // Unknown token, skip one character and try again
      std::string error_msg =
          "Unknown token starting at position " + std::to_string(current_pos);
      token_list.push_back(Token{absolute_pos, absolute_pos + 1, current_pos,
                                 current_pos + 1, line, TokenType::ERR,
                                 ErrType{error_msg}});
      remaining.remove_prefix(1);
      current_pos += 1;
      absolute_pos += 1;
    }
  }

  if (!token_list.empty()) {
    return Tokens{token_list};
  }

  return std::nullopt;
}

std::optional<Tokens> scan_file(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    return std::nullopt;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  return scan_all(content);
}

namespace Test_Fn {
void print_tokens(const Tokens& tokens, const std::string& source) {
  for (const auto& token : tokens.tokens) {
    printf("line: %u | %u - %u | ", token.line, token.begin + 1, token.end);

    std::string token_text = source.substr(
        token.absolute_begin, token.absolute_end - token.absolute_begin);

    switch (token.type) {
      case TokenType::OP:
        printf("OP | %s | %u\n", token_text.c_str(),
               static_cast<uint8_t>(std::get<OpType>(token.value).value));
        break;
      case TokenType::REGISTER:
        printf("REGISTER | %s | %u\n", token_text.c_str(),
               std::get<RegisterType>(token.value).value);
        break;
      case TokenType::INT32:
        printf("INT32 | %s | %d\n", token_text.c_str(),
               std::get<Int32Type>(token.value).value);
        break;
      case TokenType::FLOAT32:
        printf("FLOAT32 | %s | %f\n", token_text.c_str(),
               std::get<Float32Type>(token.value).value);
        break;
      case TokenType::STRING:
        printf("STRING | %s | %s\n", token_text.c_str(),
               std::get<StringType>(token.value).value.c_str());
        break;
      case TokenType::ERR:
        printf("ERR | %s | %s\n", token_text.c_str(),
               std::get<ErrType>(token.value).msg.c_str());
        break;
    }
  }
}

void test(const std::string& filename) {
  auto tokens_opt = TPV::scan_file(filename);
  if (tokens_opt) {
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    print_tokens(*tokens_opt, content);
  } else {
    std::cerr << "Failed to scan file" << std::endl;
  }
}
}  // namespace Test_Fn

}  // namespace TPV
