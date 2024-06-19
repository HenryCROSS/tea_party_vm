#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <cstdint>

namespace TPV {

enum class Opcode : int8_t {
  LOAD_I, // rd, 32bit
  LOAD_D, // rd, 32bit
  STORE_I,
  STORE_D,
  ADD_I,  // rd, r1, r2
  ADDI_I,  // rd, r1, imm
  SUB_I,  // rd, r1, r2
  MUL_I,  // rd, r1, r2
  MULI_I,  // rd, r1, r2
  DIV_I,  // rd, r1, r2
  ADD_D,  // frd, fr1, fr2
  ADDI_D,  // frd, fr1, imm
  SUB_D,  // frd, fr1, fr2
  MUL_D,  // frd, fr1, fr2
  DIV_D,  // frd, fr1, fr2
  CVT_I_D,  // frd, r1
  CVT_D_I,  // rd, fr1
  NEGATE_I,   //rd, r1
  NEGATE_D,   //rd, r1
  HLT,
  JMP,   // 32bit
  EQ_I,    // rd, r1, r2
  NEQ_I,   // rd, r1, r2
  GT_I,    // rd, r1, r2
  GTE_I,   // rd, r1, r2
  LT_I,    // rd, r1, r2
  LTE_I,   // rd, r1, r2
  EQ_D,    // rd, fr1, fr2
  NEQ_D,   // rd, fr1, fr2
  GT_D,    // rd, fr1, fr2
  GTE_D,   // rd, fr1, fr2
  LT_D,    // rd, fr1, fr2
  LTE_D,   // rd, fr1, fr2
  CALL,
  RETURN,
  CLOSURE,
  SET_GLOBAL, // r1
  SET_CONSTANT, // r1
  SET_SUM, // number
  SET_PRODUCT, // number
  SET_LIST,
  SET_MAP,
  SET_ARRAY,

  IGL,
  NOP,
};



}  // namespace TPV

#endif  // !INSTRUCTION_H