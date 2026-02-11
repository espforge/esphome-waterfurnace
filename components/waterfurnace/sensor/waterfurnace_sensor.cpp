#include "waterfurnace_sensor.h"
#include "esphome/core/log.h"
#include <cmath>

namespace esphome {
namespace waterfurnace {

static const char *const TAG = "waterfurnace.sensor";

void WaterFurnaceSensor::setup() {
  if (this->is_32bit_) {
    // 32-bit value: register hi word at address, lo word at address+1
    this->parent_->register_listener(this->register_address_,
                                      [this](uint16_t v) { this->on_register_value_hi_(v); });
    this->parent_->register_listener(this->register_address_ + 1,
                                      [this](uint16_t v) { this->on_register_value_(v); });
  } else {
    this->parent_->register_listener(this->register_address_,
                                      [this](uint16_t v) { this->on_register_value_(v); });
  }
}

void WaterFurnaceSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "WaterFurnace Sensor '%s':", this->get_name().c_str());
  ESP_LOGCONFIG(TAG, "  Register: %u (type: %s, 32bit: %s)",
                this->register_address_, this->register_type_.c_str(),
                YESNO(this->is_32bit_));
}

void WaterFurnaceSensor::on_register_value_hi_(uint16_t value) {
  this->hi_word_ = value;
  this->has_hi_word_ = true;
}

void WaterFurnaceSensor::on_register_value_(uint16_t value) {
  float result;

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
  } else if (this->register_type_ == "tenths") {
    result = value / 10.0f;
  } else if (this->register_type_ == "signed") {
    result = static_cast<float>(static_cast<int16_t>(value));
  } else if (this->register_type_ == "hundredths") {
    result = value / 100.0f;
  } else {
    // "unsigned" or default
    result = static_cast<float>(value);
  }

  if (this->has_published_ && std::abs(result - this->last_published_value_) < 0.001f)
    return;
  this->last_published_value_ = result;
  this->has_published_ = true;
  this->publish_state(result);
}

}  // namespace waterfurnace
}  // namespace esphome
