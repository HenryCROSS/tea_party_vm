#ifndef VALUE_HPP
#define VALUE_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include "common.hpp"
namespace TPV {

enum class ObjType {
  CLOSURE,
  MODULE,
  STRING,
  UPVALUE,
  FOREIGN,
  LIST,
  MAP,
  ARRAY,
  UNIT
};

struct TPV_Unit {};

struct TPV_ObjString {
  // size_t hash;
  std::string value;
};

struct TPV_ObjClosure {};

struct TPV_ObjModule {};

struct TPV_ObjUpvalue {};

struct TPV_ObjList {};
struct TPV_ObjMap {};
struct TPV_ObjArray {};

struct TPV_Obj {
  ObjType type;
  std::variant<std::shared_ptr<TPV_ObjString>,
               std::shared_ptr<TPV_ObjClosure>,
               std::shared_ptr<TPV_ObjModule>,
               std::shared_ptr<TPV_ObjUpvalue>>
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
  std::variant<TPV_INT, TPV_FLOAT, bool, TPV_Obj, TPV_Unit> value;
};

inline Value from_raw_value(TPV_INT val) {
  return {
      .type = ValueType::TPV_INT,
      .is_const = false,
      .value = val,
  };
}

inline Value from_raw_value(TPV_FLOAT val) {
  return {
      .type = ValueType::TPV_FLOAT,
      .is_const = false,
      .value = val,
  };
}

}  // namespace TPV

#endif  // !VALUE_HPP