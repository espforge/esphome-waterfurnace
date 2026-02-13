#include "waterfurnace_climate.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <cmath>

namespace esphome {
namespace waterfurnace {

static const char *const TAG = "waterfurnace.climate";

void WaterFurnaceClimate::setup() {
  // Defer listener registration until hub has detected IZ2 status
  if (this->parent_->is_setup_complete()) {
    this->register_listeners_();
  } else {
    this->parent_->register_setup_callback([this]() { this->register_listeners_(); });
  }
}

void WaterFurnaceClimate::register_listeners_() {
  if (this->zone_ == 1 && !this->parent_->has_iz2()) {
    // Zone 1 without IZ2 - use thermostat registers
    // Use register 502 for ambient temp (register 747 may read 0 when mode is OFF)
    this->parent_->register_listener(REG_TSTAT_AMBIENT, [this](uint16_t v) { this->on_ambient_temp_(v); }, RegisterCapability::AWL_THERMOSTAT);
    this->parent_->register_listener(REG_HEATING_SETPOINT, [this](uint16_t v) { this->on_heating_setpoint_(v); }, RegisterCapability::AWL_THERMOSTAT);
    this->parent_->register_listener(REG_COOLING_SETPOINT, [this](uint16_t v) { this->on_cooling_setpoint_(v); }, RegisterCapability::AWL_THERMOSTAT);
    this->parent_->register_listener(REG_MODE_CONFIG, [this](uint16_t v) { this->on_mode_config_(v); }, RegisterCapability::AWL_THERMOSTAT);
    this->parent_->register_listener(REG_FAN_CONFIG, [this](uint16_t v) { this->on_fan_config_(v); }, RegisterCapability::AWL_THERMOSTAT);
  } else {
    // IZ2 zone mode (zone 1 with IZ2, or zones 2-6)
    uint16_t base = REG_IZ2_ZONE_BASE + (this->zone_ - 1) * 3;
    this->parent_->register_listener(base, [this](uint16_t v) { this->on_ambient_temp_(v); }, RegisterCapability::IZ2);
    this->parent_->register_listener(base + 1, [this](uint16_t v) { this->on_iz2_config1_(v); }, RegisterCapability::IZ2);
    this->parent_->register_listener(base + 2, [this](uint16_t v) { this->on_iz2_config2_(v); }, RegisterCapability::IZ2);
  }
}

void WaterFurnaceClimate::dump_config() {
  ESP_LOGCONFIG(TAG, "WaterFurnace Climate:");
  ESP_LOGCONFIG(TAG, "  Zone: %d", this->zone_);
}

climate::ClimateTraits WaterFurnaceClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE |
                           climate::CLIMATE_SUPPORTS_TWO_POINT_TARGET_TEMPERATURE);

  // Visual settings in °C (HA converts to display unit)
  traits.set_visual_min_temperature((45.0f - 32.0f) * 5.0f / 9.0f);   // 45°F
  traits.set_visual_max_temperature((92.0f - 32.0f) * 5.0f / 9.0f);   // 92°F
  traits.set_visual_target_temperature_step(1.0f);
  traits.set_visual_current_temperature_step(1.0f);

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

  // E-Heat custom preset only available on zone 1 (main thermostat)
  if (this->zone_ == 1) {
    traits.set_supported_custom_presets({"E-Heat"});
  }

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
    // Optimistically update the state immediately
    this->mode = mode;
    this->clear_custom_preset_();
    this->last_mode_write_ = millis();
    this->parent_->write_register(this->get_mode_write_reg_(), wf_mode);
    this->publish_state_if_changed_();
  }

  if (call.has_custom_preset()) {
    auto custom_preset = call.get_custom_preset();
    if (custom_preset == "E-Heat") {
      // Optimistically update the state immediately
      this->mode = climate::CLIMATE_MODE_HEAT;
      this->set_custom_preset_("E-Heat");
      this->last_mode_write_ = millis();
      this->parent_->write_register(this->get_mode_write_reg_(), MODE_EHEAT);
      this->publish_state_if_changed_();
    }
  }

  if (call.get_target_temperature_low().has_value()) {
    float temp_c = *call.get_target_temperature_low();
    // Round to whole °F, then back to °C for optimistic state
    float temp_f = std::round(temp_c * 9.0f / 5.0f + 32.0f);
    temp_c = (temp_f - 32.0f) * 5.0f / 9.0f;
    this->target_temperature_low = temp_c;
    uint16_t raw = this->temp_f_to_raw_(temp_f);
    this->last_heating_sp_write_ = millis();
    this->parent_->write_register(this->get_heating_sp_write_reg_(), raw);
    this->publish_state_if_changed_();
  }

  if (call.get_target_temperature_high().has_value()) {
    float temp_c = *call.get_target_temperature_high();
    // Round to whole °F, then back to °C for optimistic state
    float temp_f = std::round(temp_c * 9.0f / 5.0f + 32.0f);
    temp_c = (temp_f - 32.0f) * 5.0f / 9.0f;
    this->target_temperature_high = temp_c;
    uint16_t raw = this->temp_f_to_raw_(temp_f);
    this->last_cooling_sp_write_ = millis();
    this->parent_->write_register(this->get_cooling_sp_write_reg_(), raw);
    this->publish_state_if_changed_();
  }

  // Single-point target (HEAT/COOL modes use one slider)
  if (call.get_target_temperature().has_value()) {
    float temp_c = *call.get_target_temperature();
    // Round to whole °F, then back to °C for optimistic state
    float temp_f = std::round(temp_c * 9.0f / 5.0f + 32.0f);
    temp_c = (temp_f - 32.0f) * 5.0f / 9.0f;
    this->target_temperature = temp_c;
    uint16_t raw = this->temp_f_to_raw_(temp_f);
    if (this->mode == climate::CLIMATE_MODE_HEAT) {
      this->last_heating_sp_write_ = millis();
      this->parent_->write_register(this->get_heating_sp_write_reg_(), raw);
    } else if (this->mode == climate::CLIMATE_MODE_COOL) {
      this->last_cooling_sp_write_ = millis();
      this->parent_->write_register(this->get_cooling_sp_write_reg_(), raw);
    }
    this->publish_state_if_changed_();
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
    // Optimistically update the state immediately
    this->fan_mode = fan_mode;
    this->clear_custom_fan_mode_();
    this->last_fan_write_ = millis();
    this->parent_->write_register(this->get_fan_mode_write_reg_(), wf_fan);
    this->publish_state_if_changed_();
  }

  if (call.has_custom_fan_mode()) {
    auto custom_fan = call.get_custom_fan_mode();
    if (custom_fan == "Intermittent") {
      // Optimistically update the state immediately
      this->set_custom_fan_mode_("Intermittent");
      this->last_fan_write_ = millis();
      this->parent_->write_register(this->get_fan_mode_write_reg_(), FAN_INTERMITTENT);
      this->publish_state_if_changed_();
    }
  }
}

// --- Register callbacks ---

void WaterFurnaceClimate::on_ambient_temp_(uint16_t value) {
  // Convert from °F * 10 to °C
  float temp_f = static_cast<int16_t>(value) / 10.0f;
  float temp_c = (temp_f - 32.0f) * 5.0f / 9.0f;
  // Round to nearest 0.5556°C (1°F) for clean conversion back to Fahrenheit
  temp_c = std::round(temp_c / 0.5556f) * 0.5556f;
  ESP_LOGD(TAG, "Zone %d ambient temp: raw=%u (0x%04X) -> %.1f°F -> %.2f°C",
           this->zone_, value, value, temp_f, temp_c);
  this->current_temperature = temp_c;
  this->publish_state_if_changed_();
}

void WaterFurnaceClimate::on_heating_setpoint_(uint16_t value) {
  if (this->in_cooldown_(this->last_heating_sp_write_))
    return;
  // Convert from °F * 10 to °C
  float temp_f = value / 10.0f;
  float temp_c = (temp_f - 32.0f) * 5.0f / 9.0f;
  // Round to nearest 0.5556°C (1°F) for clean conversion back to Fahrenheit
  temp_c = std::round(temp_c / 0.5556f) * 0.5556f;
  ESP_LOGD(TAG, "Zone %d heating setpoint: raw=%u (0x%04X) -> %.1f°F -> %.2f°C",
           this->zone_, value, value, temp_f, temp_c);
  this->target_temperature_low = temp_c;
  this->publish_state_if_changed_();
}

void WaterFurnaceClimate::on_cooling_setpoint_(uint16_t value) {
  if (this->in_cooldown_(this->last_cooling_sp_write_))
    return;
  // Convert from °F * 10 to °C
  float temp_f = value / 10.0f;
  float temp_c = (temp_f - 32.0f) * 5.0f / 9.0f;
  // Round to nearest 0.5556°C (1°F) for clean conversion back to Fahrenheit
  temp_c = std::round(temp_c / 0.5556f) * 0.5556f;
  ESP_LOGD(TAG, "Zone %d cooling setpoint: raw=%u (0x%04X) -> %.1f°F -> %.2f°C",
           this->zone_, value, value, temp_f, temp_c);
  this->target_temperature_high = temp_c;
  this->publish_state_if_changed_();
}

void WaterFurnaceClimate::on_mode_config_(uint16_t value) {
  if (this->in_cooldown_(this->last_mode_write_))
    return;
  // Single zone: mode is in bits 8-10 of register 12006
  uint8_t wf_mode = (value >> 8) & 0x07;
  ESP_LOGD(TAG, "Zone %d mode config: raw=%u (0x%04X) -> wf_mode=%u",
           this->zone_, value, value, wf_mode);

  switch (wf_mode) {
    case MODE_OFF:
      this->mode = climate::CLIMATE_MODE_OFF;
      this->clear_custom_preset_();
      break;
    case MODE_AUTO:
      this->mode = climate::CLIMATE_MODE_HEAT_COOL;
      this->clear_custom_preset_();
      break;
    case MODE_COOL:
      this->mode = climate::CLIMATE_MODE_COOL;
      this->clear_custom_preset_();
      break;
    case MODE_HEAT:
      this->mode = climate::CLIMATE_MODE_HEAT;
      this->clear_custom_preset_();
      break;
    case MODE_EHEAT:
      this->mode = climate::CLIMATE_MODE_HEAT;
      this->set_custom_preset_("E-Heat");
      break;
    default:
      break;
  }
  this->publish_state_if_changed_();
}

void WaterFurnaceClimate::on_fan_config_(uint16_t value) {
  if (this->in_cooldown_(this->last_fan_write_))
    return;
  ESP_LOGD(TAG, "Zone %d fan config: raw=%u (0x%04X)", this->zone_, value, value);
  // Single zone: fan mode extracted from register 12005
  if (value & 0x80) {
    this->fan_mode = climate::CLIMATE_FAN_ON;
    this->clear_custom_fan_mode_();
  } else if (value & 0x100) {
    this->set_custom_fan_mode_("Intermittent");
  } else {
    this->fan_mode = climate::CLIMATE_FAN_AUTO;
    this->clear_custom_fan_mode_();
  }
  this->publish_state_if_changed_();
}

void WaterFurnaceClimate::on_iz2_config1_(uint16_t value) {
  this->iz2_config1_ = value;
  ESP_LOGD(TAG, "Zone %d IZ2 config1: raw=%u (0x%04X)", this->zone_, value, value);

  // Extract fan mode (skip if recently written)
  if (!this->in_cooldown_(this->last_fan_write_)) {
    uint8_t fan = iz2_extract_fan_mode(value);
    switch (fan) {
      case FAN_AUTO:
        this->fan_mode = climate::CLIMATE_FAN_AUTO;
        this->clear_custom_fan_mode_();
        break;
      case FAN_CONTINUOUS:
        this->fan_mode = climate::CLIMATE_FAN_ON;
        this->clear_custom_fan_mode_();
        break;
      case FAN_INTERMITTENT:
        this->set_custom_fan_mode_("Intermittent");
        break;
    }
  }

  // Extract cooling setpoint (skip if recently written)
  if (!this->in_cooldown_(this->last_cooling_sp_write_)) {
    uint8_t cool_sp = iz2_extract_cooling_setpoint(value);
    float temp_c = (static_cast<float>(cool_sp) - 32.0f) * 5.0f / 9.0f;
    this->target_temperature_high = temp_c;
  }

  // If we have both config registers, extract heating setpoint (skip if recently written)
  if (!this->in_cooldown_(this->last_heating_sp_write_) && this->iz2_config2_ != 0) {
    uint8_t heat_sp = iz2_extract_heating_setpoint(value, this->iz2_config2_);
    float heat_c = (static_cast<float>(heat_sp) - 32.0f) * 5.0f / 9.0f;
    this->target_temperature_low = heat_c;
  }

  this->publish_state_if_changed_();
}

void WaterFurnaceClimate::on_iz2_config2_(uint16_t value) {
  this->iz2_config2_ = value;
  ESP_LOGD(TAG, "Zone %d IZ2 config2: raw=%u (0x%04X) -> mode=%u",
           this->zone_, value, value, iz2_extract_mode(value));

  // Extract mode (skip if recently written)
  if (!this->in_cooldown_(this->last_mode_write_)) {
    uint8_t wf_mode = iz2_extract_mode(value);
    switch (wf_mode) {
      case MODE_OFF:
        this->mode = climate::CLIMATE_MODE_OFF;
        this->clear_custom_preset_();
        break;
      case MODE_AUTO:
        this->mode = climate::CLIMATE_MODE_HEAT_COOL;
        this->clear_custom_preset_();
        break;
      case MODE_COOL:
        this->mode = climate::CLIMATE_MODE_COOL;
        this->clear_custom_preset_();
        break;
      case MODE_HEAT:
        this->mode = climate::CLIMATE_MODE_HEAT;
        this->clear_custom_preset_();
        break;
      case MODE_EHEAT:
        this->mode = climate::CLIMATE_MODE_HEAT;
        if (this->zone_ == 1) {
          this->set_custom_preset_("E-Heat");
        }
        break;
      default:
        break;
    }
  }

  // Extract heating setpoint (skip if recently written)
  if (!this->in_cooldown_(this->last_heating_sp_write_) && this->iz2_config1_ != 0) {
    uint8_t heat_sp = iz2_extract_heating_setpoint(this->iz2_config1_, value);
    float heat_c = (static_cast<float>(heat_sp) - 32.0f) * 5.0f / 9.0f;
    this->target_temperature_low = heat_c;
  }

  this->publish_state_if_changed_();
}

// --- Write cooldown ---

bool WaterFurnaceClimate::in_cooldown_(uint32_t last_write) const {
  if (last_write == 0)
    return false;
  return (millis() - last_write) < WRITE_COOLDOWN_MS;
}

// --- Write register helpers ---

uint16_t WaterFurnaceClimate::temp_f_to_raw_(float temp_f) const {
  // Write registers expect tenths of °F: 68°F -> 680
  return static_cast<uint16_t>(std::round(temp_f)) * 10;
}

uint16_t WaterFurnaceClimate::get_mode_write_reg_() const {
  if (this->zone_ == 1 && !this->parent_->has_iz2())
    return REG_WRITE_MODE;
  return REG_IZ2_WRITE_BASE + (this->zone_ - 1) * 9;
}

uint16_t WaterFurnaceClimate::get_heating_sp_write_reg_() const {
  if (this->zone_ == 1 && !this->parent_->has_iz2())
    return REG_WRITE_HEATING_SP;
  return REG_IZ2_WRITE_BASE + 1 + (this->zone_ - 1) * 9;
}

uint16_t WaterFurnaceClimate::get_cooling_sp_write_reg_() const {
  if (this->zone_ == 1 && !this->parent_->has_iz2())
    return REG_WRITE_COOLING_SP;
  return REG_IZ2_WRITE_BASE + 2 + (this->zone_ - 1) * 9;
}

uint16_t WaterFurnaceClimate::get_fan_mode_write_reg_() const {
  if (this->zone_ == 1 && !this->parent_->has_iz2())
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
    else if (this->has_custom_fan_mode() != this->last_has_custom_fan_mode_)
      changed = true;
    else if (this->has_custom_preset() != this->last_has_custom_preset_)
      changed = true;

    if (!changed)
      return;
  }

  this->last_current_temp_ = this->current_temperature;
  this->last_target_low_ = this->target_temperature_low;
  this->last_target_high_ = this->target_temperature_high;
  this->last_mode_ = this->mode;
  this->last_fan_mode_ = this->fan_mode;
  this->last_has_custom_fan_mode_ = this->has_custom_fan_mode();
  this->last_has_custom_preset_ = this->has_custom_preset();
  this->has_published_ = true;
  this->publish_state();
}

}  // namespace waterfurnace
}  // namespace esphome
