#pragma once

#include <string>
#include <map>

namespace esphome {
namespace awox_mesh {

#define FEATURE_LIGHT_MODE 0x01
#define FEATURE_COLOR 0x02
#define FEATURE_WHITE_BRIGHTNESS 0x03
#define FEATURE_WHITE_TEMPERATURE 0x04
#define FEATURE_COLOR_BRIGHTNESS 0x04

class DeviceInfo {
 protected:
  std::string component_type;
  std::string product_id;
  std::string name;
  std::string model;
  std::string manufacturer;
  std::string icon;
  std::map<int, bool> features;

  void add_feature(int feature) { features[feature] = true; }

 public:
  DeviceInfo(std::string product_id = "", std::string name = "", std::string model = "", std::string manufacturer = "",
             std::string icon = "") {
    this->product_id = product_id;
    this->name = name;
    this->model = model;
    this->manufacturer = manufacturer;
    this->icon = icon;
  }

  std::string get_component_type() const { return this->component_type; }
  std::string get_product_id() const { return this->product_id; }
  std::string get_name() const { return this->name; }
  std::string get_model() const { return this->model; }
  std::string get_manufacturer() const { return this->manufacturer; }
  std::string get_icon() const { return this->icon; }

  bool has_feature(int feature) { return features.count(feature) > 0; }
};

class MeshLightColor : public DeviceInfo {
 public:
  MeshLightColor(std::string product_id, std::string name, std::string model, std::string manufacturer,
                 std::string icon = "")
      : DeviceInfo(product_id, name, model, manufacturer, icon) {
    component_type = "light";

    this->add_feature(FEATURE_LIGHT_MODE);
    this->add_feature(FEATURE_COLOR);
    this->add_feature(FEATURE_WHITE_BRIGHTNESS);
    this->add_feature(FEATURE_WHITE_TEMPERATURE);
    this->add_feature(FEATURE_COLOR_BRIGHTNESS);
  }
};

class MeshLightWhite : public DeviceInfo {
 public:
  MeshLightWhite(std::string product_id, std::string name, std::string model, std::string manufacturer,
                 std::string icon = "")
      : DeviceInfo(product_id, name, model, manufacturer, icon) {
    this->component_type = "light";

    this->add_feature(FEATURE_LIGHT_MODE);
    this->add_feature(FEATURE_WHITE_BRIGHTNESS);
    this->add_feature(FEATURE_WHITE_TEMPERATURE);
  }
};

class MeshPlug : public DeviceInfo {
 public:
  MeshPlug(std::string product_id, std::string name, std::string model, std::string manufacturer, std::string icon = "")
      : DeviceInfo(product_id, name, model, manufacturer, icon) {
    this->component_type = "switch";
  }
};

class DeviceInfoResolver {
  std::map<std::string, DeviceInfo *> devices_info{};

  void add_device(DeviceInfo *device_info) { this->devices_info[device_info->get_product_id()] = device_info; }

 public:
  DeviceInfoResolver() {
    this->add_device(new MeshLightColor("0050", "Spot 120", "EGLOSpot 120/w", "Eglo", "mdi:wall-sconce-flat"));
    this->add_device(new MeshLightColor("0051", "Spot 170", "EGLOSpot 170/w", "Eglo", "mdi:wall-sconce-flat"));
    this->add_device(new MeshLightColor("0052", "Spot 225", "EGLOSpot 225/w", "Eglo", "mdi:wall-sconce-flat"));
  }

  DeviceInfo *get_by_product_id(std::string product_id) {
    if (devices_info.count(product_id)) {
      return devices_info[product_id];
    }

    return new MeshLightWhite(product_id, "Unknown device type", "Unknown device, product id: " + product_id, "AwoX",
                              "mdi:lightbulb-help-outline");
  }
};

}  // namespace awox_mesh
}  // namespace esphome
