#ifdef USE_ESP32

#include "group.h"
#include "device.h"
#include <string>
#include <vector>
#include <algorithm>

namespace esphome {
namespace awox_mesh {

std::string Group::state_as_string() {
  std::string output = "group ";

  output += std::to_string(this->group_id);
  output += ": ";
  output += "(";
  output += std::to_string(this->dest());
  output += ")";
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

std::vector<Device *> Group::get_devices() { return this->devices_; }

void Group::add_device(Device *device) {
  auto found = std::find_if(this->devices_.begin(), this->devices_.end(),
                            [device](const Device *_f) { return _f->mesh_id == device->mesh_id; });

  if (found == devices_.end()) {
    this->devices_.push_back(device);
  }
}

}  // namespace awox_mesh
}  // namespace esphome

#endif
