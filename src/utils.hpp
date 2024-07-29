#ifndef UTILS_HPP
#define UTILS_HPP

#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <type_traits>
#include <vector>

namespace TPV {

template <typename T>
auto to_integral(T e) {
  return static_cast<std::underlying_type_t<T>>(e);
}

inline std::vector<uint8_t> int32_to_bytes(int32_t value) {
  std::vector<uint8_t> bytes(sizeof(int32_t));
  for (size_t i = 0; i < sizeof(int32_t); ++i) {
    bytes[sizeof(int32_t) - 1 - i] = (value >> (i * 8)) & 0xFF;
  }
  return bytes;
}

inline std::vector<uint8_t> float32_to_bytes(float_t val) {
  uint32_t value = 0;
  memcpy(&value, &val, 4);

  std::vector<uint8_t> bytes(sizeof(int32_t));

  for (size_t i = 0; i < sizeof(int32_t); ++i) {
    bytes[sizeof(int32_t) - 1 - i] = (value >> (i * 8)) & 0xFF;
  }

  return bytes;
}

inline int32_t bytes_to_int32(const std::vector<uint8_t>& bytes) {
  int32_t value = 0;
  for (size_t i = 0; i < bytes.size(); ++i) {
    value |= (bytes[i] << (8 * (3 - i)));
  }
  return value;
}

inline float_t bytes_to_float32(const std::vector<uint8_t>& bytes) {
  int32_t value = 0;
  for (size_t i = 0; i < bytes.size(); ++i) {
    value |= (bytes[i] << (8 * (3 - i)));
  }

  float_t result;
  memcpy(&result, &value, 4);

  return result;
}

inline std::string bytes_to_string(const std::vector<uint8_t>& bytes) {
  std::string result;
  for (uint8_t byte : bytes) {
    if (byte == '\0') {
      break;
    }
    result += static_cast<char>(byte);
  }
  return result;
}

inline std::size_t hash_string(const std::string& str) {
  return std::hash<std::string>{}(str);
}

}  // namespace TPV

#endif  // !UTILS_HPP