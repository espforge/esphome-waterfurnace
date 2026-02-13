#include "waterfurnace_text_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace waterfurnace {

static const char *const TAG = "waterfurnace.text_sensor";

static std::string bitmask_to_string(uint16_t value, const BitLabel *bits, size_t count) {
  if (value == 0)
    return "None";
  std::string result;
  for (size_t i = 0; i < count; i++) {
    if (value & bits[i].mask) {
      if (!result.empty())
        result += ", ";
      result += bits[i].label;
    }
  }
  return result.empty() ? "None" : result;
}

void WaterFurnaceTextSensor::setup() {
  if (this->sensor_type_ == "fault") {
    this->parent_->register_listener(REG_LAST_FAULT, [this](uint16_t v) {
      this->on_fault_register_(v);
    });
  } else if (this->sensor_type_ == "model") {
    // Model is read once during hub setup; publish after detection completes
    this->parent_->register_setup_callback([this]() {
      this->publish_state_dedup_(this->parent_->model_number());
    });
  } else if (this->sensor_type_ == "serial") {
    this->parent_->register_setup_callback([this]() {
      this->publish_state_dedup_(this->parent_->serial_number());
    });
  } else if (this->sensor_type_ == "mode") {
    this->parent_->register_listener(REG_SYSTEM_OUTPUTS, [this](uint16_t v) {
      this->on_system_outputs_(v);
    });
    this->parent_->register_listener(REG_ACTIVE_DEHUMIDIFY, [this](uint16_t v) {
      this->on_active_dehumidify_(v);
    }, RegisterCapability::VS_DRIVE);
    this->parent_->register_listener(REG_COMPRESSOR_DELAY, [this](uint16_t v) {
      this->on_compressor_delay_(v);
    });
  } else if (this->sensor_type_ == "outputs_at_lockout") {
    this->parent_->register_listener(REG_OUTPUTS_AT_LOCKOUT, [this](uint16_t v) {
      this->publish_state_dedup_(bitmask_to_string(v, OUTPUT_BITS, OUTPUT_BITS_SIZE));
    });
  } else if (this->sensor_type_ == "inputs_at_lockout") {
    this->parent_->register_listener(REG_INPUTS_AT_LOCKOUT, [this](uint16_t v) {
      this->publish_state_dedup_(bitmask_to_string(v, INPUT_BITS, INPUT_BITS_SIZE));
    });
  }
}

void WaterFurnaceTextSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "WaterFurnace Text Sensor '%s':", this->get_name().c_str());
  ESP_LOGCONFIG(TAG, "  Type: %s", this->sensor_type_.c_str());
}

void WaterFurnaceTextSensor::on_fault_register_(uint16_t value) {
  bool locked_out = (value & 0x8000) != 0;
  uint8_t fault_code = value & 0x7FFF;

  if (fault_code == 0) {
    this->publish_state_dedup_("No Fault");
    return;
  }

  char buf[64];
  const char *desc = fault_code_to_string(fault_code);
  snprintf(buf, sizeof(buf), "E%d %s%s", fault_code, desc,
           locked_out ? " (LOCKOUT)" : "");
  this->publish_state_dedup_(buf);
}

void WaterFurnaceTextSensor::on_system_outputs_(uint16_t value) {
  this->system_outputs_ = value;
  this->has_outputs_ = true;
  this->compute_system_mode_();
}

void WaterFurnaceTextSensor::on_active_dehumidify_(uint16_t value) {
  this->active_dehumidify_ = value;
  if (this->has_outputs_)
    this->compute_system_mode_();
}

void WaterFurnaceTextSensor::on_compressor_delay_(uint16_t value) {
  this->compressor_delay_ = value;
  if (this->has_outputs_)
    this->compute_system_mode_();
}

void WaterFurnaceTextSensor::compute_system_mode_() {
  uint16_t outputs = this->system_outputs_;

  if (outputs & OUTPUT_LOCKOUT) {
    this->publish_state_dedup_("Lockout");
  } else if (this->active_dehumidify_ != 0) {
    this->publish_state_dedup_("Dehumidify");
  } else if ((outputs & OUTPUT_CC) || (outputs & OUTPUT_CC2)) {
    // Compressor running - check if cooling, heating with aux, or plain heating
    if (outputs & OUTPUT_RV) {
      this->publish_state_dedup_("Cooling");
    } else if (outputs & (OUTPUT_EH1 | OUTPUT_EH2)) {
      this->publish_state_dedup_("Heating with Aux");
    } else {
      this->publish_state_dedup_("Heating");
    }
  } else if (outputs & (OUTPUT_EH1 | OUTPUT_EH2)) {
    // EH without compressor = emergency heat
    this->publish_state_dedup_("Emergency Heat");
  } else if (outputs & OUTPUT_BLOWER) {
    this->publish_state_dedup_("Fan Only");
  } else if (this->compressor_delay_ != 0) {
    this->publish_state_dedup_("Waiting");
  } else {
    this->publish_state_dedup_("Standby");
  }
}

void WaterFurnaceTextSensor::publish_state_dedup_(const std::string &state) {
  if (state == this->last_published_state_)
    return;
  this->last_published_state_ = state;
  this->publish_state(state);
}

}  // namespace waterfurnace
}  // namespace esphome
