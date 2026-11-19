#pragma once

#include <cstdint>
#include <vector>
#include "esphome/core/hal.h"

namespace esphome {
namespace awox_mesh {

class Group;
class MeshEntity;

struct MeshDestinationState {
  unsigned char state[7];
};

class MeshDestination {
 public:
  bool state = false;
  bool color_mode = false;
  bool sequence_mode = false;
  bool candle_mode = false;
  unsigned char white_brightness = 0;
  unsigned char temperature = 0;
  unsigned char color_brightness = 0;
  unsigned char R = 0;
  unsigned char G = 0;
  unsigned char B = 0;

  bool online = false;
  // Product info received (devices) or device type known (groups)
  bool identified = false;
  // ESPHome entity configured for this device or group, nullptr when not in the YAML
  MeshEntity *entity = nullptr;

  // Set when a command is sent to the mesh (from an API client, the web server or an automation).
  // A group keeps the commanded state until its members report the same, so members that have
  // not reported yet do not flip it back.
  bool command_pending = false;
  uint32_t command_time = 0;

  void command_sent() {
    this->command_pending = true;
    this->command_time = millis();
  }

  virtual int dest();
  virtual const char *type() const;
  virtual bool can_publish_state();
  virtual std::vector<Group *> get_groups() const;
  virtual std::string state_as_string();

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
    if (this->online)
      state.state[0] += 16;

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
