#include "waterfurnace_text_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace waterfurnace {

static const char *const TAG = "waterfurnace.text_sensor";

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
  if (value & OUTPUT_LOCKOUT) {
    this->publish_state_dedup_("Lockout");
  } else if (value & OUTPUT_EH1) {
    this->publish_state_dedup_("Emergency Heat");
  } else if ((value & OUTPUT_CC) && (value & OUTPUT_RV)) {
    this->publish_state_dedup_("Cooling");
  } else if (value & OUTPUT_CC) {
    this->publish_state_dedup_("Heating");
  } else if (value & OUTPUT_BLOWER) {
    this->publish_state_dedup_("Fan Only");
  } else {
    this->publish_state_dedup_("Idle");
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
