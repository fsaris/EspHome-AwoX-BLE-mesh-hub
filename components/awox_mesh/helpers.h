#pragma once

#include <string>
#include <bitset>

namespace esphome {
namespace awox_mesh {

static std::string string_as_binary_string(std::string words) {
  std::string binary_string = "";
  for (char &_char : words) {
    binary_string += std::bitset<8>(_char).to_string();
  }
  return binary_string;
}

static std::string string_as_hex_string(std::string words) {
  std::string hex_string = "";
  for (char &_char : words) {
    char value[3];
    sprintf(value, "%02X", _char);
    hex_string += value;
    if (hex_string.size() > 2) {
      hex_string += ":";
    }
  }
  return hex_string;
}

static std::string int_as_hex_string(unsigned char hex1, unsigned char hex2, unsigned char hex3) {
  char value[7];
  sprintf(value, "%02X%02X%02X", hex1, hex2, hex3);
  return std::string((char *) value, 6);
}

}  // namespace awox_mesh
}  // namespace esphome
