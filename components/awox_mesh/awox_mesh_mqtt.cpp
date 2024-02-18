#ifdef USE_ESP32

#include <cstring>
#include <cstdio>
#include <algorithm>
#include <math.h>
#include <regex>

#include "esphome/components/mqtt/mqtt_const.h"
#include "esphome/components/mqtt/mqtt_component.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"

#include "awox_mesh_mqtt.h"
#include "awox_mesh.h"

namespace esphome {
namespace awox_mesh {

static const char *const TAG = "awox.mesh.mqtt";

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

void AwoxMeshMqtt::setup() {
  // Use retained MQTT messages to publish a default offline status for all devices
  global_mqtt_client->subscribe(
      global_mqtt_client->get_topic_prefix() + "/#", [this](const std::string &topic, const std::string &payload) {
        if (std::regex_match(topic, std::regex(global_mqtt_client->get_topic_prefix() + "/[0-9]+/availability"))) {
          ESP_LOGD(TAG, "Received topic: %s, %s", topic.c_str(), payload.c_str());
          if (payload == "online") {
            global_mqtt_client->publish(topic.c_str(), "offline");
          }
        }
      });
}

std::string AwoxMeshMqtt::get_discovery_topic_(const MQTTDiscoveryInfo &discovery_info, Device *device) const {
  return discovery_info.prefix + "/" + device->device_info->get_component_type() + "/awox-" +
         device->address_str_hex_only() + "/config";
}

std::string AwoxMeshMqtt::get_mqtt_topic_for_(MeshDestination *mesh_destination, const std::string &suffix) const {
  if (strcmp(mesh_destination->type(), "group") == 0) {
    return global_mqtt_client->get_topic_prefix() + "/group-" + std::to_string(mesh_destination->dest()) + "/" + suffix;
  } else {
    return global_mqtt_client->get_topic_prefix() + "/" + std::to_string(mesh_destination->dest()) + "/" + suffix;
  }
}

void AwoxMeshMqtt::publish_connected(bool has_active_connection, int online_devices,
                                     const std::vector<MeshConnection *> &connections) {
  if (!this->published_connected) {
    // todo.... find proper solution
    // stop_listening_for_availability
    global_mqtt_client->unsubscribe(global_mqtt_client->get_topic_prefix() + "/#");
    this->published_connected = true;
  }

  const std::string message = has_active_connection ? "online" : "offline";
  ESP_LOGI(TAG, "Publish connected to mesh device - %s", message.c_str());
  global_mqtt_client->publish(global_mqtt_client->get_topic_prefix() + "/connected", message, 0, true);

  global_mqtt_client->publish_json(
      global_mqtt_client->get_topic_prefix() + "/connection_status",
      [&](JsonObject root) {
        root["now"] = esphome::millis();
        root["active_connections"] = has_active_connection;
        root["online_devices"] = online_devices;

        for (int i = 0; i < connections.size(); i++) {
          JsonObject connection = root.createNestedObject("connection_" + std::to_string(i));
          connection["connected"] = connections[i]->connected();
          connection["mac"] = connections[i]->connected() ? connections[i]->address_str() : "";
          connection["mesh_id"] = connections[i]->connected() ? std::to_string(connections[i]->mesh_id()) : "";
          connection["devices"] = connections[i]->get_linked_mesh_ids().size();

          std::stringstream mesh_ids;
          std::copy(connections[i]->get_linked_mesh_ids().begin(), connections[i]->get_linked_mesh_ids().end(),
                    std::ostream_iterator<int>(mesh_ids, ", "));
          connection["mesh_ids"] = mesh_ids.str().substr(0, mesh_ids.str().size() - 2);
        }
      },
      0, false);
}

void AwoxMeshMqtt::publish_availability(Device *device) {
  const std::string message = device->online ? "online" : "offline";
  ESP_LOGI(TAG, "Publish online/offline for %d - %s", device->mesh_id, message.c_str());
  global_mqtt_client->publish(this->get_mqtt_topic_for_(device, "availability"), message, 0, true);
}

void AwoxMeshMqtt::publish_availability(Group *group) {
  const std::string message = group->online ? "online" : "offline";
  ESP_LOGI(TAG, "Publish online/offline for group %d - %s", group->group_id, message.c_str());
  global_mqtt_client->publish(this->get_mqtt_topic_for_(group, "availability"), message, 0, true);
}

void AwoxMeshMqtt::publish_state(MeshDestination *mesh_destination) {
  if (!mesh_destination->can_publish_state()) {
    ESP_LOGW(TAG, "[%d] Can not yet send publish state for %s", mesh_destination->dest(), mesh_destination->type());
    return;
  }
  if (mesh_destination->device_info->has_feature(FEATURE_LIGHT_MODE)) {
    global_mqtt_client->publish_json(
        this->get_mqtt_topic_for_(mesh_destination, "state"),
        [this, mesh_destination](JsonObject root) {
          root["state"] = mesh_destination->state ? "ON" : "OFF";

          if (mesh_destination->color_mode) {
            root["color_mode"] = "rgb";
            root["brightness"] =
                convert_value_to_available_range(mesh_destination->color_brightness, 0xa, 0x64, 0, 255);
          } else {
            if (mesh_destination->device_info->has_feature(FEATURE_WHITE_TEMPERATURE)) {
              root["color_mode"] = "color_temp";
              root["color_temp"] = convert_value_to_available_range(mesh_destination->temperature, 0, 0x7f, 153, 370);
            } else {
              root["color_mode"] = "brightness";
            }
            root["brightness"] = convert_value_to_available_range(mesh_destination->white_brightness, 1, 0x7f, 0, 255);
          }

          // https://developers.home-assistant.io/docs/core/entity/light#color-mode-when-rendering-effects
          if (mesh_destination->candle_mode || mesh_destination->sequence_mode) {
            root["color_mode"] = "brightness";
          }

          JsonObject color = root.createNestedObject("color");
          color["r"] = mesh_destination->R;
          color["g"] = mesh_destination->G;
          color["b"] = mesh_destination->B;
        },
        0, true);
  } else {
    global_mqtt_client->publish(this->get_mqtt_topic_for_(mesh_destination, "state"),
                                mesh_destination->state ? "ON" : "OFF", mesh_destination->state ? 2 : 3, 0, true);
  }
}

void AwoxMeshMqtt::publish_connection_sensor_discovery(const std::vector<MeshConnection *> &connections) {
  const MQTTDiscoveryInfo &discovery_info = global_mqtt_client->get_discovery_info();
  const std::string sanitized_name = str_sanitize(App.get_name());

  for (int i = 0; i < connections.size(); i++) {
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

void AwoxMeshMqtt::send_discovery(Device *device) {
  if (!device->address_set() || device->device_info == nullptr) {
    ESP_LOGW(TAG, "'%s': Can not yet send discovery, mac address not known...",
             std::to_string(device->mesh_id).c_str());
    return;
  }

  ESP_LOGD(TAG, "[%d]: Sending discovery productID: 0x%02X (%s - %s) mac: %s", device->mesh_id,
           device->device_info->get_product_id(), device->device_info->get_name(), device->device_info->get_model(),
           device->address_str().c_str());

  const MQTTDiscoveryInfo &discovery_info = global_mqtt_client->get_discovery_info();

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
                                        this->mesh_->set_power(device->mesh_id, true);
                                        break;
                                      case PARSE_OFF:
                                        device->state = false;
                                        this->mesh_->set_power(device->mesh_id, false);
                                        break;
                                      case PARSE_TOGGLE:
                                        device->state = !device->state;
                                        this->mesh_->set_power(device->mesh_id, device->state);
                                        break;
                                      case PARSE_NONE:
                                        break;
                                    }
                                  });
  }
  this->publish_availability(device);
}

void AwoxMeshMqtt::send_group_discovery(Group *group) {
  if (group->device_info == nullptr) {
    ESP_LOGW(TAG, "'%s': Can not yet send discovery, component_type not known...",
             std::to_string(group->group_id).c_str());
    return;
  }

  const MQTTDiscoveryInfo &discovery_info = global_mqtt_client->get_discovery_info();

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

void AwoxMeshMqtt::process_incomming_command(MeshDestination *mesh_destination, JsonObject root) {
  int dest = mesh_destination->dest();
  bool state_set = false;

  ESP_LOGD(TAG, "[%d] Process command %s", dest, mesh_destination->type());

  if (root.containsKey("fade_duration")) {
    ESP_LOGD(TAG, "[%d] set sequence fade_duration %d", dest, (int) root["fade_duration"]);
    this->mesh_->set_sequence_fade_duration(dest, (int) root["fade_duration"]);
  }

  if (root.containsKey("color_duration")) {
    ESP_LOGD(TAG, "[%d] set sequence color_duration %d", dest, (int) root["color_duration"]);
    this->mesh_->set_sequence_color_duration(dest, (int) root["color_duration"]);
  }

  if (root.containsKey("color")) {
    JsonObject color = root["color"];

    state_set = true;
    mesh_destination->state = true;
    mesh_destination->color_mode = true;
    mesh_destination->R = (int) color["r"];
    mesh_destination->G = (int) color["g"];
    mesh_destination->B = (int) color["b"];

    ESP_LOGD(TAG, "[%d] Process command color %d %d %d", dest, (int) color["r"], (int) color["g"], (int) color["b"]);

    this->mesh_->set_color(dest, (int) color["r"], (int) color["g"], (int) color["b"]);
  }

  if (root.containsKey("brightness") && !root.containsKey("color_temp") &&
      (root.containsKey("color") || mesh_destination->color_mode)) {
    int brightness = convert_value_to_available_range((int) root["brightness"], 0, 255, 0xa, 0x64);

    state_set = true;
    mesh_destination->state = true;
    mesh_destination->color_brightness = brightness;

    ESP_LOGD(TAG, "[%d] Process command color_brightness %d", dest, (int) root["brightness"]);
    this->mesh_->set_color_brightness(dest, brightness);

  } else if (root.containsKey("brightness")) {
    int brightness = convert_value_to_available_range((int) root["brightness"], 0, 255, 1, 0x7f);

    state_set = true;
    mesh_destination->state = true;
    mesh_destination->white_brightness = brightness;

    ESP_LOGD(TAG, "[%d] Process command white_brightness %d", dest, (int) root["brightness"]);
    this->mesh_->set_white_brightness(dest, brightness);
  }

  if (root.containsKey("color_temp")) {
    int temperature = convert_value_to_available_range((int) root["color_temp"], 153, 370, 0, 0x7f);

    state_set = true;
    mesh_destination->state = true;
    mesh_destination->color_mode = false;
    mesh_destination->temperature = temperature;

    ESP_LOGD(TAG, "[%d] Process command color_temp %d", dest, (int) root["color_temp"]);
    this->mesh_->set_white_temperature(dest, temperature);
  }

  if (root.containsKey("effect")) {
    state_set = true;
    mesh_destination->state = true;
    mesh_destination->sequence_mode = false;
    mesh_destination->candle_mode = false;
    if (root["effect"] == "color loop") {
      mesh_destination->sequence_mode = true;
      this->mesh_->set_sequence(dest, 0);
    } else if (root["effect"] == "candle") {
      mesh_destination->candle_mode = true;
      this->mesh_->set_candle_mode(dest);
    } else {
      if (mesh_destination->color_mode) {
        this->mesh_->set_color(dest, mesh_destination->R, mesh_destination->G, mesh_destination->B);
      } else {
        this->mesh_->set_white_temperature(dest, mesh_destination->temperature);
      }
    }
  }

  if (root.containsKey("state")) {
    ESP_LOGD(TAG, "[%d] Process command state", dest);
    auto val = parse_on_off(root["state"]);
    switch (val) {
      case PARSE_ON:
        mesh_destination->state = true;
        if (!state_set) {
          this->mesh_->set_power(dest, true);
        }
        break;
      case PARSE_OFF:
        mesh_destination->state = false;
        this->mesh_->set_power(dest, false);
        break;
      case PARSE_TOGGLE:
        mesh_destination->state = !mesh_destination->state;
        this->mesh_->set_power(dest, mesh_destination->state);
        break;
      case PARSE_NONE:
        break;
    }
  }

  this->publish_state(mesh_destination);
}

}  // namespace awox_mesh
}  // namespace esphome

#endif
