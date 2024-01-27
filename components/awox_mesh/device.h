#pragma once

#ifdef USE_ESP32

#include <string>
#include <vector>
#include "helpers.h"
#include "mesh_destination.h"
#include <esp_bt_defs.h>
#include "esphome/components/esp32_ble/ble.h"

namespace esphome {
namespace awox_mesh {

using namespace esp32_ble;

class Group;

class Device : public MeshDestination {
  esp_bd_addr_t address_{
      0,
  };
  std::vector<Group *> groups_{};

 public:
  int mesh_id;

  int product_id;

  uint32_t last_online = 0;

  uint32_t device_info_requested = 0;

  int dest() override { return this->mesh_id; };

  const char *type() const override { return "device"; }

  bool can_publish_state() override { return !!this->address_set(); };

  std::string address_str() const;

  std::string address_str_hex_only() const;

  uint64_t address_uint64() const;

  void set_address(unsigned char part3, unsigned char part4, unsigned char part5, unsigned char part6);

  bool address_set();

  std::string device_state_as_string();

  void add_group(Group *group);

  std::vector<Group *> get_groups() const override;
};

}  // namespace awox_mesh
}  // namespace esphome

#endif
