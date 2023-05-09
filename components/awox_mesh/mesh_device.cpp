#include <cstring>
#include <bitset>
#include <algorithm>

#include <math.h>
#include "mesh_device.h"
#include "device_info.h"
#include <AES.h>
#include <Crypto.h>
#include <GCM.h>
#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/components/mqtt/mqtt_const.h"
#include "esphome/components/mqtt/mqtt_component.h"

namespace esphome {
namespace awox_mesh {

static const char *const TAG = "mesh_device";

/** \fn static std::string encrypt(std::string key, std::string data)
 *  \brief Encrypts a n x 16-byte data string with a 16-byte key, using AES encryption.
 *  \param key : 16-byte encryption key.
 *  \param data : n x 16-byte data string
 *  \returns the encrypted data string.
 */
static std::string encrypt(std::string key, std::string data) {
  // ESP_LOGD(TAG, "encrypt [key: %s, data: %s] data length [%d]", TextToBinaryString(key).c_str(),
  //          TextToBinaryString(data).c_str(), data.size());

  std::reverse(key.begin(), key.end());
  std::reverse(data.begin(), data.end());

  unsigned char buffer[16];
  auto aes128 = AES128();

  if (!aes128.setKey((uint8_t *) key.c_str(), key.size())) {
    ESP_LOGE(TAG, "Failed to set key");
  };
  aes128.encryptBlock(buffer, (uint8_t *) data.c_str());

  std::string result = std::string((char *) buffer, 16);

  std::reverse(result.begin(), result.end());
  // ESP_LOGD(TAG, "encrypted [%s]", TextToBinaryString(result).c_str());

  return result;
}

static std::string int_as_hex_string(unsigned char hex1, unsigned char hex2, unsigned char hex3) {
  char value[6];
  sprintf(value, "%02X%02X%02X", hex1, hex2, hex3);
  return std::string((char *) value, 6);
}

static unsigned char turn_off_bit(unsigned char value, int bit) {
  if (value & bit) {
    value = value ^ bit;
  }
  return value;
}

static int get_product_code(unsigned char part1, unsigned char part2) { return int(part2); }

static std::string get_product_code_as_hex_string(int product_id) {
    char value[15];
    sprintf(value, "Product: 0x%02X", product_id);
    return std::string((char *) value, 15);
  }

static std::string get_device_mac(unsigned char part3, unsigned char part4, unsigned char part5, unsigned char part6) {
  char value[17];
  sprintf(value, "A4:C1:%02X:%02X:%02X:%02X", part3, part4, part5, part6);
  return std::string((char *) value, 17);
}

static int convert_value_to_available_range(int value, int min_from, int max_from, int min_to, int max_to) {
  float normalized = (float) (value - min_from) / (float) (max_from - min_from);
  int new_value = std::min((int) round((normalized * (float) (max_to - min_to)) + min_to), max_to);

  return std::max(new_value, min_to);
}

void MeshDevice::on_shutdown() {
  // todo assure this message is published
  for (int i = 0; i < this->devices_.size(); i++) {
    this->devices_[i]->online = false;
    this->publish_availability(this->devices_[i], false);
  }
  this->publish_connected(false);
}

void MeshDevice::loop() {
  esp32_ble_client::BLEClientBase::loop();

  if (this->connected() && !this->command_queue.empty() && this->last_send_command < esphome::millis() - 180) {
    ESP_LOGV(TAG, "Send command, time since last command: %d", esphome::millis() - this->last_send_command);
    this->last_send_command = esphome::millis();
    QueuedCommand item = this->command_queue.front();
    ESP_LOGV(TAG, "Send command %d, for dest: %d", item.command, item.dest);
    this->command_queue.pop_front();
    ESP_LOGV(TAG, "remove item from queue");
    this->write_command(item.command, item.data, item.dest, false);
  }

  while (!this->delayed_availability_publish.empty()) {
    if (this->delayed_availability_publish.front().time > esphome::millis() - 3000) {
      break;
    }

    PublishOnlineStatus publish = this->delayed_availability_publish.front();
    this->delayed_availability_publish.pop_front();
    if (publish.online == publish.device->online) {
      this->publish_availability(publish.device, false);
    } else {
      ESP_LOGD(TAG, "Skipped publishing availability for %d - %s (is currently %s)", publish.device->mesh_id,
               publish.online ? "Online" : "Offline", publish.device->online ? "Online" : "Offline");
    }
  }

  for (auto *device : this->devices_) {
    if (!device->send_discovery && device->device_info_requested > 0 &&
        device->device_info_requested < esphome::millis() - 5000) {
      ESP_LOGD(TAG, "Request info again for %d", device->mesh_id);
      this->request_device_info(device);
    }
  }
}

bool MeshDevice::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                     esp_ble_gattc_cb_param_t *param) {
  ESP_LOGVV(TAG, "[%d] [%s] gattc_event_handler: event=%d gattc_if=%d", this->connection_index_,
            this->address_str_.c_str(), event, gattc_if);

  if (!esp32_ble_client::BLEClientBase::gattc_event_handler(event, gattc_if, param))
    return false;

  // ESP_LOGD(TAG, "gattc_event_handler");

  switch (event) {
    case ESP_GATTC_DISCONNECT_EVT: {
      ESP_LOGD(TAG, "[%d] [%s] ESP_GATTC_DISCONNECT_EVT, reason %d", this->connection_index_,
               this->address_str_.c_str(), param->disconnect.reason);
      if (param->disconnect.reason > 0) {
        this->set_address(0);
      }
      break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
    case ESP_GATTC_OPEN_EVT: {
      if (this->state_ == esp32_ble_tracker::ClientState::ESTABLISHED) {
        ESP_LOGI(TAG, "Connected....");
        this->setup_connection();
      }
      break;
    }

    case ESP_GATTC_NOTIFY_EVT: {
      if (param->notify.conn_id != this->conn_id_) {
        ESP_LOGW(TAG, "Notification received from different connection, skipped");
        break;
      }
      if (param->notify.handle != this->notification_char->handle) {
        ESP_LOGW(TAG, "Unknown notification received from handle %d: %s", param->notify.handle,
                 TextToBinaryString(std::string((char *) param->notify.value, param->notify.value_len)).c_str());
        break;
      }
      std::string notification = std::string((char *) param->notify.value, param->notify.value_len);
      std::string packet = this->decrypt_packet(notification);
      ESP_LOGV(TAG, "Notification received: %s", TextToBinaryString(packet).c_str());
      this->handle_packet(packet);
      break;
    }

    case ESP_GATTC_READ_CHAR_EVT: {
      if (param->read.conn_id != this->get_conn_id())
        break;
      if (param->read.status != ESP_GATT_OK) {
        ESP_LOGW(TAG, "Error reading char at handle %d, status=%d", param->read.handle, param->read.status);
        break;
      }
      if (param->read.handle == this->pair_char->handle) {
        if (param->read.value[0] == 0xd) {
          ESP_LOGI(TAG, "Response OK, let go");
          this->generate_session_key(this->random_key,
                                     std::string((char *) param->read.value, param->read.value_len).substr(1, 9));

          ESP_LOGI(TAG, "[%d] [%s] session key %s", this->get_conn_id(), this->address_str_.c_str(),
                   TextToBinaryString(this->session_key).c_str());

          this->request_status();

          break;
        } else if (param->read.value[0] == 0xe) {
          ESP_LOGE(TAG, "Device authentication error: known mesh credentials are not excepted by the device. Did you "
                        "re-pair them to your Awox app with a different account?");
        } else {
          ESP_LOGE(TAG, "Unexpected pair value");
        }

        ESP_LOGI(TAG, "[%d] [%s] response %s", this->get_conn_id(), this->address_str_.c_str(),
                 TextToBinaryString(std::string((char *) param->read.value, param->read.value_len)).c_str());
        this->disconnect();
        this->set_address(0);
      }
      break;
    }
  }

  return true;
}

void MeshDevice::setup_connection() {
  this->notification_char = this->get_characteristic(esp32_ble_tracker::ESPBTUUID::from_raw(uuid_info_service),
                                                     esp32_ble_tracker::ESPBTUUID::from_raw(uuid_notification_char));
  this->command_char = this->get_characteristic(esp32_ble_tracker::ESPBTUUID::from_raw(uuid_info_service),
                                                esp32_ble_tracker::ESPBTUUID::from_raw(uuid_command_char));
  this->pair_char = this->get_characteristic(esp32_ble_tracker::ESPBTUUID::from_raw(uuid_info_service),
                                             esp32_ble_tracker::ESPBTUUID::from_raw(uuid_pair_char));

  unsigned char key[8];
  esp_fill_random(key, 8);
  this->random_key = std::string((char *) key).substr(0, 8);
  std::string enc_data = this->key_encrypt(this->random_key);
  std::string packet = '\x0c' + this->random_key + enc_data.substr(0, 8);
  this->pair_char->write_value((uint8_t *) packet.data(), packet.size());

  esp_err_t status = esp_ble_gattc_read_char(this->get_gattc_if(), this->get_conn_id(), this->pair_char->handle,
                                             ESP_GATT_AUTH_REQ_NONE);

  if (status != ESP_OK) {
    ESP_LOGW(TAG, "[%d] [%s] esp_ble_gattc_read_char failed, error=%d", this->get_conn_id(), this->address_str_.c_str(),
             status);
  }

  ESP_LOGD(TAG, "Listen for notifications");
  status =
      esp_ble_gattc_register_for_notify(this->get_gattc_if(), this->get_remote_bda(), this->notification_char->handle);
  if (status) {
    ESP_LOGW(TAG, "[%d] [%s] esp_ble_gattc_register_for_notify failed, status=%d", this->get_conn_id(),
             this->address_str_.c_str(), status);
  }
  ESP_LOGD(TAG, "Enable notifications");
  uint16_t notify_en = 1;
  this->notification_char->write_value((uint8_t *) &notify_en, sizeof(notify_en));

  this->publish_connected(true);
}

std::string MeshDevice::combine_name_and_password() const {
  std::string data;
  ESP_LOGD(TAG, "combine mesh name + password: %s:%s", this->mesh_name.c_str(), this->mesh_password.c_str());
  std::string mesh_name = this->mesh_name;
  std::string mesh_password = this->mesh_password;
  mesh_name.append(16 - mesh_name.size(), 0);
  mesh_password.append(16 - mesh_password.size(), 0);

  for (int i = 0; i < 16; i++) {
    data.push_back(mesh_name[i] ^ mesh_password[i]);
  }

  return data;
}

void MeshDevice::generate_session_key(const std::string &data1, const std::string &data2) {
  std::string key = this->combine_name_and_password();

  this->session_key = encrypt(key, data1.substr(0, 8) + data2.substr(0, 8));
}

std::string MeshDevice::key_encrypt(std::string &key) const {
  std::string data = this->combine_name_and_password();
  std::string result;
  std::string e_key = key;
  e_key.append(16 - e_key.size(), 0);

  return encrypt(e_key, data);
}

std::string MeshDevice::encrypt_packet(std::string &packet) const {
  std::string auth_nonce = this->reverse_address.substr(0, 4) + '\1' + packet.substr(0, 3) + '\x0f';
  auth_nonce.append(7, 0);
  std::string authenticator;

  authenticator = encrypt(this->session_key, auth_nonce);

  for (int i = 0; i < 15; i++)
    authenticator[i] ^= packet[i + 5];

  std::string mac;

  mac = encrypt(this->session_key, authenticator);

  for (int i = 0; i < 2; i++)
    packet[i + 3] = mac[i];

  std::string iv = '\0' + this->reverse_address.substr(0, 4) + '\1' + packet.substr(0, 3);
  iv.append(7, 0);

  std::string buffer;
  buffer = encrypt(this->session_key, iv);

  for (int i = 0; i < 15; i++)
    packet[i + 5] ^= buffer[i];

  return packet;
}

std::string MeshDevice::decrypt_packet(std::string &packet) const {
  std::string iv = '\0' + this->reverse_address.substr(0, 3) + packet.substr(0, 5);
  iv.append(7, 0);

  std::string result;

  result = encrypt(this->session_key, iv);

  for (int i = 0; i < packet.size() - 7; i++)
    packet[i + 7] ^= result[i];

  return packet;
}

void MeshDevice::set_disconnect_callback(std::function<void()> &&f) { this->disconnect_callback = std::move(f); }

void MeshDevice::handle_packet(std::string &packet) {
  int mesh_id, mode;
  bool online, state, color_mode, transition_mode;
  unsigned char white_brightness, temperature, color_brightness, R, G, B;

  if (static_cast<unsigned char>(packet[7]) == COMMAND_ONLINE_STATUS_REPORT) {  // DC
    mesh_id = (static_cast<unsigned char>(packet[19]) * 256) + static_cast<unsigned char>(packet[10]);
    mode = static_cast<unsigned char>(packet[12]);
    online = packet[11] > 0;
    state = (mode & 1) == 1;
    color_mode = ((mode >> 1) & 1) == 1;
    transition_mode = ((mode >> 2) & 1) == 1;

    white_brightness = packet[13];
    temperature = packet[14];
    color_brightness = packet[15];

    R = packet[16];
    G = packet[17];
    B = packet[18];

    ESP_LOGD(TAG,
             "online status report: mesh: %d, on: %d, color_mode: %d, transition_mode: %d, w_b: %d, temp: %d, "
             "c_b: %d, rgb: %02X%02X%02X ",
             mesh_id, state, color_mode, transition_mode, white_brightness, temperature, color_brightness, R, G, B);

  } else if (static_cast<unsigned char>(packet[7]) == COMMAND_STATUS_REPORT) {  // DB
    mode = static_cast<unsigned char>(packet[10]);
    mesh_id = (static_cast<unsigned char>(packet[4]) * 256) + static_cast<unsigned char>(packet[3]);
    online = true;
    state = (mode & 1) == 1;
    color_mode = ((mode >> 1) & 1) == 1;
    transition_mode = ((mode >> 2) & 1) == 1;

    white_brightness = packet[11];
    temperature = packet[12];
    color_brightness = packet[13];

    R = packet[14];
    G = packet[15];
    B = packet[16];

    ESP_LOGD(TAG,
             "status report: mesh: %d, on: %d, color_mode: %d, transition_mode: %d, w_b: %d, temp: %d, "
             "c_b: %d, rgb: %02X%02X%02X ",
             mesh_id, state, color_mode, transition_mode, white_brightness, temperature, color_brightness, R, G, B);

  } else if (static_cast<unsigned char>(packet[7]) == 0xD8 && !packet[10]) {
    mesh_id = (static_cast<unsigned char>(packet[4]) * 256) + static_cast<unsigned char>(packet[3]);

    Device *device = this->get_device(mesh_id);
    device->mac = get_device_mac(static_cast<unsigned char>(packet[16]), static_cast<unsigned char>(packet[15]),
                                 static_cast<unsigned char>(packet[14]), static_cast<unsigned char>(packet[13]));
    device->device_info = this->device_info_resolver->get_by_product_id(
        get_product_code(static_cast<unsigned char>(packet[11]), static_cast<unsigned char>(packet[12])));

    ESP_LOGD(TAG, "MAC report, dev [%d]: productID: %02X mac: %s => %s", mesh_id, device->device_info->get_product_id(),
             device->mac.c_str(), TextToBinaryString(packet).c_str());

    this->send_discovery(device);
    return;

  } else {
    ESP_LOGW(TAG, "Unknown report, dev [%d]: command %02X => %s", mesh_id, static_cast<unsigned char>(packet[7]),
             TextToBinaryString(packet).c_str());

    return;
  }

  Device *device = this->get_device(mesh_id);
  bool online_changed = false;

  if (device->online != online) {
    online_changed = true;
  }
  device->online = online;
  device->state = state;
  device->color_mode = color_mode;
  device->transition_mode = transition_mode;

  device->white_brightness = white_brightness;
  device->temperature = temperature;
  device->color_brightness = color_brightness;

  device->R = R;
  device->G = G;
  device->B = B;
  device->last_online = esphome::millis();

  ESP_LOGI(TAG, this->device_state_as_string(device).c_str());
  this->publish_state(device);

  if (online_changed) {
    this->publish_availability(device, true);
  }
}

std::string MeshDevice::device_state_as_string(Device *device) {
  std::string output = "";

  output += std::to_string(device->mesh_id);
  output += ": ";
  output += device->state ? "ON" : "OFF";

  output += " ";

  if (device->color_mode) {
    output += "#" + int_as_hex_string(device->R, device->G, device->B);
    output += " (";
    output += std::to_string(device->color_brightness);
    output += " %%)";
  } else {
    output += "temp: " + std::to_string(device->temperature);
    output += " (";
    output += std::to_string(device->white_brightness);
    output += " %%)";
  }
  output += device->online ? " ONLINE" : " OFFLINE!!";

  return output;
}

std::string MeshDevice::get_discovery_topic_(const MQTTDiscoveryInfo &discovery_info, Device *device) const {
  return discovery_info.prefix + "/" + device->device_info->get_component_type() + "/awox-" +
         str_sanitize(device->mac) + "/config";
}

std::string MeshDevice::get_mqtt_topic_for_(Device *device, const std::string &suffix) const {
  return global_mqtt_client->get_topic_prefix() + "/" + std::to_string(device->mesh_id) + "/" + suffix;
}

void MeshDevice::publish_availability(Device *device, bool delayed) {
  if (delayed) {
    PublishOnlineStatus publish = {};
    publish.device = device;
    publish.online = device->online;
    publish.time = esphome::millis();
    this->delayed_availability_publish.push_back(publish);
    ESP_LOGD(TAG, "Delayed publish online/offline for %d - %s", device->mesh_id, device->online ? "online" : "offline");
    return;
  }

  const std::string message = device->online ? "online" : "offline";
  ESP_LOGI(TAG, "Publish online/offline for %d - %s", device->mesh_id, message.c_str());
  global_mqtt_client->publish(this->get_mqtt_topic_for_(device, "availability"), message, 0, true);
}

void MeshDevice::publish_connected(bool connected) {
  const std::string message = connected ? "online" : "offline";
  ESP_LOGI(TAG, "Publish connected to mesh device - %s", message.c_str());
  global_mqtt_client->publish(global_mqtt_client->get_topic_prefix() + "/connected", message, 0, true);
}

void MeshDevice::publish_state(Device *device) {
  if (device->mac == "") {
    ESP_LOGW(TAG, "'%s': Can not yet send publish state, mac address not known...",
             std::to_string(device->mesh_id).c_str());
    return;
  }
  if (device->device_info->has_feature(FEATURE_LIGHT_MODE)) {
    global_mqtt_client->publish_json(
        this->get_mqtt_topic_for_(device, "state"),
        [this, device](JsonObject root) {
          root["state"] = device->state ? "ON" : "OFF";

          root["color_mode"] = "color_temp";

          root["brightness"] = convert_value_to_available_range(device->white_brightness, 1, 0x7f, 0, 255);

          if (device->color_mode) {
            root["color_mode"] = "rgb";
            root["brightness"] = convert_value_to_available_range(device->color_brightness, 0xa, 0x64, 0, 255);
          } else {
            root["color_temp"] = convert_value_to_available_range(device->temperature, 0, 0x7f, 153, 370);
          }
          JsonObject color = root.createNestedObject("color");
          color["r"] = device->R;
          color["g"] = device->G;
          color["b"] = device->B;
        },
        0, true);
  } else {
    global_mqtt_client->publish(
        this->get_mqtt_topic_for_(device, "state"),
        device->state ? "ON" : "OFF",
        device->state ? 2 : 3
    );
  }
}

void MeshDevice::send_discovery(Device *device) {
  if (device->mac == "") {
    ESP_LOGW(TAG, "'%s': Can not yet send discovery, mac address not known...",
             std::to_string(device->mesh_id).c_str());
    return;
  }
  ESP_LOGD(TAG, "'%s': Sending discovery...", std::to_string(device->mesh_id).c_str());
  const MQTTDiscoveryInfo &discovery_info = global_mqtt_client->get_discovery_info();
  device->send_discovery = true;

  global_mqtt_client->publish_json(
      this->get_discovery_topic_(discovery_info, device),
      [this, device, discovery_info](JsonObject root) {
        root["schema"] = "json";

        // Entity
        root[MQTT_NAME] = device->device_info->get_name();
        root[MQTT_UNIQUE_ID] = "awox-" + device->mac + "-" + device->device_info->get_component_type();

        if (device->device_info->get_icon() != "") {
          root[MQTT_ICON] = device->device_info->get_icon();
        }

        // State and command topic
        root[MQTT_STATE_TOPIC] = this->get_mqtt_topic_for_(device, "state");
        root[MQTT_COMMAND_TOPIC] = this->get_mqtt_topic_for_(device, "command");

        // Availavility topics
        JsonArray availability = root.createNestedArray(MQTT_AVAILABILITY);
        auto availability_topic_1 = availability.createNestedObject();
        availability_topic_1[MQTT_TOPIC] = this->get_mqtt_topic_for_(device, "availability");
        auto availability_topic_2 = availability.createNestedObject();
        availability_topic_2[MQTT_TOPIC] = global_mqtt_client->get_topic_prefix() + "/status";
        auto availability_topic_3 = availability.createNestedObject();
        availability_topic_3[MQTT_TOPIC] = global_mqtt_client->get_topic_prefix() + "/connected";
        root[MQTT_AVAILABILITY_MODE] = "all";

        // Features
        root[MQTT_COLOR_MODE] = true;

        if (device->device_info->has_feature(FEATURE_WHITE_BRIGHTNESS) ||
            device->device_info->has_feature(FEATURE_COLOR_BRIGHTNESS)) {
          root["brightness"] = true;
          root["brightness_scale"] = 255;
        }

        JsonArray color_modes = root.createNestedArray("supported_color_modes");

        if (device->device_info->has_feature(FEATURE_COLOR)) {
          color_modes.add("rgb");
        }

        if (device->device_info->has_feature(FEATURE_WHITE_TEMPERATURE)) {
          color_modes.add("color_temp");

          root[MQTT_MIN_MIREDS] = 153;
          root[MQTT_MAX_MIREDS] = 370;
        }

        // brightness should always be used alone
        // https://developers.home-assistant.io/docs/core/entity/light/#color-modes
        if (color_modes.size() == 0 && device->device_info->has_feature(FEATURE_WHITE_BRIGHTNESS)) {
          color_modes.add("brightness");
        }

        if (color_modes.size() == 0) {
          color_modes.add("onoff");
        }

        // Device
        JsonObject device_info = root.createNestedObject(MQTT_DEVICE);

        JsonArray identifiers = device_info.createNestedArray(MQTT_DEVICE_IDENTIFIERS);
        identifiers.add("esp-awox-mesh-" + std::to_string(device->mesh_id));
        identifiers.add(device->mac);

        device_info[MQTT_DEVICE_NAME] = root[MQTT_NAME];

        if (device->device_info->get_model() == "") {
          device_info[MQTT_DEVICE_MODEL] = get_product_code_as_hex_string(device->device_info->get_product_id());
        } else {
          device_info[MQTT_DEVICE_MODEL] = device->device_info->get_model();
        }
        device_info[MQTT_DEVICE_MANUFACTURER] = device->device_info->get_manufacturer();
        device_info["via_device"] = get_mac_address();
      },
      0, discovery_info.retain);

  if (device->device_info->has_feature(FEATURE_LIGHT_MODE)) {
    global_mqtt_client->subscribe_json(
        this->get_mqtt_topic_for_(device, "command"),
        [this, device](const std::string &topic, JsonObject root) { this->process_incomming_command(device, root); });
  } else {
    global_mqtt_client->subscribe(
        this->get_mqtt_topic_for_(device, "command"),
        [this, device](const std::string &topic, const std::string &payload) {
                  ESP_LOGI(TAG, "command %s - %s", topic.c_str(), payload.c_str());
          auto val = parse_on_off(payload.c_str());
          switch (val) {
            case PARSE_ON:
              device->state = true;
              this->set_state(device->mesh_id, true);
              break;
            case PARSE_OFF:
              device->state = false;
              this->set_state(device->mesh_id, false);
              break;
            case PARSE_TOGGLE:
              device->state = !device->state;
              this->set_state(device->mesh_id, device->state);
              break;
            case PARSE_NONE:
              break;
          }
      });
  }
  this->publish_availability(device, true);
}

void MeshDevice::process_incomming_command(Device *device, JsonObject root) {
  ESP_LOGV(TAG, "[%d] Process command", device->mesh_id);
  bool state_set = false;
  if (root.containsKey("color")) {
    JsonObject color = root["color"];

    state_set = true;
    device->state = true;
    device->R = (int) color["r"];
    device->G = (int) color["g"];
    device->B = (int) color["b"];

    ESP_LOGD(TAG, "[%d] Process command color %d %d %d", device->mesh_id, (int) color["r"], (int) color["g"],
             (int) color["b"]);

    this->set_color(device->mesh_id, (int) color["r"], (int) color["g"], (int) color["b"]);
  }

  if (root.containsKey("brightness") && !root.containsKey("color_temp") &&
      (root.containsKey("color") || device->color_mode)) {
    int brightness = convert_value_to_available_range((int) root["brightness"], 0, 255, 0xa, 0x64);

    state_set = true;
    device->state = true;
    device->color_brightness = brightness;

    ESP_LOGD(TAG, "[%d] Process command color_brightness %d", device->mesh_id, (int) root["brightness"]);
    this->set_color_brightness(device->mesh_id, brightness);

  } else if (root.containsKey("brightness")) {
    int brightness = convert_value_to_available_range((int) root["brightness"], 0, 255, 1, 0x7f);

    state_set = true;
    device->state = true;
    device->white_brightness = brightness;

    ESP_LOGD(TAG, "[%d] Process command white_brightness %d", device->mesh_id, (int) root["brightness"]);
    this->set_white_brightness(device->mesh_id, brightness);
  }

  if (root.containsKey("color_temp")) {
    int temperature = convert_value_to_available_range((int) root["color_temp"], 153, 370, 0, 0x7f);

    state_set = true;
    device->state = true;
    device->temperature = temperature;

    ESP_LOGD(TAG, "[%d] Process command color_temp %d", device->mesh_id, (int) root["color_temp"]);
    this->set_white_temperature(device->mesh_id, temperature);
  }

  if (root.containsKey("state")) {
    ESP_LOGD(TAG, "[%d] Process command state", device->mesh_id);
    auto val = parse_on_off(root["state"]);
    switch (val) {
      case PARSE_ON:
        device->state = true;
        if (!state_set) {
          this->set_state(device->mesh_id, true);
        }
        break;
      case PARSE_OFF:
        device->state = false;
        this->set_state(device->mesh_id, false);
        break;
      case PARSE_TOGGLE:
        device->state = !device->state;
        this->set_state(device->mesh_id, device->state);
        break;
      case PARSE_NONE:
        break;
    }
  }

  this->publish_state(device);
}

std::string MeshDevice::build_packet(int dest, int command, const std::string &data) {
  /* Telink mesh packets take the following form:
 bytes 0-1   : packet counter
 bytes 2-4   : not used (=0)
 bytes 5-6   : mesh ID
 bytes 7     : command code
 bytes 8-9   : vendor code
 bytes 10-20 : command data

All multi-byte elements are in little-endian form.
Packet counter runs between 1 and 0xffff.
*/
  ESP_LOGV(TAG, "command: %d, data: %s, dest: %d", command, TextToBinaryString(data).c_str(), dest);
  std::string packet;
  packet.resize(20, 0);
  packet[0] = this->packet_count & 0xff;
  packet[1] = (this->packet_count++ >> 8) & 0xff;
  packet[5] = dest & 0xff;
  packet[6] = (dest >> 8) & 0xff;
  packet[7] = command & 0xff;
  packet[8] = 0x60;  // this->vendor & 0xff;
  packet[9] = 0x01;  //(this->vendor >> 8) & 0xff;
  for (int i = 0; i < data.size(); i++)
    packet[i + 10] = data[i];

  // ESP_LOGI(TAG, "[%d] [%s] packet (packet_count+dest+command) %s", this->get_conn_id(), this->address_str_.c_str(),
  //          TextToBinaryString(packet).c_str());

  std::string enc_packet = this->encrypt_packet(packet);

  if (this->packet_count > 0xffff)
    this->packet_count = 1;

  return enc_packet;
}

void MeshDevice::queue_command(int command, const std::string &data, int dest) {
  QueuedCommand item = {};
  item.data = data;
  item.command = command;
  item.dest = dest;
  this->command_queue.push_back(item);
}

bool MeshDevice::write_command(int command, const std::string &data, int dest, bool withResponse) {
  ESP_LOGV(TAG, "[%d] [%s] write_command packet %02X => %s", this->get_conn_id(), this->address_str_.c_str(), command,
           TextToBinaryString(data).c_str());
  std::string packet = this->build_packet(dest, command, data);
  // todo: withResponse
  auto status = this->command_char->write_value((uint8_t *) packet.data(), packet.size());
  // todo: check write return value
  return status ? false : true;
}

void MeshDevice::request_status() {
  if (this->connected()) {
    ESP_LOGD(TAG, "[%d] [%s] request status update", this->get_conn_id(), this->address_str_.c_str());
    this->write_command(C_REQUEST_STATUS, {0x10}, 0xffff);
  }
}

Device *MeshDevice::get_device(int mesh_id) {
  ESP_LOGVV(TAG, "get device %d", mesh_id);

  auto found = std::find_if(this->devices_.begin(), this->devices_.end(),
                            [mesh_id](const Device *_f) { return _f->mesh_id == mesh_id; });

  if (found != devices_.end()) {
    Device *ptr = devices_.at(found - devices_.begin());
    ESP_LOGV(TAG, "Found existing mesh_id: %d, Number of found mesh devices = %d", ptr->mesh_id, this->devices_.size());
    return ptr;
  }

  Device *device = new Device;
  device->mesh_id = mesh_id;
  this->devices_.push_back(device);

  ESP_LOGI(TAG, "Added mesh_id: %d, Number of found mesh devices = %d", device->mesh_id, this->devices_.size());

  this->request_device_info(device);
  // this->request_device_version(device->mesh_id);

  return device;
}

bool MeshDevice::set_state(int dest, bool state) {
  this->queue_command(C_POWER, {state, 0, 0}, dest);
  return true;
}

bool MeshDevice::set_color(int dest, int red, int green, int blue) {
  this->queue_command(C_COLOR, {0x04, static_cast<char>(red), static_cast<char>(green), static_cast<char>(blue)}, dest);
  return true;
}

bool MeshDevice::set_color_brightness(int dest, int brightness) {
  this->queue_command(C_COLOR_BRIGHTNESS, {static_cast<char>(brightness)}, dest);
  return true;
}

bool MeshDevice::set_white_brightness(int dest, int brightness) {
  this->queue_command(C_WHITE_BRIGHTNESS, {static_cast<char>(brightness)}, dest);
  return true;
}

bool MeshDevice::set_white_temperature(int dest, int temp) {
  this->queue_command(C_WHITE_TEMPERATURE, {static_cast<char>(temp)}, dest);
  return true;
}

bool MeshDevice::request_device_info(Device *device) {
  device->device_info_requested = esphome::millis();
  this->queue_command(COMMAND_DEVICE_INFO_QUERY, {0x10, 0x00}, device->mesh_id);
  return true;
}

bool MeshDevice::request_device_version(int dest) {
  this->queue_command(COMMAND_DEVICE_INFO_QUERY, {0x10, 0x02}, dest);
  return true;
}

}  // namespace awox_mesh
}  // namespace esphome
