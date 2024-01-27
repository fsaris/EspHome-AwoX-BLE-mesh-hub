#pragma once

#ifdef USE_ESP32

#include <string>
#include <vector>
#include "device_info.h"
#include "helpers.h"
#include <esp_bt_defs.h>
#include "esphome/components/esp32_ble/ble.h"

namespace esphome {
namespace awox_mesh {

using namespace esp32_ble;

class Group;

class Device {
  esp_bd_addr_t address_{
      0,
  };
  std::vector<Group *> groups_{};

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
  bool sequence_mode = false;
  bool candle_mode = false;
  unsigned char white_brightness;
  unsigned char temperature;
  unsigned char color_brightness;
  unsigned char R;
  unsigned char G;
  unsigned char B;

  std::string address_str() const;

  std::string address_str_hex_only() const;

  uint64_t address_uint64() const;

  void set_address(unsigned char part3, unsigned char part4, unsigned char part5, unsigned char part6);

  bool address_set();

  std::string device_state_as_string();

  void add_group(Group *group);

  std::vector<Group *> get_groups() const;
};

}  // namespace awox_mesh
}  // namespace esphome

#endif
