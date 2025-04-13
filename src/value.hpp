#ifndef VALUE_HPP
#define VALUE_HPP

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include "common.hpp"

namespace TPV {

struct Value;

enum class ObjType { FUNCTION, MODULE, STRING, UPVALUE, FOREIGN, ARRAY, UNIT };

struct TPV_Unit {};

struct TPV_Function {
  std::string name;
  size_t arity;
  std::vector<uint8_t> bytes;
};

struct TPV_ObjString {
  size_t hash;
  std::string value;
};

struct TPV_ObjFunction {
  TPV_Function* func;
  std::unordered_map<std::string, std::shared_ptr<TPV_ObjString>> str_table;
  std::unordered_map<std::string, std::shared_ptr<TPV_ObjFunction>> func_table;
  size_t upvalue_count;
  size_t stack_size;
  uint8_t* stack_frame;
};

struct TPV_ObjModule {};

struct TPV_ObjUpvalue {};

struct TPV_ObjArray {
  std::vector<Value> values;
};

struct TPV_Obj {
  ObjType type;
  std::variant<std::shared_ptr<TPV_ObjString>,
               std::shared_ptr<TPV_ObjArray>,
               std::shared_ptr<TPV_ObjFunction>,
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
enum class ValueType { TPV_INT, TPV_FLOAT, TPV_OBJ, TPV_UNIT };
constexpr std::array<const char*, 4> value_type_names = {"TPV_INT", "TPV_FLOAT",
                                                         "TPV_OBJ", "TPV_UNIT"};

// a function to get type name
inline const char* get_value_type_name(ValueType type) {
  return value_type_names[static_cast<std::underlying_type_t<ValueType>>(type)];
}

struct Value {
  ValueType type;
  bool is_const;

  using ValueObject = std::variant<TPV_INT, TPV_FLOAT, TPV_Obj, TPV_Unit>;
  ValueObject value;
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

template <typename T, typename... Ts>
inline T& get_or_crash(std::variant<Ts...>& v, const char* error_msg) {
  if (auto* ptr = std::get_if<T>(&v)) {
    return *ptr;
  } else {
    std::print(stderr, "Fatal Error: {}\n", error_msg);
    std::print(stderr, "Expected type: {}\n", typeid(T).name());
    std::print(stderr, "Actual type: {}\n", v.index());
    std::abort();
  }
}

inline TPV_INT get_int32(Value val) {
  return get_or_crash<TPV_INT>(val.value, "Expected int32 value");
}

inline TPV_FLOAT get_float32(Value val) {
  return get_or_crash<TPV_FLOAT>(val.value, "Expected float value");
}

inline auto get_str(Value val) {
  auto& obj = get_or_crash<TPV_Obj>(val.value, "Expected Object value");
  return *get_or_crash<std::shared_ptr<TPV_ObjString>>(
      obj.obj, "Expected Object value to be a string");
}

inline auto get_str_ptr(Value val) {
  auto& obj = get_or_crash<TPV_Obj>(val.value, "Expected Object value");
  return get_or_crash<std::shared_ptr<TPV_ObjString>>(
      obj.obj, "Expected Object value to be a string");
}

}  // namespace TPV

#endif  // !VALUE_HPP