#pragma once

#ifdef USE_ESP32

#include <string>
#include "device.h"
#include "device_info.h"
#include "helpers.h"
#include <vector>

#include <esp_bt_defs.h>

namespace esphome {
namespace awox_mesh {

class Device;

class Group {
  std::vector<Device *> devices_{};

 public:
  int group_id;
  bool send_discovery = false;
  uint32_t last_online = 0;
  DeviceInfo *device_info;

  bool online;

  bool state = false;
  bool color_mode = false;
  bool transition_mode = false;
  unsigned char white_brightness;
  unsigned char temperature;
  unsigned char color_brightness;
  unsigned char R;
  unsigned char G;
  unsigned char B;

  std::string state_as_string();

  std::vector<Device *> get_devices();

  void add_device(Device *device);
};

}  // namespace awox_mesh
}  // namespace esphome

#endif
