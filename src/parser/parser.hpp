#ifndef PARSER_HPP
#define PARSER_HPP

#include <cstdint>
#include <optional>
#include <vector>
#include "../scanner/scanner.hpp"

namespace TPV {
struct Parser_Result {
  std::vector<uint8_t> bytecodes;
  std::vector<std::string> err_msg;
};

class Parser {
 public:
  Parser();
  void load_tokens(const Tokens& src);
  Parser_Result parse();
  ~Parser();

 private:
  Tokens tokens;
  uint32_t offset;
  std::vector<std::string> err_msg;
  std::vector<uint8_t> bytecodes;

  Token next_token();
  void parse_instruction();
  void emit_byte(uint8_t byte);
  void emit_word(uint32_t word);
};

}  // namespace TPV

#endif  // PARSER_HPP
