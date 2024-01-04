#pragma once

#ifdef USE_ESP32

#include <map>
#include <vector>

#include "esphome/core/hal.h"
#include "esphome/components/esp32_ble_client/ble_client_base.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/defines.h"

#include "mesh_connection.h"
#include "device.h"
#include "device_info.h"

namespace esphome {
namespace awox_mesh {

using namespace esp32_ble_client;

struct PublishOnlineStatus {
  Device *device;
  bool online;
  uint32_t time;
};

struct FoundDevice {
  std::string address_str;
  uint64_t address{0};
  int rssi{0};
  uint32_t last_detected;
  esp32_ble_tracker::ESPBTDevice device;
};

class AwoxMesh : public esp32_ble_tracker::ESPBTDeviceListener, public Component {
  uint32_t start;

  std::string mesh_name = "";
  std::string mesh_password = "";
  std::string address_prefix = "A4:C1";

  DeviceInfoResolver *device_info_resolver = new DeviceInfoResolver();

  std::deque<PublishOnlineStatus> delayed_availability_publish{};

  FoundDevice add_to_devices(const esp32_ble_tracker::ESPBTDevice &device);
  void sort_devices();
  void remove_devices_that_are_not_available();
  void process_incomming_command(Device *device, JsonObject root);

  std::string get_mqtt_topic_for_(Device *device, const std::string &suffix) const;
  std::string get_discovery_topic_(const esphome::mqtt::MQTTDiscoveryInfo &discovery_info, Device *device) const;

 public:
  void set_mesh_name(const std::string &mesh_name) {
    ESP_LOGI("awox.mesh", "name: %s", mesh_name.c_str());
    this->mesh_name = mesh_name;
  }
  void set_mesh_password(const std::string &mesh_password) {
    ESP_LOGI("awox.mesh", "password: %s", mesh_password.c_str());
    this->mesh_password = mesh_password;
  }
  void register_device(int device_type, int product_id, const char *name, const char *model, const char *manufacturer,
                       const char *icon) {
    this->device_info_resolver->register_device(device_type, product_id, name, model, manufacturer, icon);
  }
  void setup() override;

  AwoxMesh() { this->start = esphome::millis(); }
  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;

  void on_scan_end() override { ESP_LOGD("AwoxMesh", "scan end"); }

  void register_connection(MeshConnection *connection) {
    ESP_LOGD("AwoxMesh", "register_connection");
    this->connections_.push_back(connection);
    connection->mesh_name = this->mesh_name;
    connection->mesh_password = this->mesh_password;
    connection->mesh_ = this;

    // legacy
    this->connection = connection;
  }
  void set_address_prefix(const std::string &address_prefix) {
    ESP_LOGI("AwoxMesh", "address_prefix: %s", address_prefix.c_str());
    this->address_prefix = address_prefix;
  }

  void loop() override;

  Device *get_device(int dest);

  void publish_availability(Device *device, bool delayed);
  void send_discovery(Device *device);
  void publish_state(Device *device);
  void publish_connected(bool connected);

 protected:
  std::vector<MeshConnection *> connections_{};
  MeshConnection *connection;  // legacy
  std::vector<FoundDevice> devices_{};
  std::vector<Device *> mesh_devices_{};

  void request_device_info(Device *device);
  void set_power(int dest, bool state);
  void set_color(int dest, int red, int green, int blue);
  void set_color_brightness(int dest, int brightness);
  void set_white_brightness(int dest, int brightness);
  void set_white_temperature(int dest, int temp);
  void request_status_update(int dest);
};

}  // namespace awox_mesh
}  // namespace esphome

#endif
