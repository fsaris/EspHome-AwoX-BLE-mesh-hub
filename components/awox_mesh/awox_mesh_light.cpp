#ifdef USE_ESP32
#ifdef USE_LIGHT

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>

#include "awox_mesh_light.h"
#include "awox_mesh.h"

#include "esphome/core/log.h"
#include "esphome/core/macros.h"
#include "esphome/core/version.h"

namespace esphome {
namespace awox_mesh {

static const char *const TAG = "awox.light";

static constexpr uint32_t EFFECT_NONE = 0;
// Effect indexes are 1-based, in the order passed to add_effects()
static constexpr uint32_t EFFECT_COLOR_LOOP = 1;
static constexpr uint32_t EFFECT_CANDLE = 2;

static constexpr float MIN_MIREDS = 153.0f;
static constexpr float MAX_MIREDS = 370.0f;
static constexpr int MAX_TEMPERATURE = 0x7f;
static constexpr int MIN_WHITE_BRIGHTNESS = 1;
static constexpr int MAX_WHITE_BRIGHTNESS = 0x7f;
static constexpr int MIN_COLOR_BRIGHTNESS = 0x0a;
static constexpr int MAX_COLOR_BRIGHTNESS = 0x64;

// All conversions below round-trip exactly (mesh -> float -> mesh), so a state reported by the
// mesh that comes back through write_state() is recognised as unchanged and not sent again.

static float white_brightness_to_float(int value) {
  return (float) std::max(value, MIN_WHITE_BRIGHTNESS) / (float) MAX_WHITE_BRIGHTNESS;
}

static int float_to_white_brightness(float value) {
  return std::clamp((int) std::lround(value * MAX_WHITE_BRIGHTNESS), MIN_WHITE_BRIGHTNESS, MAX_WHITE_BRIGHTNESS);
}

static float color_brightness_to_float(int value) {
  constexpr int steps = MAX_COLOR_BRIGHTNESS - MIN_COLOR_BRIGHTNESS + 1;
  return (float) (std::max(value, MIN_COLOR_BRIGHTNESS) - MIN_COLOR_BRIGHTNESS + 1) / (float) steps;
}

static int float_to_color_brightness(float value) {
  constexpr int steps = MAX_COLOR_BRIGHTNESS - MIN_COLOR_BRIGHTNESS + 1;
  return std::clamp((int) std::lround(value * steps) - 1 + MIN_COLOR_BRIGHTNESS, MIN_COLOR_BRIGHTNESS,
                    MAX_COLOR_BRIGHTNESS);
}

static float temperature_to_mireds(int value) {
  return MIN_MIREDS + (float) value * (MAX_MIREDS - MIN_MIREDS) / (float) MAX_TEMPERATURE;
}

static int mireds_to_temperature(float mireds) {
  return std::clamp((int) std::lround((mireds - MIN_MIREDS) * MAX_TEMPERATURE / (MAX_MIREDS - MIN_MIREDS)), 0,
                    MAX_TEMPERATURE);
}

// ESPHome scales colors so the brightest channel is 255, compare the mesh color the same way
static bool same_color(const MeshDestination *destination, int red, int green, int blue) {
  const int max = std::max({(int) destination->R, (int) destination->G, (int) destination->B});
  if (max == 0)
    return false;
  return std::lround(destination->R * 255.0f / max) == red && std::lround(destination->G * 255.0f / max) == green &&
         std::lround(destination->B * 255.0f / max) == blue;
}

bool AwoxMeshLight::has_color_() const { return this->device_type_ == DEVICE_TYPE_RGB; }

bool AwoxMeshLight::has_color_temperature_() const {
  return this->device_type_ == DEVICE_TYPE_RGB || this->device_type_ == DEVICE_TYPE_TW;
}

light::LightTraits AwoxMeshLight::get_traits() {
  auto traits = light::LightTraits();
  if (this->has_color_()) {
    traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::COLOR_TEMPERATURE});
  } else if (this->has_color_temperature_()) {
    traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
  } else {
    traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
  }
  if (this->has_color_temperature_()) {
    traits.set_min_mireds(MIN_MIREDS);
    traits.set_max_mireds(MAX_MIREDS);
  }
  return traits;
}

void AwoxMeshLight::setup_state(light::LightState *state) {
  this->state_ = state;
  if (this->has_color_()) {
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 11, 0)
    state->add_effects(this->effects_, std::size(this->effects_));
#else
    state->add_effects({this->effects_[0], this->effects_[1]});
#endif
  }
}

void AwoxMeshLight::write_state(light::LightState *state) {
  MeshDestination *destination = this->destination_;
  if (!this->synced_ || destination == nullptr) {
    ESP_LOGD(TAG, "[%d] State not reported by the mesh yet, not sending", this->dest_);
    return;
  }
  // Each step of a transition would be a mesh command, only send where it ends
  if (state->is_transformer_active())
    return;

  const light::LightColorValues &values = state->current_values;
  const int dest = this->dest_;

  if (!values.is_on()) {
    if (destination->state) {
      destination->state = false;
      this->mesh_->set_power(dest, false);
    }
    this->last_published_ = destination->state_as_char();
    return;
  }

  bool sent = false;
  bool resend_color = false;
  const uint32_t effect = state->get_current_effect_index();

  if (effect == EFFECT_COLOR_LOOP && !destination->sequence_mode) {
    destination->sequence_mode = true;
    destination->candle_mode = false;
    this->mesh_->set_sequence(dest, 0);
    sent = true;
  } else if (effect == EFFECT_CANDLE && !destination->candle_mode) {
    destination->sequence_mode = false;
    destination->candle_mode = true;
    this->mesh_->set_candle_mode(dest);
    sent = true;
  } else if (effect == EFFECT_NONE && (destination->sequence_mode || destination->candle_mode)) {
    // The bulb only leaves an effect when it gets a color or temperature again
    destination->sequence_mode = false;
    destination->candle_mode = false;
    resend_color = true;
  }

  if (effect == EFFECT_NONE) {
    if (values.get_color_mode() == light::ColorMode::RGB) {
      const int red = (int) std::lround(values.get_red() * 255.0f);
      const int green = (int) std::lround(values.get_green() * 255.0f);
      const int blue = (int) std::lround(values.get_blue() * 255.0f);
      if (resend_color || !destination->color_mode || !same_color(destination, red, green, blue)) {
        destination->color_mode = true;
        destination->R = red;
        destination->G = green;
        destination->B = blue;
        this->mesh_->set_color(dest, red, green, blue);
        sent = true;
      }
    } else if (values.get_color_mode() == light::ColorMode::COLOR_TEMPERATURE) {
      const int temperature = mireds_to_temperature(values.get_color_temperature());
      if (resend_color || destination->color_mode || temperature != destination->temperature) {
        destination->color_mode = false;
        destination->temperature = temperature;
        this->mesh_->set_white_temperature(dest, temperature);
        sent = true;
      }
    }
  }

  if (destination->color_mode) {
    const int brightness = float_to_color_brightness(values.get_brightness());
    if (brightness != destination->color_brightness) {
      destination->color_brightness = brightness;
      this->mesh_->set_color_brightness(dest, brightness);
      sent = true;
    }
  } else {
    const int brightness = float_to_white_brightness(values.get_brightness());
    if (brightness != destination->white_brightness) {
      destination->white_brightness = brightness;
      this->mesh_->set_white_brightness(dest, brightness);
      sent = true;
    }
  }

  // Any of the commands above also turns the bulb on
  if (!destination->state) {
    destination->state = true;
    if (!sent)
      this->mesh_->set_power(dest, true);
  }

  this->last_published_ = destination->state_as_char();
}

void AwoxMeshLight::publish_mesh_state() {
  MeshDestination *destination = this->destination_;
  if (this->state_ == nullptr || destination == nullptr)
    return;

  const MeshDestinationState current = destination->state_as_char();
  if (this->synced_ && memcmp(current.state, this->last_published_.state, sizeof(current.state)) == 0)
    return;
  this->synced_ = true;
  this->last_published_ = current;

  auto call = this->state_->make_call();
  // The mesh reports its state after every boot, nothing is worth writing to flash
  call.set_save(false);
  call.set_state(destination->state);

  if (destination->color_mode && this->has_color_()) {
    call.set_color_mode(light::ColorMode::RGB);
    if (destination->R > 0 || destination->G > 0 || destination->B > 0)
      call.set_rgb(destination->R / 255.0f, destination->G / 255.0f, destination->B / 255.0f);
    call.set_brightness(color_brightness_to_float(destination->color_brightness));
  } else if (this->has_color_temperature_()) {
    call.set_color_mode(light::ColorMode::COLOR_TEMPERATURE);
    call.set_color_temperature(temperature_to_mireds(destination->temperature));
    call.set_brightness(white_brightness_to_float(destination->white_brightness));
  } else {
    call.set_brightness(white_brightness_to_float(destination->white_brightness));
  }

  if (this->has_color_()) {
    uint32_t effect = EFFECT_NONE;
    if (destination->sequence_mode) {
      effect = EFFECT_COLOR_LOOP;
    } else if (destination->candle_mode) {
      effect = EFFECT_CANDLE;
    }
    if (effect != this->state_->get_current_effect_index())
      call.set_effect(effect);
  }

  call.perform();
}

}  // namespace awox_mesh
}  // namespace esphome

#endif  // USE_LIGHT
#endif  // USE_ESP32
