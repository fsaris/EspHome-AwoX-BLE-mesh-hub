#pragma once

#include <string>
#include <map>

namespace esphome {
namespace awox_mesh {

#define DEVICE_TYPE_RGB 0x01
#define DEVICE_TYPE_DIM 0x02
#define DEVICE_TYPE_TW 0x03
#define DEVICE_TYPE_PLUG 0x04

#define FEATURE_LIGHT_MODE 0x01
#define FEATURE_COLOR 0x02
#define FEATURE_WHITE_BRIGHTNESS 0x03
#define FEATURE_WHITE_TEMPERATURE 0x04
#define FEATURE_COLOR_BRIGHTNESS 0x04

class DeviceInfo {
 protected:
  const char *component_type;
  int product_id;
  const char *name;
  const char *model;
  const char *manufacturer;
  const char *icon;
  std::map<int, bool> features;

  void add_feature(int feature) { features[feature] = true; }

 public:
  DeviceInfo(int product_id = 0, const char *name = "", const char *model = "", const char *manufacturer = "",
             const char *icon = "") {
    this->product_id = product_id;
    this->name = name;
    this->model = model;
    this->manufacturer = manufacturer;
    this->icon = icon;
  }

  const char *get_component_type() const { return this->component_type; }
  int get_product_id() const { return this->product_id; }
  const char *get_name() const { return this->name; }
  const char *get_model() const { return this->model; }
  const char *get_manufacturer() const { return this->manufacturer; }
  const char *get_icon() const { return this->icon; }

  bool has_feature(int feature) { return features.count(feature) > 0; }
};

class MeshLightColor : public DeviceInfo {
 public:
  MeshLightColor(int product_id, const char *name, const char *model, const char *manufacturer, const char *icon = "")
      : DeviceInfo(product_id, name, model, manufacturer, icon) {
    component_type = "light";

    this->add_feature(FEATURE_LIGHT_MODE);
    this->add_feature(FEATURE_COLOR);
    this->add_feature(FEATURE_WHITE_BRIGHTNESS);
    this->add_feature(FEATURE_WHITE_TEMPERATURE);
    this->add_feature(FEATURE_COLOR_BRIGHTNESS);
  }
};

class MeshLightWhiteTemperature : public DeviceInfo {
 public:
  MeshLightWhiteTemperature(int product_id, const char *name, const char *model, const char *manufacturer,
                            const char *icon = "")
      : DeviceInfo(product_id, name, model, manufacturer, icon) {
    this->component_type = "light";

    this->add_feature(FEATURE_LIGHT_MODE);
    this->add_feature(FEATURE_WHITE_BRIGHTNESS);
    this->add_feature(FEATURE_WHITE_TEMPERATURE);
  }
};

class MeshLightWhite : public DeviceInfo {
 public:
  MeshLightWhite(int product_id, const char *name, const char *model, const char *manufacturer, const char *icon = "")
      : DeviceInfo(product_id, name, model, manufacturer, icon) {
    this->component_type = "light";

    this->add_feature(FEATURE_LIGHT_MODE);
    this->add_feature(FEATURE_WHITE_BRIGHTNESS);
  }
};

class MeshPlug : public DeviceInfo {
 public:
  MeshPlug(int product_id, const char *name, const char *model, const char *manufacturer, const char *icon = "")
      : DeviceInfo(product_id, name, model, manufacturer, icon) {
    this->component_type = "switch";
  }
};

class DeviceInfoResolver {
  std::map<int, DeviceInfo *> device_info{};

  DeviceInfo *create_device_info(int device_type, int product_id, const char *name, const char *model, const char *manufacturer,
                                 const char *icon = "") {

    if (device_type == DEVICE_TYPE_RGB) {
      auto device1 = new MeshLightColor(product_id, name, model, manufacturer, icon);
      this->device_info[product_id] = device1;
    } else if (device_type == DEVICE_TYPE_TW) {
      auto device2 = new MeshLightWhiteTemperature(product_id, name, model, manufacturer, icon);
      this->device_info[product_id] = device2;
    } else if (device_type == DEVICE_TYPE_PLUG) {
      auto device3 = new MeshPlug(product_id, name, model, manufacturer, icon);
      this->device_info[product_id] = device3;
    } else {
      auto device = new MeshLightWhite(product_id, name, model, manufacturer, icon);
      this->device_info[product_id] = device;
    }

    return this->device_info[product_id];
  }

 public:
  DeviceInfo *get_by_product_id(int product_id) {
    if (device_info.count(product_id)) {
      return device_info[product_id];
    }

    return this->create_device_info(DEVICE_TYPE_DIM, product_id, "Unknown device type", "", "AwoX", "mdi:lightbulb-help-outline");
  }

  void register_device(int device_type, int product_id, const char *name, const char *model, const char *manufacturer,
                                 const char *icon = "") {
    this->create_device_info(device_type, product_id, name, model, manufacturer, icon);
  }
};

}  // namespace awox_mesh
}  // namespace esphome
