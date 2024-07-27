#ifndef SCANNER_HPP
#define SCANNER_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "../instructions.hpp"

namespace TPV {

enum class TokenType { OP, REGISTER, INT32, FLOAT32, STRING, ERR };
enum class StringFormat { ASCII, UTF8 };

struct OpType {
  Opcode value;
};

struct RegisterType {
  uint8_t value;
};

struct Int32Type {
  int32_t value;
};

struct Float32Type {
  float value;
};

struct StringType {
  StringFormat type;
  std::string value;
};

struct ErrType {
  std::string msg;
};

struct Token {
  uint32_t absolute_begin, absolute_end;
  uint32_t begin, end;
  uint32_t line;
  TokenType type;
  std::
      variant<OpType, RegisterType, Int32Type, Float32Type, StringType, ErrType>
          value;
};

struct Tokens {
  std::vector<Token> tokens;
};

std::optional<Token> scan_register(std::string_view str, uint32_t start_pos);
std::optional<Token> scan_int(std::string_view str, uint32_t start_pos);
std::optional<Token> scan_float(std::string_view str, uint32_t start_pos);
std::optional<Token> scan_opcode(std::string_view str, uint32_t start_pos);
std::optional<Token> scan_string(std::string_view str,
                                 uint32_t start_pos,
                                 uint32_t absolute_pos,
                                 uint32_t line);
std::optional<Tokens> scan_all(std::string_view str);
std::optional<Tokens> scan_file(const std::string& filename);

namespace Test_Fn {
void print_tokens(const Tokens& tokens);
void test(const std::string& filename);
}  // namespace Test_Fn

}  // namespace TPV

#endif  // SCANNER_HPP
