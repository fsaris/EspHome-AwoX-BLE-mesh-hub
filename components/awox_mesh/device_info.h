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

class MeshLightWhiteTemperature : public DeviceInfo {
 public:
  MeshLightWhiteTemperature(std::string product_id, std::string name, std::string model, std::string manufacturer,
                            std::string icon = "")
      : DeviceInfo(product_id, name, model, manufacturer, icon) {
    this->component_type = "light";

    this->add_feature(FEATURE_LIGHT_MODE);
    this->add_feature(FEATURE_WHITE_BRIGHTNESS);
    this->add_feature(FEATURE_WHITE_TEMPERATURE);
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
    this->add_device(new MeshLightColor("0013", "SmartLIGHT Color Mesh 9", "SMLm_C9", "AwoX"));
    this->add_device(new MeshLightWhiteTemperature("0014", "SmartLIGHT White Mesh 13W", "SMLm_W13", "AwoX"));
    this->add_device(new MeshLightColor("0015", "SmartLIGHT Color Mesh 13W", "SMLm_C13", "AwoX"));
    this->add_device(new MeshLightWhiteTemperature("0016", "SmartLIGHT White Mesh 15W", "SMLm_W15", "AwoX"));
    this->add_device(new MeshLightColor("0017", "SmartLIGHT Color Mesh 15W", "SMLm_C15", "AwoX"));
    this->add_device(new MeshLightWhiteTemperature("0021", "SmartLIGHT White Mesh 9W", "SSMLm_w9", "AwoX"));
    this->add_device(new MeshLightColor("0022", "SmartLIGHT Color Mesh 9W", "SSMLm_c9", "AwoX"));
    this->add_device(new MeshLightColor("0023", "EGLOBulb A60 9W", "ESMLm_c9", "EGLO"));
    this->add_device(new MeshLightColor("0024", "Keria SmartLIGHT Color Mesh 9W", "KSMLm_c9", "KERIA"));
    this->add_device(new MeshLightColor("0025", "EGLOPanel 30X30", "EPanel_300", "EGLO"));
    this->add_device(new MeshLightColor("0026", "EGLOPanel 60X60", "EPanel_600", "EGLO"));
    this->add_device(new MeshLightColor("0027", "EGLO Ceiling DOWNLIGHT", "EMod_Ceil", "EGLO"));
    this->add_device(new MeshLightColor("0029", "EGLOBulb G95 13W", "ESMLm_c13g", "EGLO"));
    this->add_device(new MeshLightColor("002A", "Keria SmartLIGHT Color Mesh 13W Globe", "KSMLm_c13g", "KERIA"));
    this->add_device(new MeshLightColor("002B", "SmartLIGHT Color Mesh 13W Globe", "SMLm_c13g", "AwoX"));
    this->add_device(new MeshLightColor("0030", "EGLOPanel 30X120", "EPanel_120", "EGLO"));
    this->add_device(new MeshLightColor("0032", "Spot 120", "EGLOSpot 120/w", "EGLO", "mdi:wall-sconce-flat"));
    this->add_device(new MeshLightColor("0033", "Spot 170", "EGLOSpot 170/w", "EGLO", "mdi:wall-sconce-flat"));
    this->add_device(new MeshLightColor("0034", "Spot 225", "EGLOSpot 225/w", "EGLO", "mdi:wall-sconce-flat"));
    this->add_device(new MeshLightColor("0035", "Giron-C 17W", "EGLO 32589", "EGLO", "mdi:wall-sconce-flat"));
    this->add_device(new MeshLightColor("0036", "EGLO Ceiling GIRON 30", "ECeil_g38", "EGLO"));
    this->add_device(new MeshLightColor("0037", "SmartLIGHT Color Mesh 5W GU10", "SMLm_c5_GU10", "AwoX"));
    this->add_device(new MeshLightColor("0038", "SmartLIGHT Color Mesh 5W E14", "SMLm_c5_E14", "AwoX"));
    this->add_device(new MeshLightColor("003A", "Keria SmartLIGHT Color Mesh 5W GU10", "KSMLm_c5_GU10", "KERIA"));
    this->add_device(new MeshLightColor("003B", "Keria SmartLIGHT Color Mesh 5W E14", "KSMLm_c5_E14", "KERIA"));
    this->add_device(new MeshLightColor("003C", "SmartLIGHT Color Mesh 5W GU10", "ESMLm_c5_GU10", "EGLO"));
    this->add_device(new MeshLightColor("003D", "SmartLIGHT Color Mesh 5W E14", "ESMLm_c5_E14", "EGLO"));
    this->add_device(new MeshLightColor("003F", "EGLO Surface round", "EFueva_225r", "EGLO"));
    this->add_device(new MeshLightColor("0040", "EGLO Surface square", "EFueva_225s", "EGLO"));
    this->add_device(new MeshLightColor("0041", "EGLO Surface round", "EFueva_300r", "EGLO"));
    this->add_device(new MeshLightColor("0042", "EGLO Surface square", "EFueva_300s", "EGLO"));
    this->add_device(new MeshLightColor("0043", "SmartLIGHT Color Mesh 9W", "SMLm_c9s", "AwoX"));
    this->add_device(new MeshLightColor("0044", "SmartLIGHT Color Mesh 13W", "SMLm_c13gs", "AwoX"));
    this->add_device(new MeshLightColor("0045", "EGLOBulb A60 9W", "ESMLm_c9s", "EGLO"));
    this->add_device(new MeshLightColor("0046", "EGLOBulb G95 13W", "ESMLm_c13gs", "EGLO"));
    this->add_device(new MeshLightColor("0047", "Keria SmartLIGHT Color Mesh 9W", "KSMLm_c9s", "KERIA"));
    this->add_device(new MeshLightColor("0048", "Keria SmartLIGHT Color Mesh 13W Glob", "KSMLm_c13gs", "KERIA"));
    this->add_device(new MeshLightWhite("0049", "EGLOBulb A60 Warm", "ESMLm_w9w", "EGLO"));
    this->add_device(new MeshLightWhite("004A", "EGLOBulb A60 Neutral", "ESMLm_w9n", "EGLO"));
    this->add_device(new MeshLightColor("004B", "EGLO Ceiling", "ECeiling_30", "EGLO"));
    this->add_device(new MeshLightColor("004C", "EGLO Pendant", "EPendant_30", "EGLO"));
    this->add_device(new MeshLightColor("004D", "EGLO Pendant", "EPendant_20", "EGLO"));
    this->add_device(new MeshLightColor("004E", "EGLO Stripled 3m", "EStrip_3m", "EGLO"));
    this->add_device(new MeshLightColor("004F", "EGLO Stripled 5m", "EStrip_5m", "EGLO"));
    this->add_device(new MeshLightWhite("0050", "Outdoor", "EOutdoor_w14w", "EGLO"));
    this->add_device(new MeshLightColor("0051", "EGLOSpot", "ETriSpot_85", "EGLO"));
    this->add_device(new MeshLightColor("0053", "SmartLIGHT Color Mesh 9W", "SMLm_c9i", "AwoX"));
    this->add_device(new MeshLightColor("0054", "EGLOBulb A60 9W", "ESMLm_c9i", "EGLO"));
    this->add_device(new MeshLightColor("0055", "Keria SmartLIGHT Color Mesh 9W", "KSMLm_c9i", "KERIA"));
    this->add_device(new MeshLightColor("0056", "EGLOPanel 62X62", "EPanel_620", "EGLO"));
    this->add_device(new MeshLightColor("0057", "EGLOPanel 45X45", "EPanel_450", "EGLO"));
    this->add_device(new MeshLightColor("0059", "SmartLIGHT Color Mesh 13W Globe", "SMLm_c13gi", "AwoX"));
    this->add_device(new MeshLightColor("005A", "EGLOBulb G95 13W", "ESMLm_c13gi", "EGLO"));
    this->add_device(new MeshLightColor("005B", "Keria SmartLIGHT Color Mesh 13W Globe", "KSMLm_c13gi", "KERIA"));
    this->add_device(new MeshLightColor("005C", "SmartLIGHT Color Mesh 9W", "SSMLm_c9i", "AwoX"));
    this->add_device(new MeshLightWhiteTemperature("0064", "SmartLIGHT White Mesh 9W", "SMLm_w9", "AwoX"));
    this->add_device(new MeshLightWhiteTemperature("0065", "SmartLIGHT White Mesh 9W", "ESMLm_w9", "EGLO"));
    this->add_device(new MeshLightColor("0069", "Ceiling GIRON 60", "ECeil_g60", "EGLO"));
    this->add_device(new MeshLightWhiteTemperature("006A", "SmartLIGHT Bulb A60 Warm", "SMLm_w9w", "AwoX"));
    this->add_device(new MeshLightWhiteTemperature("006F", "EGLOBulb Filament G80", "ESMLFm-w6-G80", "EGLO"));
    this->add_device(new MeshLightWhiteTemperature("0071", "EGLOBulb Filament ST64", "ESMLFm-w6-ST64", "EGLO"));
    this->add_device(new MeshLightWhiteTemperature("0075", "EGLOBulb Filament G95", "ESMLFm-w6-G95", "EGLO"));
    this->add_device(new MeshLightColor("0077", "EGLO Spot", "ESpot_c5", "EGLO"));
    this->add_device(new MeshLightColor("0078", "EGLO Fraioli", "EFraioli_c17", "EGLO"));
    this->add_device(new MeshLightColor("0079", "EGLO Frattina", "EFrattina_c18", "EGLO"));
    this->add_device(new MeshLightColor("007A", "EGLO Frattina", "EFrattina_c27", "EGLO"));
    this->add_device(new MeshLightColor("007B", "EGLOPanel 30 Round", "EPanel_r300", "EGLO"));
    this->add_device(new MeshLightColor("007C", "EGLOPanel 45 Round", "EPanel_r450", "EGLO"));
    this->add_device(new MeshLightColor("007D", "EGLOPanel 60 Round", "EPanel_r600", "EGLO"));
    this->add_device(new MeshLightColor("007E", "EGLOPanel 10X120", "EPanel_120_10", "EGLO"));
    this->add_device(new MeshLightWhiteTemperature("0080", "EPanel white round", "EPanel_w_round", "EGLO"));
    this->add_device(new MeshLightWhiteTemperature("0081", "EPanel white square", "EPanel_w_square", "EGLO"));
    this->add_device(new MeshLightWhiteTemperature("0082", "EPanel white rectangle", "EPanel_w_rect", "EGLO"));
    this->add_device(new MeshLightWhiteTemperature("0083", "ECeiling white round", "ECeiling-w", "EGLO"));
    this->add_device(new MeshLightWhiteTemperature("0087", "EGLO Tunable White", "EDoubleWhite", "EGLO"));
    this->add_device(new MeshLightColor("0088", "EGLO Ceiling GIRON 80", "ECeil_g80", "EGLO"));
    this->add_device(new MeshLightWhiteTemperature("0089", "Outdoor Marchesa-C", "EMarchesa_C", "EGLO"));
    this->add_device(new MeshLightWhiteTemperature("008A", "Outdoor Francari-C", "EFrancari_C", "EGLO"));
    this->add_device(new MeshLightColor("0092", "EPanel square", "EPanel_36W_square", "EGLO"));
    this->add_device(new MeshLightWhite("0094", "EGLOBulb Filament ST64", "ESMLFm-w6w-ST64", "EGLO"));
    this->add_device(new MeshLightWhite("0095", "EGLOBulb Filament G95", "ESMLFm-w6w-G95", "EGLO"));
    this->add_device(new MeshLightColor("0096", "EGLO RGB+TW", "EGLO-RGB-TW", "EGLO"));
    this->add_device(new MeshLightWhiteTemperature("0097", "EGLO Tunable White", "EGLO-TW", "EGLO"));
    this->add_device(new MeshLightColor("0099", "EGLO RGB+TW", "EGLO-RGB-TW", "EGLO"));
    this->add_device(new MeshLightWhiteTemperature("009A", "EGLO Tunable White", "JBT_Gen_CCT_1", "EGLO"));
    this->add_device(new MeshLightWhite("009B", "EGLO Tunable White", "JBT_Gen_Dim_1", "EGLO"));
    this->add_device(new MeshLightColor("00A1", "EGLOLed Relax", "ELedRelax", "EGLO"));
    this->add_device(new MeshLightColor("00A2", "EGLOLed Stripe", "ELedStripe", "EGLO"));
    this->add_device(new MeshLightColor("00A3", "EGLOLed Plus", "ELedPlus", "EGLO"));
    this->add_device(new MeshLightWhiteTemperature("00A4", "EGLOLed Plus TW", "ELedPlus-TW", "EGLO"));
    this->add_device(new MeshLightWhite("00A5", "EGLOLed Plus Dimmable", "ELedPlus-Dimm", "EGLO"));
    this->add_device(new MeshLightWhiteTemperature("00A6", "EGLOBulb", "ESMLFm-w6-TW", "EGLO"));
    this->add_device(new MeshLightWhite("00A7", "EGLOBulb", "ESMLFm-w6-Dimm", "EGLO"));
    this->add_device(new MeshLightColor("00A8", "ECeiling square", "ECeiling-24W-square", "EGLO"));
    this->add_device(new MeshLightColor("00A9", "EGLO RGB+TW", "EGLO-RGB-TW-IPSU", "EGLO"));
    this->add_device(new MeshLightWhiteTemperature("00AA", "EGLO Tunable White", "EGLO-TW-IPSU", "EGLO"));
    this->add_device(new MeshLightColor("00AC", "EGLO frameless", "EPanel-Frameless", "EGLO"));
    this->add_device(new MeshLightWhiteTemperature("00AD", "EGLO Tunable White", "EDoubleWhite-ipsu", "EGLO"));

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
