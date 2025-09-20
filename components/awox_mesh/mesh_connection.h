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
#define COMMAND_ADDRESS_REPORT 0xD8
#define COMMAND_GROUP_ID_REPORT 0xD4

#define C_REQUEST_STATUS 0xda
#define C_POWER 0xd0
#define C_COLOR 0xe2
#define C_SEQUENCE 0xc8
#define C_CANDLE_MODE 0xc9
#define C_COLOR_BRIGHTNESS 0xf2
#define C_WHITE_BRIGHTNESS 0xf1
#define C_WHITE_TEMPERATURE 0xf0
#define C_SEQUENCE_COLOR_DURATION 0xf5
#define C_SEQUENCE_FADE_DURATION 0xf6
#define COMMAND_ADDRESS 0xE0
#define COMMAND_ADDRESS_REPORT_QUERY 0xE1
#define COMMAND_DEVICE_INFO_QUERY 0xEA
#define COMMAND_DEVICE_INFO_REPORT 0xEB
#define COMMAND_GROUP_ID_QUERY 0xDD

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
  uint32_t command_debounce_time = 180;

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
  void clear_linked_mesh_ids();

  virtual void set_state(esp32_ble_tracker::ClientState st) override {
    esp32_ble_client::BLEClientBase::set_state(st);

    std::string state = "";
    switch (st) {
      case esp32_ble_tracker::ClientState::INIT:
        state = "INIT";
        break;

      case esp32_ble_tracker::ClientState::DISCONNECTING:
        state = "DISCONNECTING";
        break;

      case esp32_ble_tracker::ClientState::IDLE:
        state = "IDLE";
        break;

      case esp32_ble_tracker::ClientState::DISCOVERED:
        state = "DISCOVERED";
        break;

      case esp32_ble_tracker::ClientState::READY_TO_CONNECT:
        state = "READY_TO_CONNECT";
        break;

      case esp32_ble_tracker::ClientState::CONNECTING:
        state = "CONNECTING";
        break;

      case esp32_ble_tracker::ClientState::CONNECTED:
        state = "CONNECTED";
        break;

      case esp32_ble_tracker::ClientState::ESTABLISHED:
        state = "ESTABLISHED";
        break;

      default:
        state = "Unknown state";
        break;
    }

    ESP_LOGI("awox.connection", "[%d] set_state %s", this->connection_index_, state.c_str());
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

  void set_sequence(int dest, int sequence);

  void set_candle_mode(int dest);

  void set_sequence_fade_duration(int dest, int duration);

  void set_sequence_color_duration(int dest, int duration);

  void request_status_update(int dest);

  void request_device_info(Device *device);

  void request_group_info(Device *device);

  bool request_device_version(int dest);

  void connect_to(FoundDevice *found_device);

  int mesh_id();

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
