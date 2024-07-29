#ifndef PARSER_HPP
#define PARSER_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "../scanner/scanner.hpp"

namespace TPV {
struct Instruction {
  OpType op_val;
  std::optional<RegisterType> rd, r1, r2;
  std::optional<Int32Type> int_val;
  std::optional<Float32Type> float_val;
  std::optional<StringType> str_val;
  std::optional<LabelRefType> label_ref;
};

struct Parser_Result {
  std::vector<uint8_t> bytecodes;
  std::vector<std::string> err_msg;
};

class Parser {
 public:
  Parser();
  void load_tokens(const Tokens& src);
  Parser_Result parse();
  void parse_to_file();
  void print_bytecodes() const;
  ~Parser();

 private:
  Tokens tokens;
  uint32_t offset;
  std::vector<std::string> err_msg;
  std::vector<uint8_t> bytecodes;
  std::unordered_map<std::string, uint32_t> labels;
  std::vector<Instruction> instructions;

  Token next_token();
  void first_pass();
  void second_pass();
  void emit_byte(uint8_t byte);
  void emit_word(uint32_t word);
};

}  // namespace TPV

#endif  // PARSER_HPP
