#include "waterfurnace_binary_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace waterfurnace {

static const char *const TAG = "waterfurnace.binary_sensor";

void WaterFurnaceBinarySensor::setup() {
  this->parent_->register_listener(this->register_address_, [this](uint16_t value) {
    this->publish_state((value & this->bitmask_) != 0);
  });
}

void WaterFurnaceBinarySensor::dump_config() {
  ESP_LOGCONFIG(TAG, "WaterFurnace Binary Sensor '%s':", this->get_name().c_str());
  ESP_LOGCONFIG(TAG, "  Register: %u, Bitmask: 0x%04X", this->register_address_, this->bitmask_);
}

}  // namespace waterfurnace
}  // namespace esphome
