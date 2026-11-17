#pragma once

#ifdef USE_ESP32
#ifdef USE_LIGHT

#include "esphome/components/light/light_effect.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"

#include "mesh_destination.h"
#include "mesh_entity.h"

namespace esphome {
namespace awox_mesh {

/// Placeholder effect: the effect itself runs inside the bulb, write_state() sends the command to start it.
class AwoxMeshEffect : public light::LightEffect {
 public:
  explicit AwoxMeshEffect(const char *name) : light::LightEffect(name) {}
  void apply() override {}
};

class AwoxMeshLight : public light::LightOutput, public MeshEntity {
 public:
  AwoxMeshLight(AwoxMesh *mesh, int dest, int device_type) : MeshEntity(mesh, dest), device_type_(device_type) {}

  light::LightTraits get_traits() override;
  void setup_state(light::LightState *state) override;
  void write_state(light::LightState *state) override;

  void publish_mesh_state() override;

 protected:
  bool has_color_() const;
  bool has_color_temperature_() const;

  const int device_type_;
  light::LightState *state_{nullptr};
  // Commands are only sent once the mesh has reported the current state, so a restored or
  // default state at boot never overrides what the bulb is actually doing.
  bool synced_{false};
  MeshDestinationState last_published_{};

  AwoxMeshEffect color_loop_effect_{"Color loop"};
  AwoxMeshEffect candle_effect_{"Candle"};
};

}  // namespace awox_mesh
}  // namespace esphome

#endif  // USE_LIGHT
#endif  // USE_ESP32
