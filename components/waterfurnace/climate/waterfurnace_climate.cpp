#include "waterfurnace_climate.h"
#include "esphome/core/log.h"
#include <cmath>

namespace esphome {
namespace waterfurnace {

static const char *const TAG = "waterfurnace.climate";

void WaterFurnaceClimate::setup() {
  if (this->zone_ == 0) {
    // Single zone mode - register for thermostat registers
    this->parent_->register_listener(REG_AMBIENT_TEMP, [this](uint16_t v) { this->on_ambient_temp_(v); });
    this->parent_->register_listener(REG_HEATING_SETPOINT, [this](uint16_t v) { this->on_heating_setpoint_(v); });
    this->parent_->register_listener(REG_COOLING_SETPOINT, [this](uint16_t v) { this->on_cooling_setpoint_(v); });
    this->parent_->register_listener(REG_MODE_CONFIG, [this](uint16_t v) { this->on_mode_config_(v); });
    this->parent_->register_listener(REG_FAN_CONFIG, [this](uint16_t v) { this->on_fan_config_(v); });
  } else {
    // IZ2 zone mode
    uint16_t base = REG_IZ2_ZONE_BASE + (this->zone_ - 1) * 3;
    this->parent_->register_listener(base, [this](uint16_t v) { this->on_ambient_temp_(v); });
    this->parent_->register_listener(base + 1, [this](uint16_t v) { this->on_iz2_config1_(v); });
    this->parent_->register_listener(base + 2, [this](uint16_t v) { this->on_iz2_config2_(v); });
  }
}

void WaterFurnaceClimate::dump_config() {
  ESP_LOGCONFIG(TAG, "WaterFurnace Climate:");
  if (this->zone_ == 0) {
    ESP_LOGCONFIG(TAG, "  Zone: Single zone");
  } else {
    ESP_LOGCONFIG(TAG, "  Zone: %d (IZ2)", this->zone_);
  }
}

climate::ClimateTraits WaterFurnaceClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE |
                           climate::CLIMATE_REQUIRES_TWO_POINT_TARGET_TEMPERATURE);

  // Visual settings aligned precisely to Fahrenheit
  traits.set_visual_min_temperature(4.44f);                        // 40°F
  traits.set_visual_max_temperature(37.22f);                       // 99°F
  traits.set_visual_target_temperature_step(0.5556f);              // 1°F
  traits.set_visual_current_temperature_step(0.5556f);             // 1°F

  // Supported modes
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_HEAT_COOL,
      climate::CLIMATE_MODE_COOL,
      climate::CLIMATE_MODE_HEAT,
  });

  // Supported fan modes
  traits.set_supported_fan_modes({
      climate::CLIMATE_FAN_AUTO,
      climate::CLIMATE_FAN_ON,
  });
  traits.set_supported_custom_fan_modes({"Intermittent"});

  // Supported presets (E-Heat as BOOST)
  traits.set_supported_presets({
      climate::CLIMATE_PRESET_NONE,
      climate::CLIMATE_PRESET_BOOST,
  });

  return traits;
}

void WaterFurnaceClimate::control(const climate::ClimateCall &call) {
  if (call.get_mode().has_value()) {
    auto mode = *call.get_mode();
    uint16_t wf_mode;
    switch (mode) {
      case climate::CLIMATE_MODE_OFF:
        wf_mode = MODE_OFF;
        break;
      case climate::CLIMATE_MODE_HEAT_COOL:
        wf_mode = MODE_AUTO;
        break;
      case climate::CLIMATE_MODE_COOL:
        wf_mode = MODE_COOL;
        break;
      case climate::CLIMATE_MODE_HEAT:
        // Check if BOOST preset is active for E-Heat
        wf_mode = MODE_HEAT;
        break;
      default:
        wf_mode = MODE_AUTO;
        break;
    }
    this->parent_->write_register(this->get_mode_write_reg_(), wf_mode);
  }

  if (call.get_preset().has_value()) {
    auto preset = *call.get_preset();
    if (preset == climate::CLIMATE_PRESET_BOOST) {
      // E-Heat mode
      this->parent_->write_register(this->get_mode_write_reg_(), MODE_EHEAT);
    }
  }

  if (call.get_target_temperature_low().has_value()) {
    float temp_f = *call.get_target_temperature_low() * 9.0f / 5.0f + 32.0f;
    uint16_t raw = static_cast<uint16_t>(temp_f * 10.0f);
    this->parent_->write_register(this->get_heating_sp_write_reg_(), raw);
  }

  if (call.get_target_temperature_high().has_value()) {
    float temp_f = *call.get_target_temperature_high() * 9.0f / 5.0f + 32.0f;
    uint16_t raw = static_cast<uint16_t>(temp_f * 10.0f);
    this->parent_->write_register(this->get_cooling_sp_write_reg_(), raw);
  }

  if (call.get_fan_mode().has_value()) {
    auto fan_mode = *call.get_fan_mode();
    uint16_t wf_fan;
    switch (fan_mode) {
      case climate::CLIMATE_FAN_AUTO:
        wf_fan = FAN_AUTO;
        break;
      case climate::CLIMATE_FAN_ON:
        wf_fan = FAN_CONTINUOUS;
        break;
      default:
        wf_fan = FAN_AUTO;
        break;
    }
    this->parent_->write_register(this->get_fan_mode_write_reg_(), wf_fan);
  }

  if (call.has_custom_fan_mode()) {
    if (call.get_custom_fan_mode() == "Intermittent") {
      this->parent_->write_register(this->get_fan_mode_write_reg_(), FAN_INTERMITTENT);
    }
  }
}

// --- Register callbacks ---

void WaterFurnaceClimate::on_ambient_temp_(uint16_t value) {
  // Convert from °F * 10 to °C
  float temp_f = static_cast<int16_t>(value) / 10.0f;
  float temp_c = (temp_f - 32.0f) * 5.0f / 9.0f;
  this->current_temperature = temp_c;
  this->publish_state_if_changed_();
}

void WaterFurnaceClimate::on_heating_setpoint_(uint16_t value) {
  // Convert from °F * 10 to °C
  float temp_f = value / 10.0f;
  float temp_c = (temp_f - 32.0f) * 5.0f / 9.0f;
  this->target_temperature_low = temp_c;
  this->publish_state_if_changed_();
}

void WaterFurnaceClimate::on_cooling_setpoint_(uint16_t value) {
  // Convert from °F * 10 to °C
  float temp_f = value / 10.0f;
  float temp_c = (temp_f - 32.0f) * 5.0f / 9.0f;
  this->target_temperature_high = temp_c;
  this->publish_state_if_changed_();
}

void WaterFurnaceClimate::on_mode_config_(uint16_t value) {
  // Single zone: mode is in bits 8-10 of register 12006
  uint8_t wf_mode = (value >> 8) & 0x07;

  switch (wf_mode) {
    case MODE_OFF:
      this->mode = climate::CLIMATE_MODE_OFF;
      this->preset = climate::CLIMATE_PRESET_NONE;
      break;
    case MODE_AUTO:
      this->mode = climate::CLIMATE_MODE_HEAT_COOL;
      this->preset = climate::CLIMATE_PRESET_NONE;
      break;
    case MODE_COOL:
      this->mode = climate::CLIMATE_MODE_COOL;
      this->preset = climate::CLIMATE_PRESET_NONE;
      break;
    case MODE_HEAT:
      this->mode = climate::CLIMATE_MODE_HEAT;
      this->preset = climate::CLIMATE_PRESET_NONE;
      break;
    case MODE_EHEAT:
      this->mode = climate::CLIMATE_MODE_HEAT;
      this->preset = climate::CLIMATE_PRESET_BOOST;
      break;
    default:
      break;
  }
  this->publish_state_if_changed_();
}

void WaterFurnaceClimate::on_fan_config_(uint16_t value) {
  // Single zone: fan mode extracted from register 12005
  if (value & 0x80) {
    this->fan_mode = climate::CLIMATE_FAN_ON;
    this->clear_custom_fan_mode_();
    this->current_custom_fan_mode_.clear();
  } else if (value & 0x100) {
    this->fan_mode.reset();
    this->set_custom_fan_mode_("Intermittent");
    this->current_custom_fan_mode_ = "Intermittent";
  } else {
    this->fan_mode = climate::CLIMATE_FAN_AUTO;
    this->clear_custom_fan_mode_();
    this->current_custom_fan_mode_.clear();
  }
  this->publish_state_if_changed_();
}

void WaterFurnaceClimate::on_iz2_config1_(uint16_t value) {
  this->iz2_config1_ = value;

  // Extract fan mode
  uint8_t fan = iz2_extract_fan_mode(value);
  switch (fan) {
    case FAN_AUTO:
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
      this->clear_custom_fan_mode_();
      this->current_custom_fan_mode_.clear();
      break;
    case FAN_CONTINUOUS:
      this->fan_mode = climate::CLIMATE_FAN_ON;
      this->clear_custom_fan_mode_();
      this->current_custom_fan_mode_.clear();
      break;
    case FAN_INTERMITTENT:
      this->fan_mode.reset();
      this->set_custom_fan_mode_("Intermittent");
      this->current_custom_fan_mode_ = "Intermittent";
      break;
  }

  // Extract cooling setpoint
  uint8_t cool_sp = iz2_extract_cooling_setpoint(value);
  float temp_c = (static_cast<float>(cool_sp) - 32.0f) * 5.0f / 9.0f;
  this->target_temperature_high = temp_c;

  // If we have both config registers, extract heating setpoint
  if (this->iz2_config2_ != 0) {
    uint8_t heat_sp = iz2_extract_heating_setpoint(value, this->iz2_config2_);
    float heat_c = (static_cast<float>(heat_sp) - 32.0f) * 5.0f / 9.0f;
    this->target_temperature_low = heat_c;
  }

  this->publish_state_if_changed_();
}

void WaterFurnaceClimate::on_iz2_config2_(uint16_t value) {
  this->iz2_config2_ = value;

  // Extract mode
  uint8_t wf_mode = iz2_extract_mode(value);
  switch (wf_mode) {
    case MODE_OFF:
      this->mode = climate::CLIMATE_MODE_OFF;
      this->preset = climate::CLIMATE_PRESET_NONE;
      break;
    case MODE_AUTO:
      this->mode = climate::CLIMATE_MODE_HEAT_COOL;
      this->preset = climate::CLIMATE_PRESET_NONE;
      break;
    case MODE_COOL:
      this->mode = climate::CLIMATE_MODE_COOL;
      this->preset = climate::CLIMATE_PRESET_NONE;
      break;
    case MODE_HEAT:
      this->mode = climate::CLIMATE_MODE_HEAT;
      this->preset = climate::CLIMATE_PRESET_NONE;
      break;
    default:
      break;
  }

  // Extract heating setpoint (needs carry bit from config1)
  if (this->iz2_config1_ != 0) {
    uint8_t heat_sp = iz2_extract_heating_setpoint(this->iz2_config1_, value);
    float heat_c = (static_cast<float>(heat_sp) - 32.0f) * 5.0f / 9.0f;
    this->target_temperature_low = heat_c;
  }

  this->publish_state_if_changed_();
}

// --- Write register helpers ---

uint16_t WaterFurnaceClimate::get_mode_write_reg_() const {
  if (this->zone_ == 0)
    return REG_WRITE_MODE;
  return REG_IZ2_WRITE_BASE + (this->zone_ - 1) * 9;
}

uint16_t WaterFurnaceClimate::get_heating_sp_write_reg_() const {
  if (this->zone_ == 0)
    return REG_WRITE_HEATING_SP;
  return REG_IZ2_WRITE_BASE + 1 + (this->zone_ - 1) * 9;
}

uint16_t WaterFurnaceClimate::get_cooling_sp_write_reg_() const {
  if (this->zone_ == 0)
    return REG_WRITE_COOLING_SP;
  return REG_IZ2_WRITE_BASE + 2 + (this->zone_ - 1) * 9;
}

uint16_t WaterFurnaceClimate::get_fan_mode_write_reg_() const {
  if (this->zone_ == 0)
    return REG_WRITE_FAN_MODE;
  return REG_IZ2_WRITE_BASE + 3 + (this->zone_ - 1) * 9;
}

void WaterFurnaceClimate::publish_state_if_changed_() {
  if (this->has_published_) {
    bool changed = false;

    // Compare temperatures with epsilon
    auto temp_changed = [](float a, float b) -> bool {
      if (std::isnan(a) != std::isnan(b))
        return true;
      if (std::isnan(a))
        return false;
      return std::abs(a - b) >= 0.001f;
    };

    if (temp_changed(this->current_temperature, this->last_current_temp_))
      changed = true;
    else if (temp_changed(this->target_temperature_low, this->last_target_low_))
      changed = true;
    else if (temp_changed(this->target_temperature_high, this->last_target_high_))
      changed = true;
    else if (this->mode != this->last_mode_)
      changed = true;
    else if (this->fan_mode != this->last_fan_mode_)
      changed = true;
    else if (this->preset != this->last_preset_)
      changed = true;
    else if (this->current_custom_fan_mode_ != this->last_custom_fan_mode_)
      changed = true;

    if (!changed)
      return;
  }

  this->last_current_temp_ = this->current_temperature;
  this->last_target_low_ = this->target_temperature_low;
  this->last_target_high_ = this->target_temperature_high;
  this->last_mode_ = this->mode;
  this->last_fan_mode_ = this->fan_mode;
  this->last_preset_ = this->preset;
  this->last_custom_fan_mode_ = this->current_custom_fan_mode_;
  this->has_published_ = true;
  this->publish_state();
}

}  // namespace waterfurnace
}  // namespace esphome
