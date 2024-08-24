#ifndef VALUE_HPP
#define VALUE_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include "common.hpp"
namespace TPV {

struct Value;

enum class ObjType {
  CLOSURE,
  MODULE,
  STRING,
  UPVALUE,
  FOREIGN,
  LIST,
  MAP,
  ARRAY,
  TABLE,
  UNIT
};

struct TPV_Unit {};

struct TPV_ObjString {
  size_t hash;
  std::string value;
};

struct TPV_ObjClosure {};

struct TPV_ObjModule {};

struct TPV_ObjUpvalue {};

struct TPV_ObjList {};
struct TPV_ObjMap {};
struct TPV_ObjArray {};
struct TPV_ObjTable {
  size_t hash;
  std::unordered_map<size_t, Value> tbl;
};

struct TPV_Obj {
  ObjType type;
  std::variant<std::shared_ptr<TPV_ObjString>,
               std::shared_ptr<TPV_ObjClosure>,
               std::shared_ptr<TPV_ObjModule>,
               std::shared_ptr<TPV_ObjTable>,
               std::shared_ptr<TPV_ObjUpvalue>>
      obj;
};

/*
TPV_INT :: int32_t
TPV_FLOAT :: float_t
BOOL :: bool
UNIT :: ()
*/
enum class ValueType { TPV_INT, TPV_FLOAT, TPV_OBJ, TPV_UNIT };

struct Value {
  ValueType type;
  bool is_const;
  std::variant<TPV_INT, TPV_FLOAT, TPV_Obj, TPV_Unit> value;
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

inline Value from_obj_value(std::shared_ptr<TPV_ObjString> val) {
  return {
      .type = ValueType::TPV_OBJ,
      .is_const = false,
      .value = (TPV_Obj){.type = ObjType::STRING, .obj = val},
  };
}

inline TPV_INT get_int32(Value val) {
  return std::get<TPV_INT>(val.value);
}

inline TPV_FLOAT get_float32(Value val) {
  return std::get<TPV_FLOAT>(val.value);
}

inline TPV_ObjString get_str(Value val) {
  auto& obj = std::get<TPV_Obj>(val.value);
  return *std::get<std::shared_ptr<TPV_ObjString>>(obj.obj);
}

}  // namespace TPV

#endif  // !VALUE_HPP