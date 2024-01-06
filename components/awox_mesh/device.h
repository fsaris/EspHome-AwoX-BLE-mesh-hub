#pragma once

#include <string>
#include "device_info.h"
#include "helpers.h"

namespace esphome {
namespace awox_mesh {

class Device {
 public:
  int mesh_id;
  int product_id;
  bool send_discovery = false;
  uint32_t last_online = 0;
  uint32_t device_info_requested = 0;
  bool online;

  std::string mac = "";

  DeviceInfo *device_info;

  bool state = false;
  bool color_mode = false;
  bool transition_mode = false;
  unsigned char white_brightness;
  unsigned char temperature;
  unsigned char color_brightness;
  unsigned char R;
  unsigned char G;
  unsigned char B;

  std::string device_state_as_string() {
    std::string output = "";

    output += std::to_string(this->mesh_id);
    output += ": ";
    output += this->state ? "ON" : "OFF";

    output += " ";

    if (this->color_mode) {
      output += "#" + int_as_hex_string(this->R, this->G, this->B);
      output += " (";
      output += std::to_string(this->color_brightness);
      output += " %%)";
    } else {
      output += "temp: " + std::to_string(this->temperature);
      output += " (";
      output += std::to_string(this->white_brightness);
      output += " %%)";
    }
    output += this->online ? " ONLINE" : " OFFLINE!!";

    return output;
  }
};

}  // namespace awox_mesh
}  // namespace esphome
