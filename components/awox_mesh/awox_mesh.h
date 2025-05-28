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

#include "awox_mesh_mqtt.h"
#include "mesh_destination.h"
#include "mesh_connection.h"
#include "device.h"
#include "device_info.h"
#include "group.h"

namespace esphome {
namespace awox_mesh {

using namespace esp32_ble_client;

struct PublishOnlineStatus {
  Device *device;
  bool online;
  uint32_t time;
};

struct FoundDevice {
  int rssi{0};
  uint32_t last_detected;
  esp32_ble_tracker::ESPBTDevice device;
  bool connected = false;
  int mesh_id;
};

class AwoxMesh : public esp32_ble_tracker::ESPBTDeviceListener, public Component {
  uint32_t start;
  uint32_t device_info_request_interval_ms = 5000;
  uint32_t delayed_availability_publish_debounce_time_ms = 6000;


  bool ready_to_connect = false;

  bool has_active_connection = false;

  uint32_t last_connection_attempt = 0;

  uint32_t last_found_device_cleanup = 0;

  int minimum_rssi = -90;

  std::string mesh_name = "";

  std::string mesh_password = "";

  std::string address_prefix = "A4:C1";

  AwoxMeshMqtt *publish_connection;

  DeviceInfoResolver *device_info_resolver = new DeviceInfoResolver();

  std::deque<PublishOnlineStatus> delayed_availability_publish{};

  bool start_up_delay_done();

  FoundDevice *add_to_found_devices(const esp32_ble_tracker::ESPBTDevice &device);

  FoundDevice *next_to_connect();

  void sort_devices();

  void set_rssi_for_devices_that_are_not_available();

  void call_connection(int dest, std::function<void(MeshConnection *)> &&callback);

  void disconnect_connections_with_overlapping_mesh_ids();

  void disconnect_connection_with_overlapping_mesh_ids(int a, int b);

  bool mesh_id_allowed(int mesh_id);

  bool mac_addresses_allowed(const uint64_t address);

  void send_group_discovery(Group *group);

  void sync_and_publish_group_state(Group *group);

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

  void add_allowed_mesh_id(const int mesh_id) { this->allowed_mesh_ids_.push_back(mesh_id); }

  void add_allowed_mac_address(const uint64_t mac_address) { this->allowed_mac_addresses_.push_back(mac_address); }

  float get_setup_priority() const override;

  void setup() override;

  AwoxMesh() {
    this->start = esphome::millis();
    this->publish_connection = new AwoxMeshMqtt(this);
  }

  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;

  void on_scan_end() override { ESP_LOGD("awox.mesh", "scan end"); }

  void register_connection(MeshConnection *connection);

  void set_address_prefix(const std::string &address_prefix) {
    ESP_LOGI("awox.mesh", "address_prefix: %s", address_prefix.c_str());
    this->address_prefix = address_prefix;
  }

  void set_min_rssi(int min_rssi) { this->minimum_rssi = min_rssi; }

  void loop() override;

  Device *get_device(int dest);

  Device *get_device(const uint64_t address);

  Group *get_group(int dest, Device *device);

  void publish_availability(Device *device, bool delayed);

  void send_discovery(Device *device);

  void publish_state(MeshDestination *mesh_destination);

  void publish_connected();

  void set_power(int dest, bool state);
  void set_color(int dest, int red, int green, int blue);
  void set_color_brightness(int dest, int brightness);
  void set_white_brightness(int dest, int brightness);
  void set_white_temperature(int dest, int temp);
  void set_sequence(int dest, int sequence);
  void set_candle_mode(int dest);
  void set_sequence_fade_duration(int dest, int duration);
  void set_sequence_color_duration(int dest, int duration);

 protected:
  std::vector<MeshConnection *> connections_{};
  std::vector<FoundDevice *> found_devices_{};
  std::vector<Device *> mesh_devices_{};
  std::vector<Group *> mesh_groups_{};
  std::vector<int> allowed_mesh_ids_{};
  std::vector<uint64_t> allowed_mac_addresses_{};

  void request_device_info(Device *device);
  void request_status_update(int dest);
};

}  // namespace awox_mesh
}  // namespace esphome

#endif
