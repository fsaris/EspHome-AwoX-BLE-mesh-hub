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
    this->add_device(new MeshLightColor("0032", "Spot 120", "EGLOSpot 120/w", "EGLO", "mdi:wall-sconce-flat"));
    this->add_device(new MeshLightColor("0033", "Spot 170", "EGLOSpot 170/w", "EGLO", "mdi:wall-sconce-flat"));
    this->add_device(new MeshLightColor("0034", "Spot 225", "EGLOSpot 225/w", "EGLO", "mdi:wall-sconce-flat"));

    this->add_device(new MeshPlug("0097", "EGLO PLUG PLUS", "SMPWBm10AUS", "EGLO", "mdi:power-socket-au"));
    this->add_device(new MeshPlug("009E", "EGLO PLUG PLUS", "SMPWBm10AUSb", "EGLO", "mdi:power-socket-au"));
    this->add_device(new MeshPlug("0090", "EGLO PLUG PLUS", "SMPWBm10CH", "EGLO", "mdi:power-socket-ch"));
    this->add_device(new MeshPlug("00A0", "EGLO PLUG PLUS", "SMPWBm10CHb", "EGLO", "mdi:power-socket-ch"));
    this->add_device(new MeshPlug("0067", "EGLO PLUG PLUS", "SMPWBm10FR", "EGLO", "mdi:power-socket-fr"));
    this->add_device(new MeshPlug("0084", "EGLO PLUG PLUS", "SMPWBm10FRa", "EGLO", "mdi:power-socket-fr"));
    this->add_device(new MeshPlug("009C", "EGLO PLUG PLUS", "SMPWBm10FRb", "EGLO", "mdi:power-socket-"));
    this->add_device(new MeshPlug("0068", "EGLO PLUG PLUS", "SMPWBm10GE", "EGLO", "mdi:power-socket-de"));
    this->add_device(new MeshPlug("0085", "EGLO PLUG PLUS", "SMPWBm10GEa", "EGLO", "mdi:power-socket-de"));
    this->add_device(new MeshPlug("009D", "EGLO PLUG PLUS", "SMPWBm10GEb", "EGLO", "mdi:power-socket-de"));
    this->add_device(new MeshPlug("008F", "EGLO PLUG PLUS", "SMPWBm10UK", "EGLO", "mdi:power-socket-uk"));
    this->add_device(new MeshPlug("009F", "EGLO PLUG PLUS", "SMPWBm10UKb", "EGLO", "mdi:power-socket-uk"));
    this->add_device(new MeshPlug("008B", "EGLO PLUG", "ESMP-Bm10-AUS", "EGLO", "mdi:power-socket-au"));
    this->add_device(new MeshPlug("008D", "EGLO PLUG", "ESMP-Bm10-CH", "EGLO", "mdi:power-socket-ch"));
    this->add_device(new MeshPlug("0062", "EGLO PLUG", "ESMP-Bm10-FR", "EGLO", "mdi:power-socket-fr"));
    this->add_device(new MeshPlug("0063", "EGLO PLUG", "ESMP-Bm10-GE", "EGLO", "mdi:power-socket-de"));
    this->add_device(new MeshPlug("008C", "EGLO PLUG", "ESMP-Bm10-UK", "EGLO", "mdi:power-socket-uk"));
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
