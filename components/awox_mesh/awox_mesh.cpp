#ifdef USE_ESP32
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <math.h>
#include <regex>
#include "awox_mesh.h"
#include "esphome/components/mqtt/mqtt_const.h"
#include "esphome/components/mqtt/mqtt_component.h"

#include "esphome/core/log.h"

namespace esphome {
namespace awox_mesh {

static const char *const TAG = "awox.mesh";

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

static std::string str_sanitize_macadres(const std::string &str) {
  std::string out;
  std::copy_if(str.begin(), str.end(), std::back_inserter(out), [](const char &c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
  });
  return out;
}

FoundDevice *AwoxMesh::add_to_found_devices(const esp32_ble_tracker::ESPBTDevice &device) {
  FoundDevice *found_device;

  auto found = std::find_if(this->found_devices_.begin(), this->found_devices_.end(),
                            [device](const FoundDevice *_f) { return _f->address == device.address_uint64(); });

  if (found != found_devices_.end()) {
    found_device = found_devices_.at(found - found_devices_.begin());
    ESP_LOGV(TAG, "Found existing device: %s", found_device->address_str.c_str());
  } else {
    ESP_LOGV(TAG, "Register device: %s", device.address_str().c_str());
    found_device = new FoundDevice();
    found_device->address_str = device.address_str();
    found_device->address = device.address_uint64();
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
  this->publish_connected();
}

bool AwoxMesh::start_up_delay_done() {
  return esphome::millis() - this->start > 5000 && this->found_devices_.size() > 0;
}

void AwoxMesh::loop() {
  if (!ready_to_connect && this->start_up_delay_done()) {
    ready_to_connect = true;

    // Stop listening on all incomming topics (used in setup() to publish offline for each device)
    // todo: move to mqtt class
    global_mqtt_client->unsubscribe(global_mqtt_client->get_topic_prefix() + "/#");

    return;
  }

  if (!ready_to_connect) {
    return;
  }

  if (!this->has_active_connection || esphome::millis() - this->last_connection_attempt < 20000) {
    this->last_connection_attempt = esphome::millis();

    for (auto *connection : this->connections_) {
      if (connection->get_address() == 0) {
        auto *found_device = this->next_to_connect();

        if (found_device == nullptr) {
          ESP_LOGD(TAG, "No devices found to connect to");
          break;
        }

        if (found_device->connected) {
          ESP_LOGI(TAG, "Skipped to connect %s => rssi: %d already connected!!", found_device->address_str.c_str(),
                   found_device->rssi);
          break;
        }

        ESP_LOGI(TAG, "Try to connect %s => rssi: %d", found_device->address_str.c_str(), found_device->rssi);

        connection->connect_to(found_device);

        this->set_timeout("connecting", 20000, [this, found_device, connection]() {
          if (connection->connected()) {
            return;
          }
          ESP_LOGI(TAG, "Failed to connect %s => rssi: %d", found_device->address_str.c_str(), found_device->rssi);
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
}

void AwoxMesh::sort_devices() {
  std::stable_sort(this->found_devices_.begin(), this->found_devices_.end(),
                   [](FoundDevice *a, FoundDevice *b) { return a->rssi > b->rssi; });
}

FoundDevice *AwoxMesh::next_to_connect() {
  ESP_LOGD(TAG, "Total devices: %d", this->found_devices_.size());
  for (auto *found_device : this->found_devices_) {
    ESP_LOGD(TAG, "Available device %s => rssi: %d", found_device->address_str.c_str(), found_device->rssi);
  }

  for (auto *found_device : this->found_devices_) {
    if (!found_device->connected && found_device->rssi > -9999) {
      return found_device;
    }
  }

  return nullptr;
}

void AwoxMesh::set_rssi_for_devices_that_are_not_available() {
  const uint32_t now = esphome::millis();
  for (auto *found_device : this->found_devices_) {
    if (now - found_device->last_detected > 20000) {
      found_device->rssi = -9999;
    }
  }
}

Device *AwoxMesh::get_device(int mesh_id) {
  ESP_LOGVV(TAG, "get device %d", mesh_id);

  auto found = std::find_if(this->mesh_devices_.begin(), this->mesh_devices_.end(),
                            [mesh_id](const Device *_f) { return _f->mesh_id == mesh_id; });

  if (found != mesh_devices_.end()) {
    Device *ptr = mesh_devices_.at(found - mesh_devices_.begin());
    ESP_LOGV(TAG, "Found existing mesh_id: %d, Number of found mesh devices = %d", ptr->mesh_id,
             this->mesh_devices_.size());
    return ptr;
  }

  Device *device = new Device;
  device->mesh_id = mesh_id;
  this->mesh_devices_.push_back(device);

  ESP_LOGI(TAG, "Added mesh_id: %d, Number of found mesh devices = %d", device->mesh_id, this->mesh_devices_.size());

  this->request_device_info(device);

  return device;
}

void AwoxMesh::publish_connected() {
  this->has_active_connection = false;

  for (auto *connection : this->connections_) {
    if (connection->connected()) {
      this->has_active_connection = true;
    }
  }

  const std::string message = this->has_active_connection ? "online" : "offline";
  ESP_LOGI(TAG, "Publish connected to mesh device - %s", message.c_str());
  global_mqtt_client->publish(global_mqtt_client->get_topic_prefix() + "/connected", message, 0, true);
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

void AwoxMesh::publish_state(Device *device) {
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
    global_mqtt_client->publish(this->get_mqtt_topic_for_(device, "state"), device->state ? "ON" : "OFF",
                                device->state ? 2 : 3);
  }
}

void AwoxMesh::send_discovery(Device *device) {
  if (device->mac == "") {
    ESP_LOGW(TAG, "'%s': Can not yet send discovery, mac address not known...",
             std::to_string(device->mesh_id).c_str());
    return;
  }

  device->device_info = this->device_info_resolver->get_by_product_id(device->product_id);

  ESP_LOGD(TAG, "[%d]: Sending discovery productID: 0x%02X (%s - %s) mac: %s", device->mesh_id,
           device->device_info->get_product_id(), device->device_info->get_name(), device->device_info->get_model(),
           device->mac.c_str());

  const MQTTDiscoveryInfo &discovery_info = global_mqtt_client->get_discovery_info();
  device->send_discovery = true;

  global_mqtt_client->publish_json(
      this->get_discovery_topic_(discovery_info, device),
      [this, device, discovery_info](JsonObject root) {
        root["schema"] = "json";

        // Entity
        root[MQTT_NAME] = nullptr;
        root[MQTT_UNIQUE_ID] = "awox-" + device->mac + "-" + device->device_info->get_component_type();

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

        device_info[MQTT_DEVICE_NAME] = device->device_info->get_name();

        if (strlen(device->device_info->get_model()) == 0) {
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

void AwoxMesh::process_incomming_command(Device *device, JsonObject root) {
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

std::string AwoxMesh::get_discovery_topic_(const MQTTDiscoveryInfo &discovery_info, Device *device) const {
  return discovery_info.prefix + "/" + device->device_info->get_component_type() + "/awox-" +
         str_sanitize_macadres(device->mac) + "/config";
}

std::string AwoxMesh::get_mqtt_topic_for_(Device *device, const std::string &suffix) const {
  return global_mqtt_client->get_topic_prefix() + "/" + std::to_string(device->mesh_id) + "/" + suffix;
}

void AwoxMesh::call_connection(int dest, std::function<void(MeshConnection *)> &&callback) {
  ESP_LOGD(TAG, "Call connection for %d", dest);
  for (auto *connection : this->connections_) {
    if (connection->get_address() > 0) {
      ESP_LOGD(TAG, "Found %s as connection", connection->address_str().c_str());
      callback(connection);

      // for now we stop on first
      return;
    }
  }
  ESP_LOGW(TAG, "No active connection for %d", dest);
}

void AwoxMesh::request_device_info(Device *device) {
  this->call_connection(device->mesh_id, [device](MeshConnection *connection) {
    device->device_info_requested = esphome::millis();
    connection->request_device_info(device);
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

void AwoxMesh::request_status_update(int dest) {
  this->call_connection(dest, [dest](MeshConnection *connection) { connection->request_status_update(dest); });
}

}  // namespace awox_mesh
}  // namespace esphome

#endif
