#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "../waterfurnace.h"

#include <string>

namespace esphome {
namespace waterfurnace {

class WaterFurnaceSensor : public sensor::Sensor, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  void set_parent(WaterFurnace *parent) { parent_ = parent; }
  void set_register_address(uint16_t addr) { register_address_ = addr; }
  void set_register_type(const std::string &type) { register_type_ = type; }
  void set_is_32bit(bool is_32bit) { is_32bit_ = is_32bit; }
  void set_capability(const std::string &cap) { capability_ = cap; }

 protected:
  void on_register_value_(uint16_t value);
  void on_register_value_hi_(uint16_t value);

  WaterFurnace *parent_{nullptr};
  uint16_t register_address_{0};
  std::string register_type_;
  bool is_32bit_{false};
  std::string capability_{"none"};

  // For 32-bit values, cache the high word
  uint16_t hi_word_{0};
  bool has_hi_word_{false};

  // Change detection
  float last_published_value_{NAN};
  bool has_published_{false};
};

}  // namespace waterfurnace
}  // namespace esphome
