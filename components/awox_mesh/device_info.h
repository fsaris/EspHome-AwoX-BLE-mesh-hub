#pragma once

#include <string>
#include <map>

namespace esphome {
namespace awox_mesh {

#define DEVICE_TYPE_RGB 0x01
#define DEVICE_TYPE_DIM 0x02
#define DEVICE_TYPE_TW 0x03
#define DEVICE_TYPE_PLUG 0x04

#define MANUFACTURER_AWOX 0x01
#define MANUFACTURER_EGLO 0x02
#define MANUFACTURER_KERIA 0x03

#define FEATURE_LIGHT_MODE 0x01
#define FEATURE_COLOR 0x02
#define FEATURE_WHITE_BRIGHTNESS 0x03
#define FEATURE_WHITE_TEMPERATURE 0x04
#define FEATURE_COLOR_BRIGHTNESS 0x04

struct DeviceStruct {
  int device_type;
  const char *name;
  const char *model;
  int manufacturer;
  const char *icon;
};

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

  std::map<int, DeviceStruct> devices{};

  DeviceInfo *create_device_info(int product_id, DeviceStruct device) {
    return this->create_device_info(device.device_type, product_id, device.name, device.model, device.manufacturer,
                                    device.icon);
  }

  DeviceInfo *create_device_info(int device_type, int product_id, const char *name, const char *model, int manufacturer,
                                 const char *icon = "") {
    const char *manufacturer_ = "AwoX";
    switch (manufacturer) {
      case MANUFACTURER_EGLO:
        manufacturer_ = "EGLO";
        break;

      case MANUFACTURER_KERIA:
        manufacturer_ = "KERIA";
        break;
    }

    if (device_type == DEVICE_TYPE_RGB) {
      auto device1 = new MeshLightColor(product_id, name, model, manufacturer_, icon);
      this->device_info[product_id] = device1;
    } else if (device_type == DEVICE_TYPE_TW) {
      auto device2 = new MeshLightWhiteTemperature(product_id, name, model, manufacturer_, icon);
      this->device_info[product_id] = device2;
    } else if (device_type == DEVICE_TYPE_PLUG) {
      auto device3 = new MeshPlug(product_id, name, model, manufacturer_, icon);
      this->device_info[product_id] = device3;
    } else {
      auto device = new MeshLightWhite(product_id, name, model, manufacturer_, icon);
      this->device_info[product_id] = device;
    }

    return this->device_info[product_id];
  }

  void add_device(int device_type, int product_id, const char *name, const char *model, int manufacturer,
                  const char *icon = "") {
    DeviceStruct device = {};
    device.device_type = device_type;
    device.name = name;
    device.model = model;
    device.manufacturer = manufacturer;
    device.icon = icon;
    this->devices[product_id] = device;
  }

  static const char *get_product_code_as_hex_string(int product_id) {
    char value[13];
    sprintf(value, "Product: %04X", product_id);
    return std::string((char *) value, 13).c_str();
  }

 public:
  DeviceInfoResolver() {
    // this->add_device(DEVICE_TYPE_RGB, 0x13, "SmartLIGHT Color Mesh 9", "SMLm_C9", MANUFACTURER_AWOX);
    // this->add_device(DEVICE_TYPE_TW, 0x14, "SmartLIGHT White Mesh 13W", "SMLm_W13", MANUFACTURER_AWOX);
    // this->add_device(DEVICE_TYPE_RGB, 0x15, "SmartLIGHT Color Mesh 13W", "SMLm_C13", MANUFACTURER_AWOX);
    // this->add_device(DEVICE_TYPE_TW, 0x16, "SmartLIGHT White Mesh 15W", "SMLm_W15", MANUFACTURER_AWOX);
    // this->add_device(DEVICE_TYPE_RGB, 0x17, "SmartLIGHT Color Mesh 15W", "SMLm_C15", MANUFACTURER_AWOX);
    // this->add_device(DEVICE_TYPE_TW, 0x21, "SmartLIGHT White Mesh 9W", "SSMLm_w9", MANUFACTURER_AWOX);
    // this->add_device(DEVICE_TYPE_RGB, 0x22, "SmartLIGHT Color Mesh 9W", "SSMLm_c9", MANUFACTURER_AWOX);
    // this->add_device(DEVICE_TYPE_RGB, 0x23, "EGLOBulb A60 9W", "ESMLm_c9", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x24, "Keria SmartLIGHT Color Mesh 9W", "KSMLm_c9", MANUFACTURER_KERIA);
    this->add_device(DEVICE_TYPE_RGB, 0x25, "EGLOPanel 30X30", "EPanel_300", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x26, "EGLOPanel 60X60", "EPanel_600", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x27, "EGLO Ceiling DOWNLIGHT", "EMod_Ceil", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x29, "EGLOBulb G95 13W", "ESMLm_c13g", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x2A, "Keria SmartLIGHT Color Mesh 13W Globe", "KSMLm_c13g",
    // MANUFACTURER_KERIA); this->add_device(DEVICE_TYPE_RGB, 0x2B, "SmartLIGHT Color Mesh 13W Globe", "SMLm_c13g",
    // MANUFACTURER_AWOX);
    this->add_device(DEVICE_TYPE_RGB, 0x30, "EGLOPanel 30X120", "EPanel_120", MANUFACTURER_EGLO);
    this->add_device(DEVICE_TYPE_RGB, 0x32, "Spot 120", "EGLOSpot 120/w", MANUFACTURER_EGLO, "mdis:wall-sconce-flat");
    this->add_device(DEVICE_TYPE_RGB, 0x33, "Spot 170", "EGLOSpot 170/w", MANUFACTURER_EGLO, "mdi:wall-sconce-flat");
    this->add_device(DEVICE_TYPE_RGB, 0x34, "Spot 225", "EGLOSpot 225/w", MANUFACTURER_EGLO, "mdi:wall-sconce-flat");
    this->add_device(DEVICE_TYPE_RGB, 0x35, "Giron-C 17W", "EGLO 32589", MANUFACTURER_EGLO, "mdi:wall-sconce-flat");
    // this->add_device(DEVICE_TYPE_RGB, 0x36, "EGLO Ceiling GIRON 30", "ECeil_g38", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x37, "SmartLIGHT Color Mesh 5W GU10", "SMLm_c5_GU10", MANUFACTURER_AWOX);
    // this->add_device(DEVICE_TYPE_RGB, 0x38, "SmartLIGHT Color Mesh 5W E14", "SMLm_c5_E14", MANUFACTURER_AWOX);
    // this->add_device(DEVICE_TYPE_RGB, 0x3A, "Keria SmartLIGHT Color Mesh 5W GU10", "KSMLm_c5_GU10",
    // MANUFACTURER_KERIA); this->add_device(DEVICE_TYPE_RGB, 0x3B, "Keria SmartLIGHT Color Mesh 5W E14",
    // "KSMLm_c5_E14", MANUFACTURER_KERIA); this->add_device(DEVICE_TYPE_RGB, 0x3C, "SmartLIGHT Color Mesh 5W GU10",
    // "ESMLm_c5_GU10", MANUFACTURER_EGLO); this->add_device(DEVICE_TYPE_RGB, 0x3D, "SmartLIGHT Color Mesh 5W E14",
    // "ESMLm_c5_E14", MANUFACTURER_EGLO); this->add_device(DEVICE_TYPE_RGB, 0x3F, "EGLO Surface round", "EFueva_225r",
    // MANUFACTURER_EGLO); this->add_device(DEVICE_TYPE_RGB, 0x40, "EGLO Surface square", "EFueva_225s",
    // MANUFACTURER_EGLO); this->add_device(DEVICE_TYPE_RGB, 0x41, "EGLO Surface round", "EFueva_300r",
    // MANUFACTURER_EGLO); this->add_device(DEVICE_TYPE_RGB, 0x42, "EGLO Surface square", "EFueva_300s",
    // MANUFACTURER_EGLO); this->add_device(DEVICE_TYPE_RGB, 0x43, "SmartLIGHT Color Mesh 9W", "SMLm_c9s",
    // MANUFACTURER_AWOX); this->add_device(DEVICE_TYPE_RGB, 0x44, "SmartLIGHT Color Mesh 13W", "SMLm_c13gs",
    // MANUFACTURER_AWOX); this->add_device(DEVICE_TYPE_RGB, 0x45, "EGLOBulb A60 9W", "ESMLm_c9s", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x46, "EGLOBulb G95 13W", "ESMLm_c13gs", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x47, "Keria SmartLIGHT Color Mesh 9W", "KSMLm_c9s", MANUFACTURER_KERIA);
    // this->add_device(DEVICE_TYPE_RGB, 0x48, "Keria SmartLIGHT Color Mesh 13W Glob", "KSMLm_c13gs",
    // MANUFACTURER_KERIA); this->add_device(DEVICE_TYPE_DIM, 0x49, "EGLOBulb A60 Warm", "ESMLm_w9w",
    // MANUFACTURER_EGLO); this->add_device(DEVICE_TYPE_DIM, 0x4A, "EGLOBulb A60 Neutral", "ESMLm_w9n",
    // MANUFACTURER_EGLO); this->add_device(DEVICE_TYPE_RGB, 0x4B, "EGLO Ceiling", "ECeiling_30", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x4C, "EGLO Pendant", "EPendant_30", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x4D, "EGLO Pendant", "EPendant_20", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x4E, "EGLO Stripled 3m", "EStrip_3m", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x4F, "EGLO Stripled 5m", "EStrip_5m", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_DIM, 0x50, "Outdoor", "EOutdoor_w14w", MANUFACTURER_EGLO);
    this->add_device(DEVICE_TYPE_RGB, 0x51, "EGLOSpot", "ETriSpot_85", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x53, "SmartLIGHT Color Mesh 9W", "SMLm_c9i", MANUFACTURER_AWOX);
    // this->add_device(DEVICE_TYPE_RGB, 0x54, "EGLOBulb A60 9W", "ESMLm_c9i", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x55, "Keria SmartLIGHT Color Mesh 9W", "KSMLm_c9i", MANUFACTURER_KERIA);
    // this->add_device(DEVICE_TYPE_RGB, 0x56, "EGLOPanel 62X62", "EPanel_620", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x57, "EGLOPanel 45X45", "EPanel_450", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x59, "SmartLIGHT Color Mesh 13W Globe", "SMLm_c13gi", MANUFACTURER_AWOX);
    // this->add_device(DEVICE_TYPE_RGB, 0x5A, "EGLOBulb G95 13W", "ESMLm_c13gi", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x5B, "Keria SmartLIGHT Color Mesh 13W Globe", "KSMLm_c13gi",
    // MANUFACTURER_KERIA); this->add_device(DEVICE_TYPE_RGB, 0x5C, "SmartLIGHT Color Mesh 9W", "SSMLm_c9i",
    // MANUFACTURER_AWOX); this->add_device(DEVICE_TYPE_TW, 0x64, "SmartLIGHT White Mesh 9W", "SMLm_w9",
    // MANUFACTURER_AWOX); this->add_device(DEVICE_TYPE_TW, 0x65, "SmartLIGHT White Mesh 9W", "ESMLm_w9",
    // MANUFACTURER_EGLO); this->add_device(DEVICE_TYPE_RGB, 0x69, "Ceiling GIRON 60", "ECeil_g60", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_TW, 0x6A, "SmartLIGHT Bulb A60 Warm", "SMLm_w9w", MANUFACTURER_AWOX);
    // this->add_device(DEVICE_TYPE_TW, 0x6F, "EGLOBulb Filament G80", "ESMLFm-w6-G80", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_TW, 0x71, "EGLOBulb Filament ST64", "ESMLFm-w6-ST64", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_TW, 0x75, "EGLOBulb Filament G95", "ESMLFm-w6-G95", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x77, "EGLO Spot", "ESpot_c5", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x78, "EGLO Fraioli", "EFraioli_c17", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x79, "EGLO Frattina", "EFrattina_c18", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x7A, "EGLO Frattina", "EFrattina_c27", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x7B, "EGLOPanel 30 Round", "EPanel_r300", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x7C, "EGLOPanel 45 Round", "EPanel_r450", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x7D, "EGLOPanel 60 Round", "EPanel_r600", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x7E, "EGLOPanel 10X120", "EPanel_120_10", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_TW, 0x80, "EPanel white round", "EPanel_w_round", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_TW, 0x81, "EPanel white square", "EPanel_w_square", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_TW, 0x82, "EPanel white rectangle", "EPanel_w_rect", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_TW, 0x83, "ECeiling white round", "ECeiling-w", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_TW, 0x87, "EGLO Tunable White", "EDoubleWhite", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x88, "EGLO Ceiling GIRON 80", "ECeil_g80", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_TW, 0x89, "Outdoor Marchesa-C", "EMarchesa_C", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_TW, 0x8A, "Outdoor Francari-C", "EFrancari_C", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x92, "EPanel square", "EPanel_36W_square", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_DIM, 0x94, "EGLOBulb Filament ST64", "ESMLFm-w6w-ST64", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_DIM, 0x95, "EGLOBulb Filament G95", "ESMLFm-w6w-G95", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x96, "EGLO RGB+TW", "EGLO-RGB-TW", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_TW, 0x97, "EGLO Tunable White", "EGLO-TW", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0x99, "EGLO RGB+TW", "EGLO-RGB-TW", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_TW, 0x9A, "EGLO Tunable White", "JBT_Gen_CCT_1", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_DIM, 0x9B, "EGLO Tunable White", "JBT_Gen_Dim_1", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0xA1, "EGLOLed Relax", "ELedRelax", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0xA2, "EGLOLed Stripe", "ELedStripe", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0xA3, "EGLOLed Plus", "ELedPlus", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_TW, 0xA4, "EGLOLed Plus TW", "ELedPlus-TW", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_DIM, 0xA5, "EGLOLed Plus Dimmable", "ELedPlus-Dimm", MANUFACTURER_EGLO);
    this->add_device(DEVICE_TYPE_TW, 0xA6, "EGLOBulb", "ESMLFm-w6-TW", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_DIM, 0xA7, "EGLOBulb", "ESMLFm-w6-Dimm", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0xA8, "ECeiling square", "ECeiling-24W-square", MANUFACTURER_EGLO);
    this->add_device(DEVICE_TYPE_RGB, 0xA9, "EGLO RGB+TW", "EGLO-RGB-TW-IPSU", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_TW, 0xAA, "EGLO Tunable White", "EGLO-TW-IPSU", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_RGB, 0xAC, "EGLO frameless", "EPanel-Frameless", MANUFACTURER_EGLO);
    // this->add_device(DEVICE_TYPE_TW, 0xAD, "EGLO Tunable White", "EDoubleWhite-ipsu", MANUFACTURER_EGLO);

    this->add_device(DEVICE_TYPE_PLUG, 0x97, "EGLO PLUG PLUS", "SMPWBm10AUS", MANUFACTURER_EGLO, "mdi:power-socket-au");
    this->add_device(DEVICE_TYPE_PLUG, 0x9E, "EGLO PLUG PLUS", "SMPWBm10AUSb", MANUFACTURER_EGLO,
                     "mdi:power-socket-au");
    this->add_device(DEVICE_TYPE_PLUG, 0x90, "EGLO PLUG PLUS", "SMPWBm10CH", MANUFACTURER_EGLO, "mdi:power-socket-ch");
    this->add_device(DEVICE_TYPE_PLUG, 0xA0, "EGLO PLUG PLUS", "SMPWBm10CHb", MANUFACTURER_EGLO, "mdi:power-socket-ch");
    this->add_device(DEVICE_TYPE_PLUG, 0x67, "EGLO PLUG PLUS", "SMPWBm10FR", MANUFACTURER_EGLO, "mdi:power-socket-fr");
    this->add_device(DEVICE_TYPE_PLUG, 0x84, "EGLO PLUG PLUS", "SMPWBm10FRa", MANUFACTURER_EGLO, "mdi:power-socket-fr");
    this->add_device(DEVICE_TYPE_PLUG, 0x9C, "EGLO PLUG PLUS", "SMPWBm10FRb", MANUFACTURER_EGLO, "mdi:power-socket-fr");
    this->add_device(DEVICE_TYPE_PLUG, 0x68, "EGLO PLUG PLUS", "SMPWBm10GE", MANUFACTURER_EGLO, "mdi:power-socket-de");
    this->add_device(DEVICE_TYPE_PLUG, 0x85, "EGLO PLUG PLUS", "SMPWBm10GEa", MANUFACTURER_EGLO, "mdi:power-socket-de");
    this->add_device(DEVICE_TYPE_PLUG, 0x9D, "EGLO PLUG PLUS", "SMPWBm10GEb", MANUFACTURER_EGLO, "mdi:power-socket-de");
    this->add_device(DEVICE_TYPE_PLUG, 0x8F, " EGLO PLUG PLUS ", " SMPWBm10UK ", MANUFACTURER_EGLO,
                     "mdi:power-socket-uk");
    this->add_device(DEVICE_TYPE_PLUG, 0x9F, "EGLO PLUG PLUS", "SMPWBm10UKb", MANUFACTURER_EGLO, "mdi:power-socket-uk");
    this->add_device(DEVICE_TYPE_PLUG, 0x8B, "EGLO PLUG", "ESMP-Bm10-AUS", MANUFACTURER_EGLO, "mdi:power-socket-au");
    this->add_device(DEVICE_TYPE_PLUG, 0x8D, "EGLO PLUG", "ESMP-Bm10-CH", MANUFACTURER_EGLO, "mdi:power-socket-ch");
    this->add_device(DEVICE_TYPE_PLUG, 0x62, "EGLO PLUG", "ESMP-Bm10-FR", MANUFACTURER_EGLO, "mdi:power-socket-fr");
    this->add_device(DEVICE_TYPE_PLUG, 0x63, "EGLO PLUG", "ESMP-Bm10-GE", MANUFACTURER_EGLO, "mdi:power-socket-de");
    this->add_device(DEVICE_TYPE_PLUG, 0x8C, "EGLO PLUG", "ESMP-Bm10-UK", MANUFACTURER_EGLO, "mdi:power-socket-uk");
  }

  DeviceInfo *get_by_product_id(int product_id) {
    if (device_info.count(product_id)) {
      return device_info[product_id];
    }

    if (devices.count(product_id)) {
      return this->create_device_info(product_id, devices[product_id]);
    }

    return this->create_device_info(DEVICE_TYPE_DIM, product_id, "Unknown device type",
                                    this->get_product_code_as_hex_string(product_id), MANUFACTURER_AWOX,
                                    "mdi:lightbulb-help-outline");
  }
};

}  // namespace awox_mesh
}  // namespace esphome
