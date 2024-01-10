#pragma once

#include <string>
#include "device_info.h"
#include "helpers.h"

#ifdef USE_ESP32

#include <esp_bt_defs.h>

namespace esphome {
namespace awox_mesh {

class Device {
  esp_bd_addr_t address_{
      0,
  };

 public:
  int mesh_id;
  int product_id;
  bool send_discovery = false;
  uint32_t last_online = 0;
  uint32_t device_info_requested = 0;
  bool online;
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

  std::string address_str() const {
    char mac[24];
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X", this->address_[0], this->address_[1], this->address_[2],
             this->address_[3], this->address_[4], this->address_[5]);
    return mac;
  }

  std::string address_str_hex_only() const {
    char mac[19];
    snprintf(mac, sizeof(mac), "%02X%02X%02X%02X%02X%02X", this->address_[0], this->address_[1], this->address_[2],
             this->address_[3], this->address_[4], this->address_[5]);
    return mac;
  }

  uint64_t address_uint64() const { return esp32_ble::ble_addr_to_uint64(this->address_); }

  void set_address(unsigned char part3, unsigned char part4, unsigned char part5, unsigned char part6) {
    this->address_[0] = 0xA4;
    this->address_[1] = 0xC1;
    this->address_[2] = part3;
    this->address_[3] = part4;
    this->address_[4] = part5;
    this->address_[5] = part6;
  }

  bool address_set() { return this->address_[0] > 0; }

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

#endif
