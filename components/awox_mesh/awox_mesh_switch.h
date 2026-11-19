#pragma once

#ifdef USE_ESP32
#ifdef USE_SWITCH

#include "esphome/components/switch/switch.h"
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

#include "mesh_entity.h"

namespace esphome {
namespace awox_mesh {

class AwoxMeshSwitch : public switch_::Switch, public MeshEntity {
 public:
  AwoxMeshSwitch(AwoxMesh *mesh, int dest) : MeshEntity(mesh, dest) {}

  void publish_mesh_state() override;

#ifdef USE_SENSOR
  void set_power_sensor(sensor::Sensor *sensor) { this->power_sensor_ = sensor; }
  void set_voltage_sensor(sensor::Sensor *sensor) { this->voltage_sensor_ = sensor; }

  bool has_power_measurement() const override {
    return this->power_sensor_ != nullptr || this->voltage_sensor_ != nullptr;
  }
  void publish_power_measurement(float power, float voltage) override;
#endif

 protected:
  void write_state(bool state) override;

  bool synced_{false};
#ifdef USE_SENSOR
  sensor::Sensor *power_sensor_{nullptr};
  sensor::Sensor *voltage_sensor_{nullptr};
#endif
};

}  // namespace awox_mesh
}  // namespace esphome

#endif  // USE_SWITCH
#endif  // USE_ESP32
