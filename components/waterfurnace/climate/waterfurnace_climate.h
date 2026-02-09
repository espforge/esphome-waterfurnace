#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "../waterfurnace.h"

namespace esphome {
namespace waterfurnace {

class WaterFurnaceClimate : public climate::Climate, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  void set_parent(WaterFurnace *parent) { parent_ = parent; }
  void set_zone(uint8_t zone) { zone_ = zone; }

  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

 protected:
  // Register callbacks
  void on_ambient_temp_(uint16_t value);
  void on_heating_setpoint_(uint16_t value);
  void on_cooling_setpoint_(uint16_t value);
  void on_mode_config_(uint16_t value);
  void on_fan_config_(uint16_t value);

  // IZ2 zone config register callbacks
  void on_iz2_config1_(uint16_t value);
  void on_iz2_config2_(uint16_t value);

  // Helper to get write register addresses
  uint16_t get_mode_write_reg_() const;
  uint16_t get_heating_sp_write_reg_() const;
  uint16_t get_cooling_sp_write_reg_() const;
  uint16_t get_fan_mode_write_reg_() const;

  void publish_state_if_changed_();

  WaterFurnace *parent_{nullptr};
  uint8_t zone_{0};  // 0 = single zone, 1-6 = IZ2 zone number

  // Cached IZ2 config registers for extracting packed values
  uint16_t iz2_config1_{0};
  uint16_t iz2_config2_{0};

  // Change detection for publish_state dedup
  float last_current_temp_{NAN};
  float last_target_low_{NAN};
  float last_target_high_{NAN};
  climate::ClimateMode last_mode_{climate::CLIMATE_MODE_OFF};
  optional<climate::ClimateFanMode> last_fan_mode_{};
  optional<climate::ClimatePreset> last_preset_{};
  std::string current_custom_fan_mode_{};
  std::string last_custom_fan_mode_{};
  bool has_published_{false};
};

}  // namespace waterfurnace
}  // namespace esphome
