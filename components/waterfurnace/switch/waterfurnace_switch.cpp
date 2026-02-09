#include "waterfurnace_switch.h"
#include "esphome/core/log.h"

namespace esphome {
namespace waterfurnace {

static const char *const TAG = "waterfurnace.switch";

void WaterFurnaceSwitch::setup() {
  this->parent_->register_listener(this->register_address_, [this](uint16_t value) {
    this->publish_state(value != 0);
  });
}

void WaterFurnaceSwitch::dump_config() {
  ESP_LOGCONFIG(TAG, "WaterFurnace Switch '%s':", this->get_name().c_str());
  ESP_LOGCONFIG(TAG, "  Read Register: %u, Write Register: %u",
                this->register_address_, this->write_address_);
}

void WaterFurnaceSwitch::write_state(bool state) {
  this->parent_->write_register(this->write_address_, state ? 1 : 0);
  // Optimistically publish - will be confirmed on next poll
  this->publish_state(state);
}

}  // namespace waterfurnace
}  // namespace esphome
