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

static std::string TextToBinaryString(std::string words) {
  std::string binaryString = "";
  for (char &_char : words) {
    binaryString += std::bitset<8>(_char).to_string();
  }
  return binaryString;
}

struct Device {
  int mesh_id;
  bool send_discovery = false;
  uint32_t last_online = 0;
  uint32_t device_info_requested = 0;
  bool online;

  std::string mac = "";

  DeviceInfo *device_info;

  bool state = false;
  bool color_mode = false;
  bool transition_mode = false;
  unsigned char white_brightness;
  unsigned char temperature;
  unsigned char color_brightness;
  unsigned char R;
  unsigned char G;
  unsigned char B;
};

struct PublishOnlineStatus {
  Device *device;
  bool online;
  uint32_t time;
};

struct QueuedCommand {
  int command;
  std::string data;
  int dest;
};

class MeshDevice : public esp32_ble_client::BLEClientBase {
  /**
   * Packet counter used to tag transmitted packets.
   */
  int packet_count = 1;
  uint32_t last_send_command = 0;

  DeviceInfoResolver *device_info_resolver = new DeviceInfoResolver();

  std::vector<Device *> devices_{};
  std::deque<PublishOnlineStatus> delayed_availability_publish{};
  std::deque<QueuedCommand> command_queue{};

  std::function<void()> disconnect_callback;

  std::string mesh_name = "";
  std::string mesh_password = "";
  std::string random_key;
  std::string session_key;

  std::string reverse_address;

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

  Device *get_device(int dest);

  std::string device_state_as_string(Device *device);

  std::string get_discovery_topic_(const esphome::mqtt::MQTTDiscoveryInfo &discovery_info, Device *device) const;

  std::string get_mqtt_topic_for_(Device *device, const std::string &suffix) const;

  void send_discovery(Device *device);

  void publish_state(Device *device);

  void publish_availability(Device *device, bool delayed);

  void publish_connected(bool connected);

  void process_incomming_command(Device *device, JsonObject root);

  void queue_command(int command, const std::string &data, int dest = 0);

  virtual void set_state(esp32_ble_tracker::ClientState st) override {
    this->state_ = st;
    switch (st) {
      case esp32_ble_tracker::ClientState::INIT:

        ESP_LOGI("MeshDevice", "INIT");
        break;
      case esp32_ble_tracker::ClientState::DISCONNECTING:

        ESP_LOGI("MeshDevice", "DISCONNECTING");
        break;
      case esp32_ble_tracker::ClientState::IDLE:

        ESP_LOGI("MeshDevice", "IDLE");
        break;
      case esp32_ble_tracker::ClientState::SEARCHING:

        ESP_LOGI("MeshDevice", "SEARCHING");
        break;
      case esp32_ble_tracker::ClientState::DISCOVERED:

        ESP_LOGI("MeshDevice", "DISCOVERED");
        break;
      case esp32_ble_tracker::ClientState::READY_TO_CONNECT:

        ESP_LOGI("MeshDevice", "READY_TO_CONNECT");
        break;
      case esp32_ble_tracker::ClientState::CONNECTING:

        ESP_LOGI("MeshDevice", "CONNECTING");
        break;
      case esp32_ble_tracker::ClientState::CONNECTED:

        ESP_LOGI("MeshDevice", "CONNECTED");
        break;
      case esp32_ble_tracker::ClientState::ESTABLISHED:

        ESP_LOGI("MeshDevice", "ESTABLISHED");
        break;

      default:
        ESP_LOGI("MeshDevice", "Unknown state");
        break;
    }
  }

 public:
  void set_mesh_name(const std::string &mesh_name) {
    ESP_LOGI("MeshDevice", "name: %s", mesh_name.c_str());
    this->mesh_name = mesh_name;
  }
  void set_mesh_password(const std::string &mesh_password) {
    ESP_LOGI("MeshDevice", "password: %s", mesh_password.c_str());
    this->mesh_password = mesh_password;
  }

  void loop() override;

  void on_shutdown() override;

  bool gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;

  void set_address(uint64_t address) {

    this->publish_connected(false);
    BLEClientBase::set_address(address);

    if (address == 0) {
      this->reverse_address = "";
    } else {
      unsigned char buf[6];
      buf[0] = (address >> 0) & 0xff;
      buf[1] = (address >> 8) & 0xff;
      buf[2] = (address >> 16) & 0xff;
      buf[3] = (address >> 24) & 0xff;
      buf[4] = (address >> 32) & 0xff;
      buf[5] = (address >> 40) & 0xff;

      this->reverse_address = std::string((char *) buf, 6);
    }
  };

  void set_disconnect_callback(std::function<void()> &&f);

  bool write_command(int command, const std::string &data, int dest = 0, bool withResponse = false);

  void request_status();

  bool set_state(int dest, bool state);

  bool set_color(int dest, int red, int green, int blue);

  bool set_color_brightness(int dest, int brightness);

  bool set_white_brightness(int dest, int brightness);

  bool set_white_temperature(int dest, int temp);

  bool request_device_info(Device *device);

  bool request_device_version(int dest);
};

}  // namespace awox_mesh
}  // namespace esphome

#endif  // USE_ESP32
