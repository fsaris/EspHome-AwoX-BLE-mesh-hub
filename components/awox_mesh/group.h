#pragma once

#ifdef USE_ESP32

#include <string>
#include "device.h"
#include "helpers.h"
#include "mesh_destination.h"
#include <vector>

#include <esp_bt_defs.h>

namespace esphome {
namespace awox_mesh {

class Device;

class Group : public MeshDestination {
  std::vector<Device *> devices_{};

 public:
  int group_id;

  uint32_t last_online = 0;

  int dest() override { return this->group_id + 0x8000; }

  const char *type() const override { return "group"; }

  bool can_publish_state() override { return this->device_info != nullptr; };

  std::vector<Group *> get_groups() const override { return std::vector<Group *>{}; };

  std::string state_as_string();

  std::vector<Device *> get_devices();

  void add_device(Device *device);
};

}  // namespace awox_mesh
}  // namespace esphome

#endif
