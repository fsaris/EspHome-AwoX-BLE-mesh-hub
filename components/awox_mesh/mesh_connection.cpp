#include <cstring>
#include <bitset>
#include <algorithm>
#include <math.h>
#include "mesh_connection.h"
#include "awox_mesh.h"
#include "device_info.h"
#include "helpers.h"
#include "group.h"
#include "aes/esp_aes.h"
#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace awox_mesh {

static const char *const TAG = "awox.connection";

/** \fn static std::string encrypt(std::string key, std::string data)
 *  \brief Encrypts a n x 16-byte data string with a 16-byte key, using AES encryption.
 *  \param key : 16-byte encryption key.
 *  \param data : n x 16-byte data string
 *  \returns the encrypted data string.
 */
static std::string encrypt(std::string key, std::string data) {
  // ESP_LOGD(TAG, "encrypt [key: %s, data: %s] data length [%d]", string_as_binary_string(key).c_str(),
  //          string_as_binary_string(data).c_str(), data.size());

  std::reverse(key.begin(), key.end());
  std::reverse(data.begin(), data.end());

  unsigned char buffer[16];

  esp_aes_context aes;
  esp_aes_init(&aes);
  esp_aes_setkey(&aes, (const unsigned char *) key.c_str(), key.size() * 8);
  esp_aes_crypt_ecb(&aes, 1, (const unsigned char *) data.c_str(), buffer);
  esp_aes_free(&aes);

  std::string result = std::string((char *) buffer, 16);

  std::reverse(result.begin(), result.end());
  // ESP_LOGD(TAG, "encrypted [%s]", string_as_binary_string(result).c_str());

  return result;
}

static int get_product_code(unsigned char part1, unsigned char part2) { return int(part2); }

void MeshConnection::connect_to(FoundDevice *found_device) {
  this->set_address(found_device->device.address_uint64());
  this->set_state(esp32_ble_tracker::ClientState::IDLE);
  this->found_device = found_device;
  this->found_device->connected = true;
  this->parse_device(found_device->device);
  if (this->found_device->mesh_id) {
    this->add_mesh_id(this->found_device->mesh_id);
  }
}

void MeshConnection::set_address(uint64_t address) {
  if (this->found_device) {
    this->found_device->connected = false;
  }

  if (address == 0) {
    this->disconnect_callback();

    // Mark each linked mesh device as offline
    for (int mesh_id : this->linked_mesh_ids_) {
      Device *device = this->mesh_->get_device(mesh_id);
      if (device != nullptr) {
        device->online = false;
        this->mesh_->publish_availability(device, true);
      }
    }
  }

  this->clear_linked_mesh_ids();

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
}

void MeshConnection::loop() {
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
}

bool MeshConnection::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
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
                 string_as_binary_string(std::string((char *) param->notify.value, param->notify.value_len)).c_str());
        break;
      }
      std::string notification = std::string((char *) param->notify.value, param->notify.value_len);
      std::string packet = this->decrypt_packet(notification);
      ESP_LOGV(TAG, "Notification received: %s", string_as_binary_string(packet).c_str());
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
          ESP_LOGI(TAG, "Response OK, let's go");
          this->generate_session_key(this->random_key,
                                     std::string((char *) param->read.value, param->read.value_len).substr(1, 9));

          ESP_LOGI(TAG, "[%d] [%s] session key %s", this->get_conn_id(), this->address_str_.c_str(),
                   string_as_binary_string(this->session_key).c_str());

          this->request_status();

          break;
        } else if (param->read.value[0] == 0xe) {
          ESP_LOGE(TAG, "Device authentication error: known mesh credentials are not excepted by the device. Did you "
                        "re-pair them to your Awox app with a different account?");
        } else {
          ESP_LOGE(TAG, "Unexpected pair value");
        }

        ESP_LOGI(TAG, "[%d] [%s] response %s", this->get_conn_id(), this->address_str_.c_str(),
                 string_as_binary_string(std::string((char *) param->read.value, param->read.value_len)).c_str());
        this->disconnect();
        this->set_address(0);
      }
      break;
    }

    default:
      break;
  }

  return true;
}

void MeshConnection::setup_connection() {
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

  // todo; set connection state and trigger mesh to publish this
  this->mesh_->publish_connected();
}

std::string MeshConnection::combine_name_and_password() const {
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

void MeshConnection::generate_session_key(const std::string &data1, const std::string &data2) {
  std::string key = this->combine_name_and_password();

  this->session_key = encrypt(key, data1.substr(0, 8) + data2.substr(0, 8));
}

std::string MeshConnection::key_encrypt(std::string &key) const {
  std::string data = this->combine_name_and_password();
  std::string result;
  std::string e_key = key;
  e_key.append(16 - e_key.size(), 0);

  return encrypt(e_key, data);
}

std::string MeshConnection::encrypt_packet(std::string &packet) const {
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

std::string MeshConnection::decrypt_packet(std::string &packet) const {
  std::string iv = '\0' + this->reverse_address.substr(0, 3) + packet.substr(0, 5);
  iv.append(7, 0);

  std::string result;

  result = encrypt(this->session_key, iv);

  for (int i = 0; i < packet.size() - 7; i++)
    packet[i + 7] ^= result[i];

  return packet;
}

void MeshConnection::set_disconnect_callback(std::function<void()> &&f) { this->disconnect_callback = std::move(f); }

void MeshConnection::handle_packet(std::string &packet) {
  int mesh_id, mode;
  bool online, state, color_mode, transition_mode;
  unsigned char white_brightness, temperature, color_brightness, R, G, B;
  Device *device;

  mesh_id = 0;

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
             "c_b: %d, rgb: %02X%02X%02X, mode: %d %02X",
             mesh_id, state, color_mode, transition_mode, white_brightness, temperature, color_brightness, R, G, B,
             mode, mode);

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
             "c_b: %d, rgb: %02X%02X%02X, mode: %d %02X",
             mesh_id, state, color_mode, transition_mode, white_brightness, temperature, color_brightness, R, G, B,
             mode, mode);

  } else if (static_cast<unsigned char>(packet[7]) == COMMAND_ADDRESS_REPORT && !packet[10]) {
    mesh_id = (static_cast<unsigned char>(packet[4]) * 256) + static_cast<unsigned char>(packet[3]);
    device = this->mesh_->get_device(mesh_id);
    if (device == nullptr) {
      ESP_LOGD(TAG, "MAC report, dev [%d] ignored. MeshID not part of allowed_mesh_ids", mesh_id);
      return;
    }

    device->set_address(static_cast<unsigned char>(packet[16]), static_cast<unsigned char>(packet[15]),
                        static_cast<unsigned char>(packet[14]), static_cast<unsigned char>(packet[13]));
    device->product_id =
        get_product_code(static_cast<unsigned char>(packet[11]), static_cast<unsigned char>(packet[12]));

    ESP_LOGD(TAG, "MAC report, dev [%d]: productID: 0x%02X mac: %s", mesh_id, device->product_id,
             device->address_str().c_str());

    this->mesh_->send_discovery(device);
    return;
  } else if (static_cast<unsigned char>(packet[7]) == COMMAND_GROUP_ID_REPORT) {
    mesh_id = (static_cast<unsigned char>(packet[4]) * 256) + static_cast<unsigned char>(packet[3]);
    device = this->mesh_->get_device(mesh_id);
    if (device == nullptr) {
      ESP_LOGD(TAG, "Group report, dev [%d] ignored. MeshID not part of allowed_mesh_ids", mesh_id);
      return;
    }

    for (int i = 10; i < 20; i++) {
      if (packet[i] == 0xFF) {
        break;
      }
      ESP_LOGD(TAG, "Group report, dev [%d] %d", mesh_id, packet[i]);
      this->mesh_->get_group(static_cast<unsigned char>(packet[i]), device);
    }

    return;
  } else {
    mesh_id = (static_cast<unsigned char>(packet[4]) * 256) + static_cast<unsigned char>(packet[3]);
    ESP_LOGW(TAG, "Unknown report, dev [%d]: command %02X => %s", mesh_id, static_cast<unsigned char>(packet[7]),
             string_as_binary_string(packet).c_str());

    return;
  }

  device = this->mesh_->get_device(mesh_id);
  if (device == nullptr) {
    ESP_LOGD(TAG, "Report, dev [%d] ignored. MeshID not part of allowed_mesh_ids", mesh_id);
    return;
  }

  if (online) {
    this->add_mesh_id(mesh_id);
  } else {
    this->remove_mesh_id(mesh_id);
  }

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

  // todo move logic below to mesh or mqtt class
  ESP_LOGI(TAG, device->device_state_as_string().c_str());
  this->mesh_->publish_state(device);

  if (online_changed) {
    this->mesh_->publish_availability(device, true);
  }
}

int MeshConnection::mesh_id() {
  if (this->found_device != nullptr) {
    return this->found_device->mesh_id;
  }

  return 0;
}

bool MeshConnection::mesh_id_linked(int mesh_id) {
  std::vector<int>::iterator position =
      std::find(this->linked_mesh_ids_.begin(), this->linked_mesh_ids_.end(), mesh_id);

  return position != this->linked_mesh_ids_.end();
}

void MeshConnection::add_mesh_id(int mesh_id) {
  if (!this->mesh_id_linked(mesh_id)) {
    this->linked_mesh_ids_.push_back(mesh_id);
    sort(this->linked_mesh_ids_.begin(), this->linked_mesh_ids_.end());
  }
}

void MeshConnection::remove_mesh_id(int mesh_id) {
  this->linked_mesh_ids_.erase(std::remove(this->linked_mesh_ids_.begin(), this->linked_mesh_ids_.end(), mesh_id),
                               this->linked_mesh_ids_.end());
}

void MeshConnection::clear_linked_mesh_ids() { this->linked_mesh_ids_.clear(); }

std::string MeshConnection::build_packet(int dest, int command, const std::string &data) {
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
  ESP_LOGV(TAG, "command: %d, data: %s, dest: %d", command, string_as_binary_string(data).c_str(), dest);
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
  //          string_as_binary_string(packet).c_str());

  std::string enc_packet = this->encrypt_packet(packet);

  if (this->packet_count > 0xffff)
    this->packet_count = 1;

  return enc_packet;
}

void MeshConnection::queue_command(int command, const std::string &data, int dest) {
  QueuedCommand item = {};
  item.data = data;
  item.command = command;
  item.dest = dest;
  this->command_queue.push_back(item);
}

bool MeshConnection::write_command(int command, const std::string &data, int dest, bool withResponse) {
  ESP_LOGD(TAG, "[%d] [%s] [%d] write_command packet %02X => %s", this->get_conn_id(), this->address_str_.c_str(), dest,
           command, string_as_binary_string(data).c_str());
  std::string packet = this->build_packet(dest, command, data);
  // todo: withResponse
  auto status = this->command_char->write_value((uint8_t *) packet.data(), packet.size());
  // todo: check write return value
  return status ? false : true;
}

void MeshConnection::request_status() {
  if (this->connected()) {
    ESP_LOGD(TAG, "[%d] [%s] request status update", this->get_conn_id(), this->address_str_.c_str());
    this->write_command(C_REQUEST_STATUS, {0x10}, 0xffff);
  }
}

void MeshConnection::set_power(int dest, bool state) { this->queue_command(C_POWER, {state, 0, 0}, dest); }

void MeshConnection::set_color(int dest, int red, int green, int blue) {
  this->queue_command(C_COLOR, {0x04, static_cast<char>(red), static_cast<char>(green), static_cast<char>(blue)}, dest);
}

void MeshConnection::set_color_brightness(int dest, int brightness) {
  this->queue_command(C_COLOR_BRIGHTNESS, {static_cast<char>(brightness)}, dest);
}

void MeshConnection::set_white_brightness(int dest, int brightness) {
  this->queue_command(C_WHITE_BRIGHTNESS, {static_cast<char>(brightness)}, dest);
}

void MeshConnection::set_white_temperature(int dest, int temp) {
  this->queue_command(C_WHITE_TEMPERATURE, {static_cast<char>(temp)}, dest);
}

void MeshConnection::set_preset(int dest, int preset) {
  this->queue_command(C_PRESET, {static_cast<char>(preset)}, dest);
  // this->queue_command(C_SEQUENCE_COLOR_DURATION, {static_cast<char>(500)}, dest);
  // this->queue_command(C_SEQUENCE_FADE_DURATION, {static_cast<char>(500)}, dest);
}

void MeshConnection::request_status_update(int dest) { this->queue_command(C_REQUEST_STATUS, {0x10}, dest); }

void MeshConnection::request_device_info(Device *device) {
  this->queue_command(COMMAND_DEVICE_INFO_QUERY, {0x10, 0x00}, device->mesh_id);
}

void MeshConnection::request_group_info(Device *device) {
  this->queue_command(COMMAND_GROUP_ID_QUERY, {0x0A, 0x01}, device->mesh_id);
}

bool MeshConnection::request_device_version(int dest) {
  this->queue_command(COMMAND_DEVICE_INFO_QUERY, {0x10, 0x02}, dest);
  return true;
}

}  // namespace awox_mesh
}  // namespace esphome
