#ifdef USE_ESP32
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <math.h>
#include <regex>
#include "awox_mesh.h"
#include "esphome/components/mqtt/mqtt_const.h"
#include "esphome/components/mqtt/mqtt_component.h"

#include "esphome/core/application.h"
#include "esphome/core/log.h"

namespace esphome {
namespace awox_mesh {

static const char *const TAG = "awox.mesh";
static const int RSSI_NOT_AVAILABLE = -9999;

static int convert_value_to_available_range(int value, int min_from, int max_from, int min_to, int max_to) {
  float normalized = (float) (value - min_from) / (float) (max_from - min_from);
  int new_value = std::min((int) round((normalized * (float) (max_to - min_to)) + min_to), max_to);

  return std::max(new_value, min_to);
}

static std::string get_product_code_as_hex_string(int product_id) {
  char value[15];
  sprintf(value, "Product: 0x%02X", product_id);
  return std::string((char *) value, 15);
}

static bool id_in_vector(int mesh_id, const std::vector<int> &vector) {
  std::vector<int>::const_iterator position = std::find(vector.begin(), vector.end(), mesh_id);

  return position != vector.end();
}

static bool matching_mesh_ids(const std::vector<int> &vector1, const std::vector<int> &vector2) {
  std::vector<int> result;

  std::set_intersection(vector1.begin(), vector1.end(), vector2.begin(), vector2.end(), back_inserter(result));

  return result.size() > 0;
}

float AwoxMesh::get_setup_priority() const { return setup_priority::AFTER_CONNECTION; }

void AwoxMesh::register_connection(MeshConnection *connection) {
  ESP_LOGD(TAG, "register_connection");

  this->connections_.push_back(connection);

  connection->mesh_name = this->mesh_name;
  connection->mesh_password = this->mesh_password;
  connection->mesh_ = this;

  connection->set_disconnect_callback([this]() {
    ESP_LOGI(TAG, "disconnected");

    this->publish_connected();
  });
}

FoundDevice *AwoxMesh::add_to_found_devices(const esp32_ble_tracker::ESPBTDevice &device) {
  FoundDevice *found_device;

  auto found = std::find_if(this->found_devices_.begin(), this->found_devices_.end(), [device](const FoundDevice *_f) {
    return _f->device.address_uint64() == device.address_uint64();
  });

  if (found != found_devices_.end()) {
    found_device = found_devices_.at(found - found_devices_.begin());
    ESP_LOGV(TAG, "Found existing device: %s", found_device->device.address_str().c_str());
  } else {
    ESP_LOGV(TAG, "Register device: %s", device.address_str().c_str());
    found_device = new FoundDevice();
    found_device->device = device;
    this->found_devices_.push_back(found_device);
  }

  found_device->rssi = device.get_rssi();
  found_device->last_detected = esphome::millis();

  this->set_rssi_for_devices_that_are_not_available();

  this->sort_devices();

  return found_device;
}

bool AwoxMesh::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  if (device.address_str().rfind(this->address_prefix, 0) != 0) {
    return false;
  }

  if (!this->mac_addresses_allowed(device.address_uint64())) {
    return false;
  }

  add_to_found_devices(device);

  ESP_LOGV(TAG, "Found Awox device %s - %s. RSSI: %d dB (total devices: %d)", device.get_name().c_str(),
           device.address_str().c_str(), device.get_rssi(), this->found_devices_.size());

  return true;
}

void AwoxMesh::setup() {
  Component::setup();

  // Use retained MQTT messages to publish a default offline status for all devices
  // todo: move to mqtt class
  global_mqtt_client->subscribe(
      global_mqtt_client->get_topic_prefix() + "/#", [this](const std::string &topic, const std::string &payload) {
        if (std::regex_match(topic, std::regex(global_mqtt_client->get_topic_prefix() + "/[0-9]+/availability"))) {
          ESP_LOGD(TAG, "Received topic: %s, %s", topic.c_str(), payload.c_str());
          if (payload == "online") {
            global_mqtt_client->publish(topic.c_str(), "offline");
          }
        }
      });
  this->set_timeout("stop_listening_for_availability", 3000,
                    []() { global_mqtt_client->unsubscribe(global_mqtt_client->get_topic_prefix() + "/#"); });

  this->set_interval("publish_connection", 5000, [this]() { this->publish_connected(); });

  this->publish_connection_sensor_discovery();
}

bool AwoxMesh::start_up_delay_done() {
  return esphome::millis() - this->start > 10000 && this->found_devices_.size() > 0;
}

void AwoxMesh::loop() {
  if (!ready_to_connect && this->start_up_delay_done()) {
    ready_to_connect = true;
  }

  if (!ready_to_connect) {
    return;
  }

  const uint32_t now = esphome::millis();
  const uint32_t since_last_attempt = now - this->last_connection_attempt;

  if ((!this->has_active_connection && since_last_attempt > 5000) || since_last_attempt > 20000) {
    this->last_connection_attempt = now;

    this->disconnect_connections_with_overlapping_mesh_ids();

    for (auto *connection : this->connections_) {
      if (connection->get_address() == 0) {
        auto *found_device = this->next_to_connect();

        if (found_device == nullptr) {
          ESP_LOGD(TAG, "No devices found to connect to");
          break;
        }

        if (found_device->connected) {
          ESP_LOGI(TAG, "Skipped to connect %s => rssi: %d already connected!!",
                   found_device->device.address_str().c_str(), found_device->rssi);
          break;
        }

        ESP_LOGI(TAG, "Try to connect %s => rssi: %d", found_device->device.address_str().c_str(), found_device->rssi);

        connection->connect_to(found_device);

        this->set_timeout("connecting", 20000, [this, found_device, connection]() {
          if (connection->connected()) {
            return;
          }
          ESP_LOGI(TAG, "Failed to connect %s => rssi: %d", found_device->device.address_str().c_str(),
                   found_device->rssi);
          this->set_rssi_for_devices_that_are_not_available();
          connection->disconnect();
          connection->set_address(0);
        });

        // max 1 new connection per loop()
        break;
      }
    }
  }

  for (auto *device : this->mesh_devices_) {
    if (!device->send_discovery && device->device_info_requested > 0 &&
        device->device_info_requested < esphome::millis() - 5000) {
      ESP_LOGD(TAG, "Request info again for %d", device->mesh_id);
      this->request_device_info(device);
    }
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

  for (Group *group : this->mesh_groups_) {
    if (!group->send_discovery && group->device_info != nullptr) {
      this->send_group_discovery(group);
    }
  }
}

void AwoxMesh::disconnect_connections_with_overlapping_mesh_ids() {
  const int connections_count = this->connections_.size();
  for (int i = 0; i < connections_count; i++) {
    if (this->connections_[i]->get_address() == 0) {
      continue;
    }
    if (i - 1 >= 0 && this->connections_[i - 1]->connected()) {
      this->disconnect_connection_with_overlapping_mesh_ids(i - 1, i);
    }
    if (i + 1 < connections_count && this->connections_[i + 1]->connected()) {
      this->disconnect_connection_with_overlapping_mesh_ids(i + 1, i);
    }
  }
}

void AwoxMesh::disconnect_connection_with_overlapping_mesh_ids(const int a, const int b) {
  if (!matching_mesh_ids(this->connections_[a]->get_linked_mesh_ids(), this->connections_[b]->get_linked_mesh_ids())) {
    return;
  }
  if (this->connections_[a]->get_linked_mesh_ids().size() > this->connections_[b]->get_linked_mesh_ids().size()) {
    ESP_LOGI(TAG, "Disconnect connection %d [%s] due to overlapping mesh_id's with other connection", b,
             this->connections_[b]->address_str().c_str());
    this->connections_[b]->clear_linked_mesh_ids();
    this->connections_[b]->disconnect();
  } else {
    ESP_LOGI(TAG, "Disconnect connection %d [%s] due to overlapping mesh_id's with other connection", a,
             this->connections_[a]->address_str().c_str());
    this->connections_[a]->clear_linked_mesh_ids();
    this->connections_[a]->disconnect();
  }
}

void AwoxMesh::sort_devices() {
  std::stable_sort(this->found_devices_.begin(), this->found_devices_.end(),
                   [](FoundDevice *a, FoundDevice *b) { return a->rssi > b->rssi; });
}

FoundDevice *AwoxMesh::next_to_connect() {
  for (auto *found_device : this->found_devices_) {
    if (found_device->mesh_id == 0) {
      Device *device = this->get_device(found_device->device.address_uint64());
      if (device != nullptr) {
        ESP_LOGD(TAG, "Set mesh_id %d for device %s", device->mesh_id, found_device->device.address_str().c_str());
        found_device->mesh_id = device->mesh_id;
      }
    }
  }

  ESP_LOGD(TAG, "Total devices: %d", this->found_devices_.size());
  for (auto *found_device : this->found_devices_) {
    ESP_LOGD(TAG, "Available device %s [%d] => rssi: %d", found_device->device.address_str().c_str(),
             found_device->mesh_id, found_device->rssi);
  }

  std::vector<int> linked_mesh_ids;
  for (auto *connection : this->connections_) {
    if (connection->connected()) {
      copy(connection->get_linked_mesh_ids().begin(), connection->get_linked_mesh_ids().end(),
           std::back_inserter(linked_mesh_ids));
    }
  }
  sort(linked_mesh_ids.begin(), linked_mesh_ids.end());
  linked_mesh_ids.erase(unique(linked_mesh_ids.begin(), linked_mesh_ids.end()), linked_mesh_ids.end());

  int known_mesh_devices = 0;
  int identified_mesh_devices = 0;
  for (auto *mesh_device : this->mesh_devices_) {
    known_mesh_devices++;
    if (mesh_device->product_id) {
      identified_mesh_devices++;
    }
  }

  ESP_LOGD(
      TAG,
      "Currently %d mesh devices reachable through active connections (%d currently known and %d fully recognized)",
      linked_mesh_ids.size(), known_mesh_devices, identified_mesh_devices);

  for (auto *found_device : this->found_devices_) {
    if (!found_device->connected && found_device->rssi >= this->minimum_rssi) {
      // unknown mesh_id then the device is definitly not in reach of our current connection
      if (found_device->mesh_id == 0) {
        ESP_LOGD(TAG, "Try to connecty to device %s no mesh id known yet", found_device->device.address_str().c_str());
        return found_device;
      }
      // No active connection for found device
      if (!id_in_vector(found_device->mesh_id, linked_mesh_ids)) {
        ESP_LOGD(TAG, "Try to connecty to device %s [%d] no active connection found for this device",
                 found_device->device.address_str().c_str(), found_device->mesh_id);
        return found_device;
      }
    }
  }

  return nullptr;
}

void AwoxMesh::set_rssi_for_devices_that_are_not_available() {
  const uint32_t now = esphome::millis();
  for (auto *found_device : this->found_devices_) {
    if (now - found_device->last_detected > 20000) {
      found_device->rssi = RSSI_NOT_AVAILABLE;
    }
  }
}

bool AwoxMesh::mac_addresses_allowed(const uint64_t address) {
  if (this->allowed_mac_addresses_.size() == 0) {
    return true;
  }

  std::vector<uint64_t>::iterator position =
      std::find(this->allowed_mac_addresses_.begin(), this->allowed_mac_addresses_.end(), address);

  return position != this->allowed_mac_addresses_.end();
}

bool AwoxMesh::mesh_id_allowed(int mesh_id) {
  if (this->allowed_mesh_ids_.size() == 0) {
    return true;
  }

  std::vector<int>::iterator position =
      std::find(this->allowed_mesh_ids_.begin(), this->allowed_mesh_ids_.end(), mesh_id);

  return position != this->allowed_mesh_ids_.end();
}

Device *AwoxMesh::get_device(const uint64_t address) {
  auto found = std::find_if(this->mesh_devices_.begin(), this->mesh_devices_.end(),
                            [address](const Device *_f) { return _f->address_uint64() == address; });

  if (found != mesh_devices_.end()) {
    return mesh_devices_.at(found - mesh_devices_.begin());
  }

  return nullptr;
}

Device *AwoxMesh::get_device(int mesh_id) {
  auto found = std::find_if(this->mesh_devices_.begin(), this->mesh_devices_.end(),
                            [mesh_id](const Device *_f) { return _f->mesh_id == mesh_id; });

  if (found != mesh_devices_.end()) {
    Device *ptr = mesh_devices_.at(found - mesh_devices_.begin());
    ESP_LOGV(TAG, "Found existing mesh_id: %d, Number of found mesh devices = %d", ptr->mesh_id,
             this->mesh_devices_.size());
    return ptr;
  }

  if (!this->mesh_id_allowed(mesh_id)) {
    ESP_LOGV(TAG, "Mesh_id: %d ignored, not part of allowed_mesh_ids", mesh_id);
    return nullptr;
  }

  Device *device = new Device;
  device->mesh_id = mesh_id;
  this->mesh_devices_.push_back(device);

  ESP_LOGI(TAG, "Added mesh_id: %d, Number of found mesh devices = %d", device->mesh_id, this->mesh_devices_.size());

  this->request_device_info(device);

  return device;
}

Group *AwoxMesh::get_group(int dest, Device *device) {
  ESP_LOGVV(TAG, "get group_id: %d", dest);

  auto found = std::find_if(this->mesh_groups_.begin(), this->mesh_groups_.end(),
                            [dest](const Group *_f) { return _f->group_id == dest; });

  if (found != mesh_groups_.end()) {
    Group *group = mesh_groups_.at(found - mesh_groups_.begin());
    ESP_LOGV(TAG, "Found existing group_id: %d, Number of found mesh groups = %d", group->group_id,
             this->mesh_groups_.size());

    group->add_device(device);
    device->add_group(group);

    this->sync_and_publish_group_state(group);
    return group;
  }

  Group *group = new Group;
  group->group_id = dest;
  group->device_info = device->device_info;

  group->add_device(device);
  device->add_group(group);

  this->mesh_groups_.push_back(group);

  ESP_LOGI(TAG, "Added group_id: %d, Number of found mesh groups = %d", dest, this->mesh_groups_.size());

  this->sync_and_publish_group_state(group);

  return group;
}

void AwoxMesh::publish_connected() {
  this->has_active_connection = false;
  int active_connections = 0;
  int online_devices = 0;

  std::vector<int> linked_mesh_ids;
  for (auto *connection : this->connections_) {
    if (connection->connected()) {
      active_connections++;
      this->has_active_connection = true;
      copy(connection->get_linked_mesh_ids().begin(), connection->get_linked_mesh_ids().end(),
           std::back_inserter(linked_mesh_ids));
    }
  }
  sort(linked_mesh_ids.begin(), linked_mesh_ids.end());
  linked_mesh_ids.erase(unique(linked_mesh_ids.begin(), linked_mesh_ids.end()), linked_mesh_ids.end());
  online_devices = linked_mesh_ids.size();

  const std::string message = this->has_active_connection ? "online" : "offline";
  ESP_LOGI(TAG, "Publish connected to mesh device - %s", message.c_str());
  global_mqtt_client->publish(global_mqtt_client->get_topic_prefix() + "/connected", message, 0, true);

  global_mqtt_client->publish_json(
      global_mqtt_client->get_topic_prefix() + "/connection_status",
      [&](JsonObject root) {
        root["now"] = esphome::millis();
        root["active_connections"] = active_connections;
        root["online_devices"] = online_devices;

        for (int i = 0; i < this->connections_.size(); i++) {
          JsonObject connection = root.createNestedObject("connection_" + std::to_string(i));
          connection["connected"] = this->connections_[i]->connected();
          connection["mac"] = this->connections_[i]->connected() ? this->connections_[i]->address_str() : "";
          connection["mesh_id"] =
              this->connections_[i]->connected() ? std::to_string(this->connections_[i]->mesh_id()) : "";
          connection["devices"] = this->connections_[i]->get_linked_mesh_ids().size();

          std::stringstream mesh_ids;
          std::copy(this->connections_[i]->get_linked_mesh_ids().begin(),
                    this->connections_[i]->get_linked_mesh_ids().end(), std::ostream_iterator<int>(mesh_ids, ", "));
          connection["mesh_ids"] = mesh_ids.str().substr(0, mesh_ids.str().size() - 2);
        }
      },
      0, false);
}

void AwoxMesh::publish_availability(Device *device, bool delayed) {
  if (delayed) {
    PublishOnlineStatus publish = {};
    publish.device = device;
    publish.online = device->online;
    publish.time = esphome::millis();
    this->delayed_availability_publish.push_back(publish);
    ESP_LOGD(TAG, "Delayed publish online/offline for %d - %s", device->mesh_id, device->online ? "online" : "offline");

    // Force info update request
    this->request_status_update(device->mesh_id);

    return;
  }

  const std::string message = device->online ? "online" : "offline";
  ESP_LOGI(TAG, "Publish online/offline for %d - %s", device->mesh_id, message.c_str());
  global_mqtt_client->publish(this->get_mqtt_topic_for_(device, "availability"), message, 0, true);
}

void AwoxMesh::publish_availability(Group *group) {
  const std::string message = group->online ? "online" : "offline";
  ESP_LOGI(TAG, "Publish online/offline for group %d - %s", group->group_id, message.c_str());
  global_mqtt_client->publish(this->get_mqtt_topic_for_(group, "availability"), message, 0, true);
}

void AwoxMesh::sync_and_publish_group_state(Group *group) {
  bool online = false;
  bool state = false;

  for (Device *device : group->get_devices()) {
    if (device->online) {
      online = true;
    }
    if (device->state) {
      state = true;
    }
    if (online && state) {
      break;
    }
  }

  group->state = state;
  group->online = online;

  this->publish_state(group);
}

void AwoxMesh::publish_state(Device *device) {
  if (!device->address_set()) {
    ESP_LOGW(TAG, "'%s': Can not yet send publish state, mac address not known...",
             std::to_string(device->mesh_id).c_str());
    return;
  }
  if (device->device_info->has_feature(FEATURE_LIGHT_MODE)) {
    global_mqtt_client->publish_json(
        this->get_mqtt_topic_for_(device, "state"),
        [this, device](JsonObject root) {
          root["state"] = device->state ? "ON" : "OFF";

          if (device->candle_mode) {
            root["effect"] = "candle";
          } else if (device->sequence_mode) {
            root["effect"] = "color loop";
          }

          if (device->color_mode) {
            root["color_mode"] = "rgb";
            root["brightness"] = convert_value_to_available_range(device->color_brightness, 0xa, 0x64, 0, 255);
          } else {
            if (device->device_info->has_feature(FEATURE_WHITE_TEMPERATURE)) {
              root["color_mode"] = "color_temp";
              root["color_temp"] = convert_value_to_available_range(device->temperature, 0, 0x7f, 153, 370);
            } else {
              root["color_mode"] = "brightness";
            }
            root["brightness"] = convert_value_to_available_range(device->white_brightness, 1, 0x7f, 0, 255);
          }
          JsonObject color = root.createNestedObject("color");
          color["r"] = device->R;
          color["g"] = device->G;
          color["b"] = device->B;
        },
        0, true);
  } else {
    global_mqtt_client->publish(this->get_mqtt_topic_for_(device, "state"), device->state ? "ON" : "OFF",
                                device->state ? 2 : 3, 0, true);
  }

  for (Group *group : device->get_groups()) {
    this->sync_and_publish_group_state(group);
  }
}

void AwoxMesh::publish_state(Group *group) {
  if (group->device_info == nullptr) {
    ESP_LOGW(TAG, "group %d: Can not yet send publish state, device info not known...", group->group_id);
    return;
  }
  if (group->device_info->has_feature(FEATURE_LIGHT_MODE)) {
    global_mqtt_client->publish_json(
        this->get_mqtt_topic_for_(group, "state"),
        [this, group](JsonObject root) {
          root["state"] = group->state ? "ON" : "OFF";

          if (group->candle_mode) {
            root["effect"] = "candle";
          } else if (group->sequence_mode) {
            root["effect"] = "color loop";
          }

          root["color_mode"] = "color_temp";

          root["brightness"] = convert_value_to_available_range(group->white_brightness, 1, 0x7f, 0, 255);

          if (group->color_mode) {
            root["color_mode"] = "rgb";
            root["brightness"] = convert_value_to_available_range(group->color_brightness, 0xa, 0x64, 0, 255);
          } else {
            root["color_temp"] = convert_value_to_available_range(group->temperature, 0, 0x7f, 153, 370);
          }
          JsonObject color = root.createNestedObject("color");
          color["r"] = group->R;
          color["g"] = group->G;
          color["b"] = group->B;
        },
        0, true);
  } else {
    global_mqtt_client->publish(this->get_mqtt_topic_for_(group, "state"), group->state ? "ON" : "OFF",
                                group->state ? 2 : 3, 0, true);
  }
}

void AwoxMesh::publish_connection_sensor_discovery() {
  const MQTTDiscoveryInfo &discovery_info = global_mqtt_client->get_discovery_info();
  const std::string sanitized_name = str_sanitize(App.get_name());

  for (int i = 0; i < this->connections_.size(); i++) {
    global_mqtt_client->publish_json(
        discovery_info.prefix + "/sensor/" + sanitized_name + "/connection-" + std::to_string(i) + "-devices/config",
        [this, i, discovery_info](JsonObject root) {
          // Entity
          root[MQTT_NAME] = "Connection " + std::to_string(i) + " devices";
          root[MQTT_UNIQUE_ID] = "awox-connection-" + std::to_string(i) + "-devices";
          root[MQTT_ENTITY_CATEGORY] = "diagnostic";
          root[MQTT_ICON] = "mdi:counter";
          root[MQTT_ENABLED_BY_DEFAULT] = false;

          // State and command topic
          root[MQTT_STATE_TOPIC] = global_mqtt_client->get_topic_prefix() + "/connection_status";

          root[MQTT_AVAILABILITY_TOPIC] = global_mqtt_client->get_topic_prefix() + "/status";
          root[MQTT_VALUE_TEMPLATE] = "{{ value_json.connection_" + std::to_string(i) + ".devices }}";

          // Device
          JsonObject device_info = root.createNestedObject(MQTT_DEVICE);
          device_info[MQTT_DEVICE_IDENTIFIERS] = get_mac_address();
        },
        0, discovery_info.retain);

    global_mqtt_client->publish_json(
        discovery_info.prefix + "/sensor/" + sanitized_name + "/connection-" + std::to_string(i) + "-mesh-ids/config",
        [this, i, discovery_info](JsonObject root) {
          // Entity
          root[MQTT_NAME] = "Connection " + std::to_string(i) + " Mesh ID's";
          root[MQTT_UNIQUE_ID] = "awox-connection-" + std::to_string(i) + "-mesh-ids";
          root[MQTT_ENTITY_CATEGORY] = "diagnostic";
          root[MQTT_ICON] = "mdi:vector-polyline";
          root[MQTT_ENABLED_BY_DEFAULT] = false;

          // State and command topic
          root[MQTT_STATE_TOPIC] = global_mqtt_client->get_topic_prefix() + "/connection_status";

          root[MQTT_AVAILABILITY_TOPIC] = global_mqtt_client->get_topic_prefix() + "/status";
          root[MQTT_VALUE_TEMPLATE] = "{{ value_json.connection_" + std::to_string(i) + ".mesh_ids }}";

          // Device
          JsonObject device_info = root.createNestedObject(MQTT_DEVICE);
          device_info[MQTT_DEVICE_IDENTIFIERS] = get_mac_address();
        },
        0, discovery_info.retain);

    global_mqtt_client->publish_json(
        discovery_info.prefix + "/sensor/" + sanitized_name + "/connection-" + std::to_string(i) + "-mesh-id/config",
        [this, i, discovery_info](JsonObject root) {
          // Entity
          root[MQTT_NAME] = "Connection " + std::to_string(i) + " Mesh ID";
          root[MQTT_UNIQUE_ID] = "awox-connection-" + std::to_string(i) + "-mesh-id";
          root[MQTT_ENTITY_CATEGORY] = "diagnostic";
          root[MQTT_ICON] = "mdi:vector-point-select";
          root[MQTT_ENABLED_BY_DEFAULT] = false;

          // State and command topic
          root[MQTT_STATE_TOPIC] = global_mqtt_client->get_topic_prefix() + "/connection_status";

          root[MQTT_AVAILABILITY_TOPIC] = global_mqtt_client->get_topic_prefix() + "/status";
          root[MQTT_VALUE_TEMPLATE] = "{{ value_json.connection_" + std::to_string(i) + ".mesh_id }}";

          // Device
          JsonObject device_info = root.createNestedObject(MQTT_DEVICE);
          device_info[MQTT_DEVICE_IDENTIFIERS] = get_mac_address();
        },
        0, discovery_info.retain);

    global_mqtt_client->publish_json(
        discovery_info.prefix + "/sensor/" + sanitized_name + "/connection-" + std::to_string(i) + "-mac/config",
        [this, i, discovery_info](JsonObject root) {
          // Entity
          root[MQTT_NAME] = "Connection " + std::to_string(i) + " mac address";
          root[MQTT_UNIQUE_ID] = "awox-connection-" + std::to_string(i) + "-mac";
          root[MQTT_ENTITY_CATEGORY] = "diagnostic";
          root[MQTT_ICON] = "mdi:information";
          root[MQTT_ENABLED_BY_DEFAULT] = false;

          // State and command topic
          root[MQTT_STATE_TOPIC] = global_mqtt_client->get_topic_prefix() + "/connection_status";

          root[MQTT_AVAILABILITY_TOPIC] = global_mqtt_client->get_topic_prefix() + "/status";
          root[MQTT_VALUE_TEMPLATE] = "{{ value_json.connection_" + std::to_string(i) + ".mac }}";

          // Device
          JsonObject device_info = root.createNestedObject(MQTT_DEVICE);
          device_info[MQTT_DEVICE_IDENTIFIERS] = get_mac_address();
        },
        0, discovery_info.retain);

    global_mqtt_client->publish_json(
        discovery_info.prefix + "/binary_sensor/" + sanitized_name + "/connection-" + std::to_string(i) +
            "-connected/config",
        [this, i, discovery_info](JsonObject root) {
          // Entity
          root[MQTT_NAME] = "Connection " + std::to_string(i) + " connected";
          root[MQTT_UNIQUE_ID] = "awox-connection-" + std::to_string(i) + "-connected";
          root[MQTT_ENTITY_CATEGORY] = "diagnostic";
          root[MQTT_ICON] = "mdi:connection";

          root[MQTT_AVAILABILITY_TOPIC] = global_mqtt_client->get_topic_prefix() + "/status";

          // State and command topic
          root[MQTT_STATE_TOPIC] = global_mqtt_client->get_topic_prefix() + "/connection_status";

          root[MQTT_PAYLOAD_ON] = true;
          root[MQTT_PAYLOAD_OFF] = false;
          root[MQTT_VALUE_TEMPLATE] = "{{ value_json.connection_" + std::to_string(i) + ".connected }}";

          // Device
          JsonObject device_info = root.createNestedObject(MQTT_DEVICE);
          device_info[MQTT_DEVICE_IDENTIFIERS] = get_mac_address();
        },
        0, discovery_info.retain);
  }
}

void AwoxMesh::send_discovery(Device *device) {
  if (!device->address_set()) {
    ESP_LOGW(TAG, "'%s': Can not yet send discovery, mac address not known...",
             std::to_string(device->mesh_id).c_str());
    return;
  }

  device->device_info = this->device_info_resolver->get_by_product_id(device->product_id);

  ESP_LOGD(TAG, "[%d]: Sending discovery productID: 0x%02X (%s - %s) mac: %s", device->mesh_id,
           device->device_info->get_product_id(), device->device_info->get_name(), device->device_info->get_model(),
           device->address_str().c_str());

  const MQTTDiscoveryInfo &discovery_info = global_mqtt_client->get_discovery_info();
  device->send_discovery = true;

  global_mqtt_client->publish_json(
      this->get_discovery_topic_(discovery_info, device),
      [this, device, discovery_info](JsonObject root) {
        root["schema"] = "json";

        // Entity
        root[MQTT_NAME] = nullptr;
        root[MQTT_UNIQUE_ID] = "awox-" + device->address_str() + "-" + device->device_info->get_component_type();

        if (strlen(device->device_info->get_icon()) > 0) {
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

          root["effect"] = true;
          JsonArray effect_list = root.createNestedArray(MQTT_EFFECT_LIST);
          effect_list.add("candle");
          effect_list.add("color loop");
          effect_list.add("stop");
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
        identifiers.add(device->address_str());

        device_info[MQTT_DEVICE_NAME] = device->device_info->get_name();

        if (strlen(device->device_info->get_model()) == 0) {
          device_info[MQTT_DEVICE_MODEL] = get_product_code_as_hex_string(device->device_info->get_product_id()) +
                                           " (" + std::to_string(device->mesh_id) + ")";
        } else {
          device_info[MQTT_DEVICE_MODEL] =
              std::string(device->device_info->get_model()) + " (" + std::to_string(device->mesh_id) + ")";
        }
        device_info[MQTT_DEVICE_MANUFACTURER] = device->device_info->get_manufacturer();
        device_info["via_device"] = get_mac_address();
        // device_info["serial_number"] = "mesh-id: " + std::to_string(device->mesh_id);
      },
      0, discovery_info.retain);

  if (device->device_info->has_feature(FEATURE_LIGHT_MODE)) {
    global_mqtt_client->subscribe_json(
        this->get_mqtt_topic_for_(device, "command"),
        [this, device](const std::string &topic, JsonObject root) { this->process_incomming_command(device, root); });
  } else {
    global_mqtt_client->subscribe(this->get_mqtt_topic_for_(device, "command"),
                                  [this, device](const std::string &topic, const std::string &payload) {
                                    ESP_LOGI(TAG, "command %s - %s", topic.c_str(), payload.c_str());
                                    auto val = parse_on_off(payload.c_str());
                                    switch (val) {
                                      case PARSE_ON:
                                        device->state = true;
                                        this->set_power(device->mesh_id, true);
                                        break;
                                      case PARSE_OFF:
                                        device->state = false;
                                        this->set_power(device->mesh_id, false);
                                        break;
                                      case PARSE_TOGGLE:
                                        device->state = !device->state;
                                        this->set_power(device->mesh_id, device->state);
                                        break;
                                      case PARSE_NONE:
                                        break;
                                    }
                                  });
  }
  this->publish_availability(device, true);
}

void AwoxMesh::send_group_discovery(Group *group) {
  if (group->device_info == nullptr) {
    ESP_LOGW(TAG, "'%s': Can not yet send discovery, component_type not known...",
             std::to_string(group->group_id).c_str());
    return;
  }

  const MQTTDiscoveryInfo &discovery_info = global_mqtt_client->get_discovery_info();
  group->send_discovery = true;

  global_mqtt_client->publish_json(
      discovery_info.prefix + "/" + group->device_info->get_component_type() + "/group-" +
          std::to_string(group->group_id) + "/config",
      [this, group, discovery_info](JsonObject root) {
        root["schema"] = "json";
        // Entity
        root[MQTT_NAME] = nullptr;
        root[MQTT_UNIQUE_ID] = "group-" + std::to_string(group->group_id);

        root[MQTT_ICON] = "mdi:lightbulb-group";

        // State and command topic
        root[MQTT_STATE_TOPIC] = this->get_mqtt_topic_for_(group, "state");
        root[MQTT_COMMAND_TOPIC] = this->get_mqtt_topic_for_(group, "command");

        // Availavility topics
        JsonArray availability = root.createNestedArray(MQTT_AVAILABILITY);
        auto availability_topic_1 = availability.createNestedObject();
        availability_topic_1[MQTT_TOPIC] = this->get_mqtt_topic_for_(group, "availability");
        auto availability_topic_2 = availability.createNestedObject();
        availability_topic_2[MQTT_TOPIC] = global_mqtt_client->get_topic_prefix() + "/status";
        auto availability_topic_3 = availability.createNestedObject();
        availability_topic_3[MQTT_TOPIC] = global_mqtt_client->get_topic_prefix() + "/connected";
        root[MQTT_AVAILABILITY_MODE] = "all";

        // Features
        root[MQTT_COLOR_MODE] = true;

        if (group->device_info->has_feature(FEATURE_WHITE_BRIGHTNESS) ||
            group->device_info->has_feature(FEATURE_COLOR_BRIGHTNESS)) {
          root["brightness"] = true;
          root["brightness_scale"] = 255;
        }

        JsonArray color_modes = root.createNestedArray("supported_color_modes");

        if (group->device_info->has_feature(FEATURE_COLOR)) {
          color_modes.add("rgb");

          root["effect"] = true;
          JsonArray effect_list = root.createNestedArray(MQTT_EFFECT_LIST);
          effect_list.add("candle");
          effect_list.add("color loop");
          effect_list.add("stop");
        }

        if (group->device_info->has_feature(FEATURE_WHITE_TEMPERATURE)) {
          color_modes.add("color_temp");

          root[MQTT_MIN_MIREDS] = 153;
          root[MQTT_MAX_MIREDS] = 370;
        }

        // brightness should always be used alone
        // https://developers.home-assistant.io/docs/core/entity/light/#color-modes
        if (color_modes.size() == 0 && group->device_info->has_feature(FEATURE_WHITE_BRIGHTNESS)) {
          color_modes.add("brightness");
        }

        if (color_modes.size() == 0) {
          color_modes.add("onoff");
        }

        // Device
        JsonObject device_info = root.createNestedObject(MQTT_DEVICE);

        JsonArray identifiers = device_info.createNestedArray(MQTT_DEVICE_IDENTIFIERS);
        identifiers.add("esp-awox-mesh-group-" + std::to_string(group->group_id));

        device_info[MQTT_DEVICE_MODEL] = "Group - " + std::to_string(group->group_id);
        device_info[MQTT_DEVICE_MANUFACTURER] = "ESPHome AwoX BLE mesh";

        device_info[MQTT_DEVICE_NAME] = "Group " + std::to_string(group->group_id);

        device_info["via_device"] = get_mac_address();
        // device_info["serial_number"] = "group-id: " + std::to_string(group->group_id);
      },
      0, discovery_info.retain);

  if (group->device_info->has_feature(FEATURE_LIGHT_MODE)) {
    global_mqtt_client->subscribe_json(
        this->get_mqtt_topic_for_(group, "command"),
        [this, group](const std::string &topic, JsonObject root) { this->process_incomming_command(group, root); });
  } else {
    ESP_LOGE(TAG, "Non light group isn't supported currently");
  }
  this->publish_availability(group);
}

void AwoxMesh::process_incomming_command(Device *device, JsonObject root) {
  ESP_LOGV(TAG, "[%d] Process command", device->mesh_id);
  bool state_set = false;

  if (root.containsKey("fade_duration")) {
    ESP_LOGD(TAG, "[%d] set sequence fade_duration %d", device->mesh_id, (int) root["fade_duration"]);
    this->set_sequence_fade_duration(device->mesh_id, (int) root["fade_duration"]);
  }

  if (root.containsKey("color_duration")) {
    ESP_LOGD(TAG, "[%d] set sequence color_duration %d", device->mesh_id, (int) root["color_duration"]);
    this->set_sequence_color_duration(device->mesh_id, (int) root["color_duration"]);
  }

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

  if (root.containsKey("effect")) {
    state_set = true;

    if (root["effect"] == "color loop") {
      ESP_LOGD(TAG, "[%d] Effect command %s", device->mesh_id, "color loop");
      this->set_sequence(device->mesh_id, 0);
    } else if (root["effect"] == "candle") {
      ESP_LOGD(TAG, "[%d] Effect command %s", device->mesh_id, "candle");
      this->set_candle_mode(device->mesh_id);
    } else {
      if (device->color_mode) {
        this->set_color(device->mesh_id, device->R, device->G, device->B);
      } else {
        this->set_white_temperature(device->mesh_id, device->temperature);
      }
    }
  }

  if (root.containsKey("state")) {
    ESP_LOGD(TAG, "[%d] Process command state", device->mesh_id);
    auto val = parse_on_off(root["state"]);
    switch (val) {
      case PARSE_ON:
        device->state = true;
        if (!state_set) {
          this->set_power(device->mesh_id, true);
        }
        break;
      case PARSE_OFF:
        device->state = false;
        this->set_power(device->mesh_id, false);
        break;
      case PARSE_TOGGLE:
        device->state = !device->state;
        this->set_power(device->mesh_id, device->state);
        break;
      case PARSE_NONE:
        break;
    }
  }

  this->publish_state(device);
}

void AwoxMesh::process_incomming_command(Group *group, JsonObject root) {
  ESP_LOGD(TAG, "[%d] Process command group", group->group_id);
  int dest = group->group_id + 0x8000;
  bool state_set = false;

  if (root.containsKey("fade_duration")) {
    ESP_LOGD(TAG, "[%d] set sequence fade_duration %d", dest, (int) root["fade_duration"]);
    this->set_sequence_fade_duration(dest, (int) root["fade_duration"]);
  }

  if (root.containsKey("color_duration")) {
    ESP_LOGD(TAG, "[%d] set sequence color_duration %d", dest, (int) root["color_duration"]);
    this->set_sequence_color_duration(dest, (int) root["color_duration"]);
  }

  if (root.containsKey("color")) {
    JsonObject color = root["color"];

    state_set = true;
    group->state = true;
    group->color_mode = true;
    group->R = (int) color["r"];
    group->G = (int) color["g"];
    group->B = (int) color["b"];

    ESP_LOGD(TAG, "[%d] Process group command color %d %d %d", group->group_id, (int) color["r"], (int) color["g"],
             (int) color["b"]);

    this->set_color(dest, (int) color["r"], (int) color["g"], (int) color["b"]);
  }

  if (root.containsKey("brightness") && !root.containsKey("color_temp") &&
      (root.containsKey("color") || group->color_mode)) {
    int brightness = convert_value_to_available_range((int) root["brightness"], 0, 255, 0xa, 0x64);

    state_set = true;
    group->state = true;
    group->color_brightness = brightness;

    ESP_LOGD(TAG, "[%d] Process group command color_brightness %d", dest, (int) root["brightness"]);
    this->set_color_brightness(dest, brightness);

  } else if (root.containsKey("brightness")) {
    int brightness = convert_value_to_available_range((int) root["brightness"], 0, 255, 1, 0x7f);

    state_set = true;
    group->state = true;
    group->white_brightness = brightness;

    ESP_LOGD(TAG, "[%d] Process group command white_brightness %d", dest, (int) root["brightness"]);
    this->set_white_brightness(dest, brightness);
  }

  if (root.containsKey("color_temp")) {
    int temperature = convert_value_to_available_range((int) root["color_temp"], 153, 370, 0, 0x7f);

    state_set = true;
    group->state = true;
    group->color_mode = false;
    group->temperature = temperature;

    ESP_LOGD(TAG, "[%d] Process group command color_temp %d", dest, (int) root["color_temp"]);
    this->set_white_temperature(dest, temperature);
  }

  if (root.containsKey("effect")) {
    state_set = true;
    group->state = true;
    group->sequence_mode = false;
    group->candle_mode = false;
    if (root["effect"] == "color loop") {
      group->sequence_mode = true;
      this->set_sequence(dest, 0);
    } else if (root["effect"] == "candle") {
      group->candle_mode = true;
      this->set_candle_mode(dest);
    } else {
      if (group->color_mode) {
        this->set_color(dest, group->R, group->G, group->B);
      } else {
        this->set_white_temperature(dest, group->temperature);
      }
    }
  }

  if (root.containsKey("state")) {
    ESP_LOGD(TAG, "[%d] Process group command state", dest);
    auto val = parse_on_off(root["state"]);
    switch (val) {
      case PARSE_ON:
        group->state = true;
        if (!state_set) {
          this->set_power(dest, true);
        }
        break;
      case PARSE_OFF:
        group->state = false;
        this->set_power(dest, false);
        break;
      case PARSE_TOGGLE:
        group->state = !group->state;
        this->set_power(dest, group->state);
        break;
      case PARSE_NONE:
        break;
    }
  }

  this->publish_state(group);
}

std::string AwoxMesh::get_discovery_topic_(const MQTTDiscoveryInfo &discovery_info, Device *device) const {
  return discovery_info.prefix + "/" + device->device_info->get_component_type() + "/awox-" +
         device->address_str_hex_only() + "/config";
}

std::string AwoxMesh::get_mqtt_topic_for_(Device *device, const std::string &suffix) const {
  return global_mqtt_client->get_topic_prefix() + "/" + std::to_string(device->mesh_id) + "/" + suffix;
}

std::string AwoxMesh::get_mqtt_topic_for_(Group *group, const std::string &suffix) const {
  return global_mqtt_client->get_topic_prefix() + "/group-" + std::to_string(group->group_id) + "/" + suffix;
}

void AwoxMesh::call_connection(int dest, std::function<void(MeshConnection *)> &&callback) {
  ESP_LOGD(TAG, "Call connection for %d", dest);
  for (auto *connection : this->connections_) {
    // todo: when supporting groups dest doesn't have to be a linked mesh_id
    if (connection->get_address() > 0 && connection->mesh_id_linked(dest)) {
      ESP_LOGD(TAG, "Found %s as connection", connection->address_str().c_str());
      callback(connection);

      // for now we stop on first
      return;
    }
  }

  ESP_LOGI(TAG, "No active connection for %d, we trigger message on all", dest);
  for (auto *connection : this->connections_) {
    if (connection->connected()) {
      callback(connection);
    }
  }
}

void AwoxMesh::request_device_info(Device *device) {
  this->call_connection(device->mesh_id, [device](MeshConnection *connection) {
    device->device_info_requested = esphome::millis();
    connection->request_device_info(device);
    connection->request_group_info(device);
    // connection->request_device_version(device->mesh_id);
  });
}

void AwoxMesh::set_power(int dest, bool state) {
  this->call_connection(dest, [dest, state](MeshConnection *connection) { connection->set_power(dest, state); });
}

void AwoxMesh::set_color(int dest, int red, int green, int blue) {
  this->call_connection(
      dest, [dest, red, green, blue](MeshConnection *connection) { connection->set_color(dest, red, green, blue); });
}

void AwoxMesh::set_color_brightness(int dest, int brightness) {
  this->call_connection(
      dest, [dest, brightness](MeshConnection *connection) { connection->set_color_brightness(dest, brightness); });
}

void AwoxMesh::set_white_brightness(int dest, int brightness) {
  this->call_connection(
      dest, [dest, brightness](MeshConnection *connection) { connection->set_white_brightness(dest, brightness); });
}

void AwoxMesh::set_white_temperature(int dest, int temp) {
  this->call_connection(dest,
                        [dest, temp](MeshConnection *connection) { connection->set_white_temperature(dest, temp); });
}

void AwoxMesh::set_sequence(int dest, int preset) {
  this->call_connection(dest, [dest, preset](MeshConnection *connection) { connection->set_sequence(dest, preset); });
}

void AwoxMesh::set_candle_mode(int dest) {
  this->call_connection(dest, [dest](MeshConnection *connection) { connection->set_candle_mode(dest); });
}

void AwoxMesh::set_sequence_fade_duration(int dest, int duration) {
  this->call_connection(
      dest, [dest, duration](MeshConnection *connection) { connection->set_sequence_fade_duration(dest, duration); });
}

void AwoxMesh::set_sequence_color_duration(int dest, int duration) {
  this->call_connection(
      dest, [dest, duration](MeshConnection *connection) { connection->set_sequence_color_duration(dest, duration); });
}

void AwoxMesh::request_status_update(int dest) {
  this->call_connection(dest, [dest](MeshConnection *connection) { connection->request_status_update(dest); });
}

}  // namespace awox_mesh
}  // namespace esphome

#endif
