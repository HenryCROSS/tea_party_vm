#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <cstdint>

namespace TPV {

enum class Opcode : uint8_t {
  LOADI, // rd, 32bit imm
  LOADF, // rd, 32bit imm
  LOADS, // rd, 32bit imm
  STORES, // imm, string
  ADD,  // rd, r1, r2
  SUB,  // rd, r1, r2
  MUL,  // rd, r1, r2
  DIV,  // rd, r1, r2
  CVT_I_D,  // frd, r1
  CVT_D_I,  // rd, fr1
  NEGATE,   //rd, r1
  HLT,
  JMP,   // 32bit
  EQ,    // rd, r1, r2
  NEQ,   // rd, r1, r2
  GT,    // rd, r1, r2
  GTE,   // rd, r1, r2
  LT,    // rd, r1, r2
  LTE,   // rd, r1, r2

  BITAND, // rd, r1, r2
  BITOR,  // rd, r1, r2
  BITXOR, // rd, r1, r2
  BITNOT, // rd, r1
  BITSHL, // rd, r1, imm
  BITSHRL, // rd, r1, imm
  BITSHRA, // rd, r1, imm

  // set register to variable
  SET_LOCAL, 
  GET_LOCAL,
  SET_GLOBAL,
  GET_GLOBAL,
  SET_CONSTANT, // r1

  CALL,
  RETURN,
  CLOSURE,

  SET_SUM,
  GET_SUM,

  SET_LIST,
  GET_LIST,

  SET_TABLE,
  GET_TABLE,

  SET_ARRAY,
  GET_ARRAY,

  IGL,
  NOP,
};



}  // namespace TPV

#endif  // !INSTRUCTION_H