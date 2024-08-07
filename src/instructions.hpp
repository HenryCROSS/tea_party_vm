#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <cstdint>

namespace TPV {

enum class Opcode : uint8_t {
  LOADI, // rd, 32bit imm
  LOADF, // rd, 32bit imm
  LOADS, // rd, 32bit imm
  LOADNIL, // rd
  STORES, // imm, string
  ADD,  // rd, r1, r2
  SUB,  // rd, r1, r2
  MUL,  // rd, r1, r2
  DIV,  // rd, r1, r2
  CVT_I_D,  // frd, r1
  CVT_D_I,  // rd, fr1
  NEGATE,   //rd, r1
  HLT,
  JMP,   // 32bit imm | @label
  JMP_IF, // r1, 32bit imm | @label
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

  // for IO
  VMCALL, // r1, r2, imm

  // set register to variable
  PUSH, // r1
  POP, // rd
  SET_GLOBAL, // r1, imm
  GET_GLOBAL, // rd, imm
  SET_CONSTANT, // r1, imm
  GET_CONSTANT, // rd, imm

  SET_UPVAL, // r1, imm
  GET_UPVAL, // rd, imm

  GET_LEN, // rd, r1

  SET_ARG, // rd, imm
  CALL, // rd | @label
  RETURN, //
  CLOSURE, // Not Sure ????

  NEW_LIST,
  SET_LIST, // 
  GET_LIST, // rd, imm

  NEW_TABEL,
  SET_TABLE, // r1, imm
  GET_TABLE, // rd

  NEW_ARRAY,
  SET_ARRAY,
  GET_ARRAY,

  IGL,
  NOP,
};



}  // namespace TPV

#endif  // !INSTRUCTION_H