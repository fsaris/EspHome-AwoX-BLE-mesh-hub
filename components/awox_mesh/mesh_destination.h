#pragma once

#include <vector>
#include "device_info.h"

namespace esphome {
namespace awox_mesh {

class Group;

class MeshDestination {
 public:
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

  bool online;
  bool send_discovery = false;
  DeviceInfo *device_info;

  virtual int dest();
  virtual const char *type() const;
  virtual bool can_publish_state();
  virtual std::vector<Group *> get_groups() const;
};

}  // namespace awox_mesh
}  // namespace esphome
