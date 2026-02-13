#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "../waterfurnace.h"

namespace esphome {
namespace waterfurnace {

class WaterFurnaceSwitch : public switch_::Switch, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  void set_parent(WaterFurnace *parent) { parent_ = parent; }
  void set_register_address(uint16_t addr) { register_address_ = addr; }
  void set_write_address(uint16_t addr) { write_address_ = addr; }
  void set_capability(const std::string &cap) { capability_ = capability_from_string(cap.c_str()); }

 protected:
  void write_state(bool state) override;

  WaterFurnace *parent_{nullptr};
  uint16_t register_address_{0};
  uint16_t write_address_{0};
  RegisterCapability capability_{RegisterCapability::NONE};
};

}  // namespace waterfurnace
}  // namespace esphome
