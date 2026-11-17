#ifdef USE_ESP32
#ifdef USE_SWITCH

#include "awox_mesh_switch.h"
#include "awox_mesh.h"

#include "esphome/core/log.h"

namespace esphome {
namespace awox_mesh {

static const char *const TAG = "awox.switch";

void AwoxMeshSwitch::write_state(bool state) {
  if (!this->synced_) {
    ESP_LOGD(TAG, "[%d] State not reported by the mesh yet, not sending", this->dest_);
    return;
  }
  this->destination_->state = state;
  this->mesh_->set_power(this->dest_, state);
  this->publish_state(state);
}

void AwoxMeshSwitch::publish_mesh_state() {
  if (this->destination_ == nullptr)
    return;
  if (this->synced_ && this->state == this->destination_->state)
    return;
  this->synced_ = true;
  this->publish_state(this->destination_->state);
}

}  // namespace awox_mesh
}  // namespace esphome

#endif  // USE_SWITCH
#endif  // USE_ESP32
