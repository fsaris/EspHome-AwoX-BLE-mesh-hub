#pragma once

#ifdef USE_ESP32
#ifdef USE_SWITCH

#include "esphome/components/switch/switch.h"

#include "mesh_entity.h"

namespace esphome {
namespace awox_mesh {

class AwoxMeshSwitch : public switch_::Switch, public MeshEntity {
 public:
  AwoxMeshSwitch(AwoxMesh *mesh, int dest) : MeshEntity(mesh, dest) {}

  void publish_mesh_state() override;

 protected:
  void write_state(bool state) override;

  bool synced_{false};
};

}  // namespace awox_mesh
}  // namespace esphome

#endif  // USE_SWITCH
#endif  // USE_ESP32
