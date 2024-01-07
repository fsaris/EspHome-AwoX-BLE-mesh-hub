#pragma once

#ifdef USE_ESP32
#include <cstring>
#include <bitset>
#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/log.h"
#include "esphome/components/esp32_ble_client/ble_client_base.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/mqtt/mqtt_client.h"
#include "device_info.h"
#include "device.h"

namespace esphome {
namespace awox_mesh {

using namespace esphome::mqtt;

/** UUID for Bluetooth GATT information service */
static std::string uuid_info_service = "00010203-0405-0607-0809-0a0b0c0d1910";
/** UUID for Bluetooth GATT notification characteristic */
static std::string uuid_notification_char = "00010203-0405-0607-0809-0a0b0c0d1911";
/** UUID for Bluetooth GATT command characteristic */
static std::string uuid_command_char = "00010203-0405-0607-0809-0a0b0c0d1912";
/** UUID for Bluetooth GATT pairing characteristic */
static std::string uuid_pair_char = "00010203-0405-0607-0809-0a0b0c0d1914";

#define COMMAND_ONLINE_STATUS_REPORT 0xDC
#define COMMAND_STATUS_REPORT 0xDB

#define C_REQUEST_STATUS 0xda
#define C_POWER 0xd0
#define C_COLOR 0xe2
#define C_COLOR_BRIGHTNESS 0xf2
#define C_WHITE_BRIGHTNESS 0xf1
#define C_WHITE_TEMPERATURE 0xf0
#define COMMAND_ADDRESS 0xE0
#define COMMAND_ADDRESS_REPORT 0xE1
#define COMMAND_DEVICE_INFO_QUERY 0xEA
#define COMMAND_DEVICE_INFO_REPORT 0xEB

struct QueuedCommand {
  int command;
  std::string data;
  int dest;
};

struct FoundDevice;
class AwoxMesh;

class MeshConnection : public esp32_ble_client::BLEClientBase {
  /**
   * Packet counter used to tag transmitted packets.
   */
  int packet_count = 1;
  uint32_t last_send_command = 0;

  std::deque<QueuedCommand> command_queue{};

  std::function<void()> disconnect_callback;

  std::string random_key;
  std::string session_key;

  std::string reverse_address;

  FoundDevice *found_device;

  esp32_ble_client::BLECharacteristic *notification_char;
  esp32_ble_client::BLECharacteristic *command_char;
  esp32_ble_client::BLECharacteristic *pair_char;

  void setup_connection();

  std::string combine_name_and_password() const;

  void generate_session_key(const std::string &data1, const std::string &data2);

  std::string key_encrypt(std::string &key) const;

  std::string encrypt_packet(std::string &packet) const;

  std::string decrypt_packet(std::string &packet) const;

  std::string build_packet(int dest, int command, const std::string &data);

  void handle_packet(std::string &packet);

  void queue_command(int command, const std::string &data, int dest = 0);

  void add_mesh_id(int mesh_id);
  void remove_mesh_id(int mesh_id);

  virtual void set_state(esp32_ble_tracker::ClientState st) override {
    this->state_ = st;
    switch (st) {
      case esp32_ble_tracker::ClientState::INIT:

        ESP_LOGI("awox.connection", "INIT");
        break;
      case esp32_ble_tracker::ClientState::DISCONNECTING:

        ESP_LOGI("awox.connection", "DISCONNECTING");
        break;
      case esp32_ble_tracker::ClientState::IDLE:

        ESP_LOGI("awox.connection", "IDLE");
        break;
      case esp32_ble_tracker::ClientState::SEARCHING:

        ESP_LOGI("awox.connection", "SEARCHING");
        break;
      case esp32_ble_tracker::ClientState::DISCOVERED:

        ESP_LOGI("awox.connection", "DISCOVERED");
        break;
      case esp32_ble_tracker::ClientState::READY_TO_CONNECT:

        ESP_LOGI("awox.connection", "READY_TO_CONNECT");
        break;
      case esp32_ble_tracker::ClientState::CONNECTING:

        ESP_LOGI("awox.connection", "CONNECTING");
        break;
      case esp32_ble_tracker::ClientState::CONNECTED:

        ESP_LOGI("awox.connection", "CONNECTED");
        break;
      case esp32_ble_tracker::ClientState::ESTABLISHED:

        ESP_LOGI("awox.connection", "ESTABLISHED");
        break;

      default:
        ESP_LOGI("awox.connection", "Unknown state");
        break;
    }
  }

 public:
  void loop() override;

  bool gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;

  void set_address(uint64_t address);

  void set_disconnect_callback(std::function<void()> &&f);

  bool write_command(int command, const std::string &data, int dest = 0, bool withResponse = false);

  void request_status();

  void set_power(int dest, bool state);

  void set_color(int dest, int red, int green, int blue);

  void set_color_brightness(int dest, int brightness);

  void set_white_brightness(int dest, int brightness);

  void set_white_temperature(int dest, int temp);

  void request_status_update(int dest);

  void request_device_info(Device *device);

  bool request_device_version(int dest);

  void connect_to(FoundDevice *found_device);

  bool mesh_id_linked(int mesh_id);

  const std::vector<int> &get_linked_mesh_ids() const { return this->linked_mesh_ids_; }

 protected:
  friend class AwoxMesh;

  std::vector<int> linked_mesh_ids_;

  std::string mesh_name = "";
  std::string mesh_password = "";
  AwoxMesh *mesh_;
};

}  // namespace awox_mesh
}  // namespace esphome

#endif  // USE_ESP32
