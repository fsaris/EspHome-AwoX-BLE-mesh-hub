#pragma once

#ifdef USE_ESP32

#include <map>
#include <vector>

#include <AES.h>
#include <Crypto.h>
#include <GCM.h>

#include "esphome/core/hal.h"
#include "esphome/components/esp32_ble_client/ble_client_base.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/defines.h"

#include "mesh_device.h"

namespace esphome {
namespace awox_mesh {

using namespace esp32_ble_client;

struct FoundDevice {
  std::string address_str;
  uint64_t address{0};
  int rssi{0};
  uint32_t last_detected;
  esp32_ble_tracker::ESPBTDevice device;
};

class AwoxMesh : public esp32_ble_tracker::ESPBTDeviceListener, public Component {
  uint32_t start;
  FoundDevice add_to_devices(const esp32_ble_tracker::ESPBTDevice &device);
  void sort_devices();
  void remove_devices_that_are_not_available();

 public:
  void setup() override;

  AwoxMesh() { this->start = esphome::millis(); }
  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;

  void on_scan_end() override { ESP_LOGD("AwoxMesh", "scan end"); }

  void register_connection(MeshDevice *connection) {
    ESP_LOGD("AwoxMesh", "register_connection");
    this->connection = connection;
  }
  void set_address_prefix(const std::string &address_prefix) {
    ESP_LOGI("AwoxMesh", "address_prefix: %s", address_prefix.c_str());
    this->address_prefix = address_prefix;
  }

  void loop() override;

 protected:
  MeshDevice *connection;
  std::vector<FoundDevice> devices_{};
  std::string address_prefix = "A4:C1";
};

}  // namespace awox_mesh
}  // namespace esphome

#endif
