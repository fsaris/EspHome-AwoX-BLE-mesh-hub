#ifdef USE_ESP32
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <math.h>
#include <regex>
#include "awox_mesh.h"

#include "esphome/core/log.h"

namespace esphome {
namespace awox_mesh {

static const char *const TAG = "awox.mesh";
static const int RSSI_NOT_AVAILABLE = -9999;

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

  this->publish_connection->setup();

  this->publish_connection->publish_connection_sensor_discovery(this->connections_);

  this->set_interval("publish_connection", 5000, [this]() { this->publish_connected(); });
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

  this->publish_connection->publish_connected(this->has_active_connection, online_devices, this->connections_);
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

  this->publish_connection->publish_availability(device);
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

void AwoxMesh::publish_state(MeshDestination *mesh_destination) {
  this->publish_connection->publish_state(mesh_destination);

  for (Group *group : mesh_destination->get_groups()) {
    this->sync_and_publish_group_state(group);
  }
}

void AwoxMesh::send_discovery(Device *device) {
  if (!device->address_set()) {
    ESP_LOGW(TAG, "'%s': Can not yet send discovery, mac address not known...",
             std::to_string(device->mesh_id).c_str());
    return;
  }

  device->device_info = this->device_info_resolver->get_by_product_id(device->product_id);
  device->send_discovery = true;

  this->publish_connection->send_discovery(device);
}

void AwoxMesh::send_group_discovery(Group *group) {
  if (group->device_info == nullptr) {
    ESP_LOGW(TAG, "'%s': Can not yet send discovery, component_type not known...",
             std::to_string(group->group_id).c_str());
    return;
  }

  group->send_discovery = true;

  this->publish_connection->send_group_discovery(group);
}

void AwoxMesh::call_connection(int dest, std::function<void(MeshConnection *)> &&callback) {
  ESP_LOGD(TAG, "Call connection for %d", dest);
  for (auto *connection : this->connections_) {
    if (connection->get_address() > 0 && connection->mesh_id_linked(dest)) {
      ESP_LOGD(TAG, "Found %s as connection", connection->address_str().c_str());
      callback(connection);

      // for now we stop on first
      return;
    }
  }

  ESP_LOGI(TAG, "No active connection for %d, we trigger message on all could be also a group", dest);
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
