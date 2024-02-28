#pragma once

#include <vector>
#include "device_info.h"

namespace esphome {
namespace awox_mesh {

class Group;

struct MeshDestinationState {
  unsigned char state[7];
};

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

  const MeshDestinationState state_as_char() {
    MeshDestinationState state = {};

    state.state[0] = 0;
    if (this->state)
      state.state[0] += 1;
    if (this->color_mode)
      state.state[0] += 2;
    if (this->sequence_mode)
      state.state[0] += 4;
    if (this->candle_mode)
      state.state[0] += 8;

    state.state[1] = this->white_brightness;
    state.state[2] = this->temperature;
    state.state[3] = this->color_brightness;
    state.state[4] = this->R;
    state.state[5] = this->G;
    state.state[6] = this->B;

    return state;
  };
};

}  // namespace awox_mesh
}  // namespace esphome
