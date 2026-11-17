#pragma once

#ifdef USE_ESP32

namespace esphome {
namespace awox_mesh {

class AwoxMesh;
class MeshDestination;

/// A Home Assistant entity (light or switch) bound to one mesh device or group.
class MeshEntity {
 public:
  MeshEntity(AwoxMesh *mesh, int dest) : mesh_(mesh), dest_(dest) {}

  int get_dest() const { return this->dest_; }

  /// Called by the hub once the mesh has reported the device or group this entity controls.
  void set_destination(MeshDestination *destination) { this->destination_ = destination; }

  /// Push the state last reported by the mesh to Home Assistant.
  virtual void publish_mesh_state() = 0;

 protected:
  AwoxMesh *const mesh_;
  const int dest_;
  MeshDestination *destination_{nullptr};
};

}  // namespace awox_mesh
}  // namespace esphome

#endif
