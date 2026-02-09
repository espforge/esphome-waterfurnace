#pragma once

// Mock ESPHome types for unit testing waterfurnace components

#include <cmath>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

// Controllable millis for testing
inline uint32_t mock_millis = 0;

namespace esphome {

inline uint32_t millis() { return mock_millis; }
inline void delay(uint32_t) {}

template<typename T>
using optional = std::optional<T>;

// Logging macros - no-ops
#define ESP_LOGCONFIG(tag, fmt, ...)
#define ESP_LOGI(tag, fmt, ...)
#define ESP_LOGW(tag, fmt, ...)
#define ESP_LOGD(tag, fmt, ...)
#define ESP_LOGE(tag, fmt, ...)
#define ESP_LOGV(tag, fmt, ...)
#define YESNO(x) ((x) ? "YES" : "NO")

namespace setup_priority {
static constexpr float HARDWARE = 100.0f;
static constexpr float BUS = 90.0f;
static constexpr float DATA = 50.0f;
static constexpr float PROCESSOR = 10.0f;
static constexpr float LATE = -100.0f;
}  // namespace setup_priority

class GPIOPin {
 public:
  virtual void setup() {}
  virtual void digital_write(bool value) {}
  virtual bool digital_read() { return false; }
};

class EntityBase {
 public:
  const std::string &get_name() const { return name_; }
  void set_name(const std::string &name) { name_ = name; }
  uint32_t get_object_id_hash() const { return 0; }
  std::string name_;
};

class Component {
 public:
  virtual ~Component() = default;
  virtual void setup() {}
  virtual void loop() {}
  virtual void dump_config() {}
  virtual float get_setup_priority() const { return 0.0f; }
};

class PollingComponent : public Component {
 public:
  virtual void update() {}
  void set_update_interval(uint32_t interval) {}
};

namespace sensor {
class Sensor : public EntityBase {
 public:
  float state{NAN};
  void publish_state(float value) { state = value; }
};
}  // namespace sensor

namespace binary_sensor {
class BinarySensor : public EntityBase {
 public:
  bool state{false};
  bool has_state_{false};
  void publish_state(bool value) {
    // BinarySensor does dedup in real ESPHome, but for tests we always update
    state = value;
    has_state_ = true;
  }
};
}  // namespace binary_sensor

namespace text_sensor {
class TextSensor : public EntityBase {
 public:
  std::string state;
  void publish_state(const std::string &value) { state = value; }
};
}  // namespace text_sensor

namespace switch_ {
class Switch : public EntityBase {
 public:
  virtual ~Switch() = default;
  bool state{false};
  void publish_state(bool value) { state = value; }
  virtual void turn_on() { write_state(true); }
  virtual void turn_off() { write_state(false); }
 protected:
  virtual void write_state(bool state) { publish_state(state); }
};
}  // namespace switch_

namespace climate {

enum ClimateMode : uint8_t {
  CLIMATE_MODE_OFF = 0,
  CLIMATE_MODE_HEAT_COOL = 1,
  CLIMATE_MODE_COOL = 2,
  CLIMATE_MODE_HEAT = 3,
  CLIMATE_MODE_FAN_ONLY = 4,
  CLIMATE_MODE_DRY = 5,
  CLIMATE_MODE_AUTO = 6,
};

enum ClimateFanMode : uint8_t {
  CLIMATE_FAN_ON = 0,
  CLIMATE_FAN_OFF = 1,
  CLIMATE_FAN_AUTO = 2,
  CLIMATE_FAN_LOW = 3,
  CLIMATE_FAN_MEDIUM = 4,
  CLIMATE_FAN_HIGH = 5,
};

enum ClimatePreset : uint8_t {
  CLIMATE_PRESET_NONE = 0,
  CLIMATE_PRESET_HOME = 1,
  CLIMATE_PRESET_AWAY = 2,
  CLIMATE_PRESET_BOOST = 3,
  CLIMATE_PRESET_COMFORT = 4,
  CLIMATE_PRESET_ECO = 5,
  CLIMATE_PRESET_SLEEP = 6,
  CLIMATE_PRESET_ACTIVITY = 7,
};

class ClimateTraits {
 public:
  void set_supports_current_temperature(bool v) {}
  void set_supports_two_point_target_temperature(bool v) {}
  void set_visual_min_temperature(float v) {}
  void set_visual_max_temperature(float v) {}
  void set_visual_target_temperature_step(float v) {}
  void set_visual_current_temperature_step(float v) {}
  void set_supported_modes(std::vector<ClimateMode> modes) {}
  void set_supported_fan_modes(std::vector<ClimateFanMode> modes) {}
  void set_supported_custom_fan_modes(std::vector<std::string> modes) {}
  void set_supported_presets(std::vector<ClimatePreset> presets) {}
};

class ClimateCall {
 public:
  optional<ClimateMode> get_mode() const { return mode_; }
  optional<ClimateFanMode> get_fan_mode() const { return fan_mode_; }
  optional<ClimatePreset> get_preset() const { return preset_; }
  optional<float> get_target_temperature_low() const { return target_low_; }
  optional<float> get_target_temperature_high() const { return target_high_; }
  bool has_custom_fan_mode() const { return custom_fan_mode_.has_value(); }
  std::string get_custom_fan_mode() const { return custom_fan_mode_.value_or(""); }

  ClimateCall &set_mode(ClimateMode mode) { mode_ = mode; return *this; }
  ClimateCall &set_fan_mode(ClimateFanMode mode) { fan_mode_ = mode; return *this; }
  ClimateCall &set_preset(ClimatePreset preset) { preset_ = preset; return *this; }
  ClimateCall &set_target_temperature_low(float v) { target_low_ = v; return *this; }
  ClimateCall &set_target_temperature_high(float v) { target_high_ = v; return *this; }
  ClimateCall &set_custom_fan_mode(const std::string &mode) { custom_fan_mode_ = mode; return *this; }

 private:
  optional<ClimateMode> mode_;
  optional<ClimateFanMode> fan_mode_;
  optional<ClimatePreset> preset_;
  optional<float> target_low_;
  optional<float> target_high_;
  optional<std::string> custom_fan_mode_;
};

class Climate : public EntityBase {
 public:
  ClimateMode mode{CLIMATE_MODE_OFF};
  float current_temperature{NAN};
  float target_temperature_low{NAN};
  float target_temperature_high{NAN};
  optional<ClimateFanMode> fan_mode{};
  optional<ClimatePreset> preset{};

  int publish_count_{0};
  void publish_state() { publish_count_++; }
  virtual ClimateTraits traits() { return ClimateTraits(); }
  virtual void control(const ClimateCall &call) {}

 protected:
  void set_custom_fan_mode_(const std::string &mode) { custom_fan_mode_ = mode; }
  void clear_custom_fan_mode_() { custom_fan_mode_.clear(); }
  std::string custom_fan_mode_;
};

}  // namespace climate

namespace uart {
class UARTDevice {
 public:
  bool available() { return false; }
  bool read_byte(uint8_t *data) { return false; }
  void write_array(const uint8_t *data, size_t len) {}
  void flush() {}
};
}  // namespace uart

namespace api {
class CustomAPIDevice {};
}  // namespace api

}  // namespace esphome
