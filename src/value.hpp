#ifndef VALUE_HPP
#define VALUE_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include "common.hpp"
namespace TPV {

enum class ObjType {
  FUNCTION,
  CLOSURE,
  MODULE,
  STRING,
  UPVALUE,
  FOREIGN,
  SUM,
  PRODUCT,
  LIST,
  MAP,
  ARRAY,
};

struct TPV_ObjString {
  int32_t hash;
  std::string value;
};

struct TPV_ObjFunc {};

struct TPV_ObjClosure {};

struct TPV_ObjModule {};

struct TPV_ObjUpvalue {};

struct TPV_ObjSum {};

struct TPV_ObjProduct {};
struct TPV_ObjList {};
struct TPV_ObjMap {};
struct TPV_ObjArray {};

struct TPV_Obj {
  ObjType type;
  std::variant<TPV_ObjString,
               TPV_ObjClosure,
               TPV_ObjModule,
               TPV_ObjFunc,
               TPV_ObjUpvalue,
               TPV_ObjSum,
               TPV_ObjProduct>
      obj;
};

/*
TPV_INT :: int32_t
TPV_FLOAT :: float_t
BOOL :: bool
UNIT :: ()
*/
enum class ValueType { TPV_INT, TPV_FLOAT, BOOL, TPV_OBJ, TPV_UNIT };

struct Value {
  ValueType type;
  bool is_const;
  std::variant<std::shared_ptr<TPV_Obj>, TPV_INT, TPV_FLOAT, bool> value;
};

}  // namespace TPV

#endif  // !VALUE_HPP