#include "waterfurnace_sensor.h"
#include "esphome/core/log.h"
#include <cmath>

namespace esphome {
namespace waterfurnace {

static const char *const TAG = "waterfurnace.sensor";

void WaterFurnaceSensor::setup() {
  auto cap = capability_from_string(this->capability_.c_str());
  if (this->is_32bit_) {
    // 32-bit value: register hi word at address, lo word at address+1
    this->parent_->register_listener(this->register_address_,
                                      [this](uint16_t v) { this->on_register_value_hi_(v); }, cap);
    this->parent_->register_listener(this->register_address_ + 1,
                                      [this](uint16_t v) { this->on_register_value_(v); }, cap);
  } else {
    this->parent_->register_listener(this->register_address_,
                                      [this](uint16_t v) { this->on_register_value_(v); }, cap);
  }
}

void WaterFurnaceSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "WaterFurnace Sensor '%s':", this->get_name().c_str());
  ESP_LOGCONFIG(TAG, "  Register: %u (type: %s, 32bit: %s, capability: %s)",
                this->register_address_, this->register_type_.c_str(),
                YESNO(this->is_32bit_), this->capability_.c_str());
}

void WaterFurnaceSensor::on_register_value_hi_(uint16_t value) {
  this->hi_word_ = value;
  this->has_hi_word_ = true;
}

void WaterFurnaceSensor::on_register_value_(uint16_t value) {
  float result;
  bool check_sentinel = false;

  if (this->is_32bit_) {
    if (!this->has_hi_word_)
      return;  // Wait for both words

    if (this->register_type_ == "uint32") {
      result = static_cast<float>(to_uint32(this->hi_word_, value));
    } else if (this->register_type_ == "int32") {
      result = static_cast<float>(to_int32(this->hi_word_, value));
    } else {
      result = static_cast<float>(to_uint32(this->hi_word_, value));
    }
  } else if (this->register_type_ == "signed_tenths") {
    result = static_cast<int16_t>(value) / 10.0f;
    check_sentinel = true;  // Check for -999.9 sentinel on tenths-based types
  } else if (this->register_type_ == "tenths") {
    result = value / 10.0f;
    check_sentinel = true;  // Check for -999.9 sentinel on tenths-based types
  } else if (this->register_type_ == "signed") {
    result = static_cast<float>(static_cast<int16_t>(value));
  } else if (this->register_type_ == "hundredths") {
    result = value / 100.0f;
  } else {
    // "unsigned" or default
    result = static_cast<float>(value);
  }

  // Check for sentinel values (sensor not available) and publish NaN instead
  // -999.9: signed_tenths where raw -9999 / 10 = -999.9
  //  999.9: tenths where raw 9999 / 10 = 999.9 (e.g. waterflow when not supported)
  if (check_sentinel && (std::abs(result - (-999.9f)) < 0.1f || std::abs(result - 999.9f) < 0.1f)) {
    result = NAN;
  }

  if (this->has_published_ && std::isnan(result) && std::isnan(this->last_published_value_))
    return;  // Both NaN, no change
  if (this->has_published_ && !std::isnan(result) && !std::isnan(this->last_published_value_) &&
      std::abs(result - this->last_published_value_) < 0.001f)
    return;
  this->last_published_value_ = result;
  this->has_published_ = true;
  this->publish_state(result);
}

}  // namespace waterfurnace
}  // namespace esphome
