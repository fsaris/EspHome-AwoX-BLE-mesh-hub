#ifdef USE_ESP32

#include <string>
#include <vector>
#include <algorithm>
#include "device.h"
#include "group.h"
#include "helpers.h"
#include <esp_bt.h>
#include <esp_bt_defs.h>

namespace esphome {
namespace awox_mesh {

std::string Device::address_str() const {
  char mac[24];
  snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X", this->address_[0], this->address_[1], this->address_[2],
           this->address_[3], this->address_[4], this->address_[5]);
  return mac;
}

std::string Device::address_str_hex_only() const {
  char mac[19];
  snprintf(mac, sizeof(mac), "%02X%02X%02X%02X%02X%02X", this->address_[0], this->address_[1], this->address_[2],
           this->address_[3], this->address_[4], this->address_[5]);
  return mac;
}

uint64_t Device::address_uint64() const { return esp32_ble::ble_addr_to_uint64(this->address_); }

void Device::set_address(unsigned char part3, unsigned char part4, unsigned char part5, unsigned char part6) {
  this->address_[0] = 0xA4;
  this->address_[1] = 0xC1;
  this->address_[2] = part3;
  this->address_[3] = part4;
  this->address_[4] = part5;
  this->address_[5] = part6;
}

bool Device::address_set() { return this->address_[0] > 0; }

std::string Device::state_as_string() {
  std::string output = "device ";

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

void Device::add_group(Group *group) {
  auto found = std::find_if(this->groups_.begin(), this->groups_.end(),
                            [group](const Group *_f) { return _f->group_id == group->group_id; });

  if (found == groups_.end()) {
    this->groups_.push_back(group);
  }
}
std::vector<Group *> Device::get_groups() const { return this->groups_; }

}  // namespace awox_mesh
}  // namespace esphome

#endif
