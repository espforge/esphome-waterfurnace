#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "../waterfurnace.h"

#include <string>

namespace esphome {
namespace waterfurnace {

class WaterFurnaceTextSensor : public text_sensor::TextSensor, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  void set_parent(WaterFurnace *parent) { parent_ = parent; }
  void set_sensor_type(const std::string &type) { sensor_type_ = type; }

 protected:
  void on_fault_register_(uint16_t value);
  void on_system_outputs_(uint16_t value);
  void publish_state_dedup_(const std::string &state);

  WaterFurnace *parent_{nullptr};
  std::string sensor_type_;
  std::string last_published_state_;
};

}  // namespace waterfurnace
}  // namespace esphome
