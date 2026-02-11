#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "../waterfurnace.h"

namespace esphome {
namespace waterfurnace {

class WaterFurnaceBinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  void set_parent(WaterFurnace *parent) { parent_ = parent; }
  void set_register_address(uint16_t addr) { register_address_ = addr; }
  void set_bitmask(uint16_t mask) { bitmask_ = mask; }

 protected:
  WaterFurnace *parent_{nullptr};
  uint16_t register_address_{0};
  uint16_t bitmask_{0};
};

}  // namespace waterfurnace
}  // namespace esphome
