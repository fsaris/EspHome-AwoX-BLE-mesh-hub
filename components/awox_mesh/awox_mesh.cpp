#pragma once

#ifdef USE_ESP32
#include <cstring>
#include <cstdio>
#include <algorithm>
#include "awox_mesh.h"

#include "esphome/core/log.h"

namespace esphome {
namespace awox_mesh {

static const char *const TAG = "AwoxMesh";

FoundDevice AwoxMesh::add_to_devices(const esp32_ble_tracker::ESPBTDevice &device) {
  this->devices_.erase(
      std::remove_if(this->devices_.begin(), this->devices_.end(),
                     [device](const FoundDevice &_f) { return _f.address == device.address_uint64(); }),
      this->devices_.end());

  static FoundDevice found = {};
  found.address_str = device.address_str();
  found.address = device.address_uint64();
  found.rssi = device.get_rssi();
  found.last_detected = esphome::millis();
  found.device = device;
  this->devices_.push_back(found);

  this->remove_devices_that_are_not_available();

  this->sort_devices();

  return found;
}

bool AwoxMesh::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  if (device.address_str().rfind(this->address_prefix, 0) != 0) {
    return false;
  }

  FoundDevice found = add_to_devices(device);

  ESP_LOGV(TAG, "Found Awox device %s - %s. RSSI: %d dB (total devices: %d)", device.get_name().c_str(),
           device.address_str().c_str(), device.get_rssi(), this->devices_.size());

  return true;
}

void AwoxMesh::setup() {
  Component::setup();

  this->connection->set_disconnect_callback([this]() { ESP_LOGI(TAG, "disconnected"); });
}

void AwoxMesh::loop() {
  if (esphome::millis() - this->start > 20000 && this->devices_.size() > 0 && this->connection->address_str() == "") {
    ESP_LOGD(TAG, "Total devices: %d", this->devices_.size());
    for (int i = 0; i < this->devices_.size(); i++) {
      ESP_LOGD(TAG, "Available device %s => rssi: %d", this->devices_[i].address_str.c_str(), this->devices_[i].rssi);
    }
    auto device = this->devices_.front();

    ESP_LOGI(TAG, "Try to connect %s => rssi: %d", device.address_str.c_str(), device.rssi);
    this->connection->set_address(device.address);
    this->connection->parse_device(device.device);

    this->set_timeout("connecting", 20000, [this, device]() {
      if (this->connection->connected()) {
        return;
      }
      ESP_LOGI(TAG, "Failed to connect %s => rssi: %d", device.address_str.c_str(), device.rssi);
      this->remove_devices_that_are_not_available();
      this->connection->disconnect();
      this->connection->set_address(0);
    });
  }
}

void AwoxMesh::sort_devices() {
  std::stable_sort(this->devices_.begin(), this->devices_.end(),
                   [](FoundDevice a, FoundDevice b) { return a.rssi > b.rssi; });
}

void AwoxMesh::remove_devices_that_are_not_available() {
  const uint32_t now = esphome::millis();
  this->devices_.erase(std::remove_if(this->devices_.begin(), this->devices_.end(),
                                      [&](const FoundDevice &_f) { return now - _f.last_detected > 20000; }),
                       this->devices_.end());
}

}  // namespace awox_mesh
}  // namespace esphome

#endif
