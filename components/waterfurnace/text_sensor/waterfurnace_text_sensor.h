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
  void on_active_dehumidify_(uint16_t value);
  void on_compressor_delay_(uint16_t value);
  void compute_system_mode_();
  void publish_state_dedup_(const std::string &state);

  WaterFurnace *parent_{nullptr};
  std::string sensor_type_;
  std::string last_published_state_;

  // Cached register values for system mode computation
  uint16_t system_outputs_{0};
  uint16_t active_dehumidify_{0};
  uint16_t compressor_delay_{0};
  bool has_outputs_{false};
};

}  // namespace waterfurnace
}  // namespace esphome
