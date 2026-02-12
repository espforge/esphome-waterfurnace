#include "waterfurnace.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#ifdef USE_API_CUSTOM_SERVICES
#include "esphome/components/api/custom_api_device.h"
#endif

namespace esphome {
namespace waterfurnace {

static const char *const TAG = "waterfurnace";

void WaterFurnace::setup() {
  if (this->flow_control_pin_ != nullptr) {
    this->flow_control_pin_->setup();
    this->flow_control_pin_->digital_write(false);  // RX mode
  }
  this->state_ = State::SETUP_READ_ID;
  this->setup_phase_ = 0;
  this->last_successful_response_ = millis();
  this->update_connected_(false);

#ifdef USE_API_CUSTOM_SERVICES
  register_service(&WaterFurnace::on_write_register_service_, "write_register",
                   {"address", "value"});
#endif

  ESP_LOGI(TAG, "WaterFurnace hub initializing...");
}

void WaterFurnace::update() {
  // PollingComponent::update() triggers a new poll cycle
  // The actual polling happens in loop() via the state machine
  if (this->state_ == State::IDLE) {
    this->current_poll_group_ = 0;
    this->poll_next_group_();
  }
}

void WaterFurnace::loop() {
  uint32_t now = millis();

  // Connectivity timeout check
  if (this->connected_ && (now - this->last_successful_response_) > this->connected_timeout_) {
    this->update_connected_(false);
  }

  switch (this->state_) {
    case State::SETUP_READ_ID: {
      if (this->setup_phase_ == 0) {
        ESP_LOGI(TAG, "Reading system identification...");
        this->read_system_id_();
        this->setup_phase_ = 1;
        this->state_ = State::WAITING_RESPONSE;
        return;
      }
      break;
    }

    case State::SETUP_DETECT_COMPONENTS: {
      if (this->setup_phase_ == 0) {
        ESP_LOGI(TAG, "Detecting installed components...");
        this->detect_components_();
        this->setup_phase_ = 1;
        this->state_ = State::WAITING_RESPONSE;
        return;
      }
      break;
    }

    case State::IDLE: {
      // Process any pending writes first
      if (!this->pending_writes_.empty()) {
        this->process_pending_writes_();
        return;
      }
      break;
    }

    case State::WAITING_RESPONSE: {
      // Try to read a complete frame
      std::vector<uint8_t> frame;
      if (this->read_frame_(frame)) {
        this->last_response_time_ = now;
        this->process_response_(frame);
        return;
      }

      // Check for timeout
      if (now - this->last_request_time_ > RESPONSE_TIMEOUT) {
        ESP_LOGW(TAG, "Response timeout (waited %ums)", RESPONSE_TIMEOUT);
        this->rx_buffer_.clear();

        // Staleness: erase expected addresses from cache on timeout
        for (uint16_t addr : this->expected_addresses_) {
          this->registers_.erase(addr);
        }

        this->error_backoff_until_ = now + ERROR_BACKOFF_TIME;
        this->state_ = State::ERROR_BACKOFF;
      }
      break;
    }

    case State::ERROR_BACKOFF: {
      if (now >= this->error_backoff_until_) {
        ESP_LOGI(TAG, "Error backoff complete, resuming");
        // If we were in setup, retry
        if (this->setup_phase_ != 0) {
          this->setup_phase_ = 0;
          // Determine which setup state to return to
          if (this->model_number_.empty()) {
            this->state_ = State::SETUP_READ_ID;
          } else if (!this->has_thermostat_ && !this->has_axb_) {
            this->state_ = State::SETUP_DETECT_COMPONENTS;
          } else {
            this->state_ = State::IDLE;
          }
        } else {
          this->state_ = State::IDLE;
        }
      }
      break;
    }
  }
}

void WaterFurnace::dump_config() {
  ESP_LOGCONFIG(TAG, "WaterFurnace Aurora:");
  ESP_LOGCONFIG(TAG, "  Model: %s", this->model_number_.c_str());
  ESP_LOGCONFIG(TAG, "  Serial: %s", this->serial_number_.c_str());
  ESP_LOGCONFIG(TAG, "  Program: %s", this->abc_program_.c_str());
  ESP_LOGCONFIG(TAG, "  Thermostat: %s (AWL: %s)",
                YESNO(this->has_thermostat_), YESNO(this->awl_thermostat_));
  ESP_LOGCONFIG(TAG, "  AXB: %s (AWL: %s)",
                YESNO(this->has_axb_), YESNO(this->awl_axb_));
  ESP_LOGCONFIG(TAG, "  IZ2: %s (zones: %d, AWL: %s)",
                YESNO(this->has_iz2_), this->iz2_zone_count_, YESNO(this->awl_iz2_));
  ESP_LOGCONFIG(TAG, "  VS Drive: %s", YESNO(this->has_vs_drive_));
  ESP_LOGCONFIG(TAG, "  Energy Monitoring: %s", YESNO(this->has_energy_monitoring_));
  if (this->flow_control_pin_ != nullptr) {
    LOG_PIN("  Flow Control Pin: ", this->flow_control_pin_);
  }
  ESP_LOGCONFIG(TAG, "  Connected timeout: %ums", this->connected_timeout_);
  ESP_LOGCONFIG(TAG, "  Poll groups: %d", this->poll_groups_.size());
  ESP_LOGCONFIG(TAG, "  Registered listeners: %d", this->listeners_.size());
}

void WaterFurnace::register_listener(uint16_t register_addr, std::function<void(uint16_t)> callback) {
  this->listeners_.push_back({register_addr, std::move(callback)});
}

void WaterFurnace::write_register(uint16_t addr, uint16_t value) {
  this->pending_writes_.push_back({addr, value});
  ESP_LOGD(TAG, "Queued write: register %u = %u", addr, value);
}

bool WaterFurnace::get_register(uint16_t addr, uint16_t &value) const {
  auto it = this->registers_.find(addr);
  if (it != this->registers_.end()) {
    value = it->second;
    return true;
  }
  return false;
}

void WaterFurnace::update_connected_(bool connected) {
  if (this->connected_ == connected)
    return;
  this->connected_ = connected;
  if (this->connected_sensor_ != nullptr) {
    this->connected_sensor_->publish_state(connected);
  }
  if (connected) {
    ESP_LOGI(TAG, "Connected");
  } else {
    ESP_LOGW(TAG, "Disconnected");
  }
}

void WaterFurnace::send_frame_(const std::vector<uint8_t> &frame) {
  // Assert DE pin for transmit
  if (this->flow_control_pin_ != nullptr) {
    this->flow_control_pin_->digital_write(true);
  }

  this->write_array(frame.data(), frame.size());
  this->flush();

  // De-assert DE pin for receive
  if (this->flow_control_pin_ != nullptr) {
    this->flow_control_pin_->digital_write(false);
  }

  this->last_request_time_ = millis();
  this->rx_buffer_.clear();

  ESP_LOGV(TAG, "TX frame (%d bytes): %s", frame.size(),
           format_hex_pretty(frame).c_str());
}

bool WaterFurnace::read_frame_(std::vector<uint8_t> &frame) {
  // Read all available bytes into buffer
  while (this->available()) {
    uint8_t byte;
    if (this->read_byte(&byte)) {
      this->rx_buffer_.push_back(byte);
    }
  }

  // Need at least: slave_addr + func_code + something
  if (this->rx_buffer_.size() < 3)
    return false;

  uint8_t func_code = this->rx_buffer_[1];

  // Error response: 5 bytes total
  if (is_error_response(func_code)) {
    if (this->rx_buffer_.size() >= 5) {
      frame.assign(this->rx_buffer_.begin(), this->rx_buffer_.begin() + 5);
      this->rx_buffer_.erase(this->rx_buffer_.begin(), this->rx_buffer_.begin() + 5);
      return validate_frame_crc(frame.data(), frame.size());
    }
    return false;
  }

  size_t expected_size;

  switch (func_code) {
    case FUNC_READ_RANGES:
    case FUNC_READ_REGISTERS: {
      // Variable length: slave + func + byte_count + data[byte_count] + CRC(2)
      if (this->rx_buffer_.size() < 3)
        return false;
      uint8_t byte_count = this->rx_buffer_[2];
      expected_size = 3 + byte_count + 2;  // header + data + CRC
      break;
    }
    case FUNC_WRITE_REGISTERS:
      // Minimal echo response: slave + func + CRC(2) = 4
      expected_size = 4;
      break;
    case FUNC_WRITE_SINGLE:
      // Echo: slave + func + addr(2) + value(2) + CRC(2) = 8
      expected_size = 8;
      break;
    default:
      // Unknown function, try variable length
      if (this->rx_buffer_.size() < 3)
        return false;
      expected_size = 3 + this->rx_buffer_[2] + 2;
      break;
  }

  if (this->rx_buffer_.size() < expected_size)
    return false;

  frame.assign(this->rx_buffer_.begin(), this->rx_buffer_.begin() + expected_size);
  this->rx_buffer_.erase(this->rx_buffer_.begin(), this->rx_buffer_.begin() + expected_size);

  ESP_LOGV(TAG, "RX frame (%d bytes): %s", frame.size(),
           format_hex_pretty(frame).c_str());

  if (!validate_frame_crc(frame.data(), frame.size())) {
    ESP_LOGW(TAG, "CRC validation failed");
    return false;
  }

  return true;
}

void WaterFurnace::process_response_(const std::vector<uint8_t> &frame) {
  if (frame.size() < MIN_FRAME_SIZE)
    return;

  uint8_t func_code = frame[1];

  // Handle error responses
  if (is_error_response(func_code)) {
    uint8_t error_code = (frame.size() > 2) ? frame[2] : 0;
    ESP_LOGW(TAG, "Error response: func=0x%02X error=0x%02X", func_code, error_code);

    // If we're in setup, go to error backoff
    if (this->state_ == State::WAITING_RESPONSE &&
        (this->model_number_.empty() || this->poll_groups_.empty())) {
      this->error_backoff_until_ = millis() + ERROR_BACKOFF_TIME;
      this->state_ = State::ERROR_BACKOFF;
    } else {
      this->state_ = State::IDLE;
    }
    return;
  }

  // Handle read responses (func 65/66)
  if (func_code == FUNC_READ_RANGES || func_code == FUNC_READ_REGISTERS) {
    if (frame.size() < 5)
      return;

    uint8_t byte_count = frame[2];
    auto values = parse_register_values(frame.data() + 3, byte_count);

    // Map values back to register addresses
    if (values.size() == this->expected_addresses_.size()) {
      // Successful read response - update connectivity
      this->last_successful_response_ = millis();
      this->update_connected_(true);

      for (size_t i = 0; i < values.size(); i++) {
        uint16_t addr = this->expected_addresses_[i];
        uint16_t val = values[i];
        this->registers_[addr] = val;
        this->dispatch_register_(addr, val);
      }
    } else {
      ESP_LOGW(TAG, "Response value count mismatch: got %d, expected %d",
               values.size(), this->expected_addresses_.size());
    }
  }

  // Handle write response (func 67 echo)
  if (func_code == FUNC_WRITE_REGISTERS) {
    ESP_LOGD(TAG, "Write acknowledged");
    this->last_successful_response_ = millis();
    this->update_connected_(true);
  }

  // Handle write single response (func 6 echo)
  if (func_code == FUNC_WRITE_SINGLE && frame.size() >= 6) {
    uint16_t addr = (frame[2] << 8) | frame[3];
    uint16_t val = (frame[4] << 8) | frame[5];
    ESP_LOGD(TAG, "Write single acknowledged: reg %u = %u", addr, val);
    this->registers_[addr] = val;
    this->dispatch_register_(addr, val);
    this->last_successful_response_ = millis();
    this->update_connected_(true);
  }

  // State transitions after successful response
  if (this->state_ == State::WAITING_RESPONSE) {
    if (this->poll_groups_.empty() && this->model_number_.empty()) {
      // Just received system ID response
      // Decode system ID from received registers
      this->abc_program_ = decode_string_(this->registers_, REG_ABC_PROGRAM, 4);
      this->model_number_ = decode_string_(this->registers_, REG_MODEL_NUMBER, 12);
      this->serial_number_ = decode_string_(this->registers_, REG_SERIAL_NUMBER, 5);

      // Trim trailing spaces/nulls
      while (!this->abc_program_.empty() && (this->abc_program_.back() == ' ' || this->abc_program_.back() == '\0'))
        this->abc_program_.pop_back();
      while (!this->model_number_.empty() && (this->model_number_.back() == ' ' || this->model_number_.back() == '\0'))
        this->model_number_.pop_back();
      while (!this->serial_number_.empty() && (this->serial_number_.back() == ' ' || this->serial_number_.back() == '\0'))
        this->serial_number_.pop_back();

      ESP_LOGI(TAG, "System ID: program=%s model=%s serial=%s",
               this->abc_program_.c_str(), this->model_number_.c_str(),
               this->serial_number_.c_str());

      // Detect VS drive from program name
      this->has_vs_drive_ = (this->abc_program_ == "ABCVSP" ||
                              this->abc_program_ == "ABCVSPR" ||
                              this->abc_program_ == "ABCSPLVS");

      this->state_ = State::SETUP_DETECT_COMPONENTS;
      this->setup_phase_ = 0;
    } else if (this->setup_phase_ != 0) {
      // Just finished component detection
      // Decode component status from registers
      auto check_component = [this](uint16_t status_reg) -> bool {
        auto it = this->registers_.find(status_reg);
        if (it == this->registers_.end())
          return false;
        return it->second != COMPONENT_REMOVED && it->second != COMPONENT_MISSING && it->second != 0;
      };

      auto get_version = [this](uint16_t version_reg) -> float {
        auto it = this->registers_.find(version_reg);
        if (it == this->registers_.end())
          return 0.0f;
        return it->second / 100.0f;
      };

      this->has_thermostat_ = check_component(REG_THERMOSTAT_STATUS);
      this->has_axb_ = check_component(REG_AXB_STATUS);
      this->has_iz2_ = check_component(REG_IZ2_STATUS);
      this->has_aoc_ = check_component(REG_AOC_STATUS);
      this->has_moc_ = check_component(REG_MOC_STATUS);

      // AWL versions
      float therm_ver = get_version(REG_THERMOSTAT_VERSION);
      float axb_ver = get_version(REG_AXB_VERSION);
      float iz2_ver = get_version(REG_IZ2_VERSION);

      this->awl_thermostat_ = this->has_thermostat_ && therm_ver >= 3.0f;
      this->awl_axb_ = this->has_axb_ && axb_ver >= 2.0f;
      this->awl_iz2_ = this->has_iz2_ && iz2_ver >= 2.0f;

      // Energy monitoring available if AXB present
      this->has_energy_monitoring_ = this->has_axb_;

      // IZ2 zone count
      if (this->awl_iz2_) {
        auto it = this->registers_.find(REG_IZ2_ZONE_COUNT);
        if (it != this->registers_.end() && it->second > 0 && it->second <= 6) {
          this->iz2_zone_count_ = it->second;
        }
      }

      ESP_LOGI(TAG, "Components detected: thermostat=%s(v%.1f) axb=%s(v%.1f) iz2=%s(v%.1f, %d zones) vs=%s",
               YESNO(this->has_thermostat_), therm_ver,
               YESNO(this->has_axb_), axb_ver,
               YESNO(this->has_iz2_), iz2_ver, this->iz2_zone_count_,
               YESNO(this->has_vs_drive_));

      // Build polling groups based on detected components
      this->build_poll_groups_();
      this->setup_complete_ = true;

      // Fire deferred setup callbacks (child entities waiting for detection results)
      for (auto &cb : this->setup_callbacks_) {
        cb();
      }
      this->setup_callbacks_.clear();

      this->setup_phase_ = 0;
      this->state_ = State::IDLE;

      ESP_LOGI(TAG, "Setup complete, %d poll groups configured", this->poll_groups_.size());
    } else {
      // Normal polling cycle - advance to next group or back to idle
      this->current_poll_group_++;
      if (this->current_poll_group_ < this->poll_groups_.size()) {
        // Small delay between groups
        this->poll_next_group_();
      } else {
        this->state_ = State::IDLE;
      }
    }
  }
}

void WaterFurnace::dispatch_register_(uint16_t addr, uint16_t value) {
  for (auto &listener : this->listeners_) {
    if (listener.address == addr) {
      listener.callback(value);
    }
  }
}

void WaterFurnace::read_system_id_() {
  auto ranges = get_system_id_ranges();

  // Build expected addresses list
  this->expected_addresses_.clear();
  for (const auto &range : ranges) {
    for (uint16_t i = 0; i < range.second; i++) {
      this->expected_addresses_.push_back(range.first + i);
    }
  }

  auto frame = build_read_ranges_request(ranges);
  this->send_frame_(frame);
  this->state_ = State::WAITING_RESPONSE;
}

void WaterFurnace::detect_components_() {
  auto ranges = get_component_detect_ranges();

  this->expected_addresses_.clear();
  for (const auto &range : ranges) {
    for (uint16_t i = 0; i < range.second; i++) {
      this->expected_addresses_.push_back(range.first + i);
    }
  }

  auto frame = build_read_ranges_request(ranges);
  this->send_frame_(frame);
  this->state_ = State::WAITING_RESPONSE;
}

void WaterFurnace::build_poll_groups_() {
  this->poll_groups_.clear();

  // Group 0: Core thermostat/status (always)
  {
    PollGroup group;
    group.ranges = get_thermostat_ranges();
    this->poll_groups_.push_back(std::move(group));
  }

  // Group 0b: Thermostat config (if AWL thermostat, single zone)
  // Separate group because registers 12005-12006 are across the 12100 breakpoint
  if (this->awl_thermostat_ && !this->has_iz2_) {
    PollGroup group;
    group.individual = get_thermostat_config_registers();
    this->poll_groups_.push_back(std::move(group));
  }

  // Group 1: AXB performance (if AXB present)
  if (this->has_axb_) {
    PollGroup group;
    group.ranges = get_axb_ranges();
    this->poll_groups_.push_back(std::move(group));
  }

  // Group 2: Power/energy (if energy monitoring)
  if (this->has_energy_monitoring_) {
    PollGroup group;
    group.ranges = get_power_ranges();
    this->poll_groups_.push_back(std::move(group));
  }

  // Group 3: VS Drive (if VS)
  if (this->has_vs_drive_) {
    PollGroup group;
    group.ranges = get_vs_drive_ranges();
    this->poll_groups_.push_back(std::move(group));
  }

  // Group 4: IZ2 zones (if IZ2)
  if (this->awl_iz2_ && this->iz2_zone_count_ > 0) {
    PollGroup group;
    group.ranges = get_iz2_ranges(this->iz2_zone_count_);
    this->poll_groups_.push_back(std::move(group));
  }
}

void WaterFurnace::poll_next_group_() {
  if (this->current_poll_group_ >= this->poll_groups_.size())
    return;

  const auto &group = this->poll_groups_[this->current_poll_group_];

  // Build expected addresses
  this->expected_addresses_.clear();
  for (const auto &range : group.ranges) {
    for (uint16_t i = 0; i < range.second; i++) {
      this->expected_addresses_.push_back(range.first + i);
    }
  }
  for (uint16_t addr : group.individual) {
    this->expected_addresses_.push_back(addr);
  }

  // Send the request
  if (!group.ranges.empty() && group.individual.empty()) {
    // All ranges - use func 65
    auto frame = build_read_ranges_request(group.ranges);
    this->send_frame_(frame);
  } else if (group.ranges.empty() && !group.individual.empty()) {
    // All individual - use func 66
    auto frame = build_read_registers_request(group.individual);
    this->send_frame_(frame);
  } else {
    // Mixed: convert ranges to individual addresses and use func 66
    std::vector<uint16_t> all_addrs;
    for (const auto &range : group.ranges) {
      for (uint16_t i = 0; i < range.second; i++) {
        all_addrs.push_back(range.first + i);
      }
    }
    for (uint16_t addr : group.individual) {
      all_addrs.push_back(addr);
    }

    if (all_addrs.size() <= MAX_REGISTERS_PER_REQUEST) {
      auto frame = build_read_registers_request(all_addrs);
      this->send_frame_(frame);
    } else {
      // Split if too many - just send ranges portion via func 65
      auto frame = build_read_ranges_request(group.ranges);
      this->send_frame_(frame);
      // Individual registers will need to be a separate poll group
      // This shouldn't happen with our current group sizes
      ESP_LOGW(TAG, "Poll group too large, individual registers skipped");
    }
  }

  this->state_ = State::WAITING_RESPONSE;
}

void WaterFurnace::process_pending_writes_() {
  if (this->pending_writes_.empty())
    return;

  // Send all pending writes in one func 67 request
  auto frame = build_write_registers_request(this->pending_writes_);
  ESP_LOGD(TAG, "Sending %d register writes", this->pending_writes_.size());

  // Build expected addresses (for write, we don't expect data back, just echo)
  this->expected_addresses_.clear();

  this->pending_writes_.clear();
  this->send_frame_(frame);
  this->state_ = State::WAITING_RESPONSE;
}

std::string WaterFurnace::decode_string_(const std::map<uint16_t, uint16_t> &regs,
                                          uint16_t start, uint8_t num_regs) {
  std::string result;
  for (uint8_t i = 0; i < num_regs; i++) {
    auto it = regs.find(start + i);
    if (it == regs.end())
      break;
    uint16_t val = it->second;
    char hi = (val >> 8) & 0xFF;
    char lo = val & 0xFF;
    if (hi != 0)
      result += hi;
    if (lo != 0)
      result += lo;
  }
  return result;
}

// --- HA API Service ---

#ifdef USE_API_CUSTOM_SERVICES

void WaterFurnace::on_write_register_service_(int32_t address, int32_t value) {
  if (address < 0 || address > 65535 || value < 0 || value > 65535) {
    ESP_LOGW(TAG, "API write_register: invalid args address=%d value=%d", (int) address, (int) value);
    return;
  }
  ESP_LOGI(TAG, "API write_register: address=%d value=%d", (int) address, (int) value);
  this->write_register(static_cast<uint16_t>(address), static_cast<uint16_t>(value));
}

#endif  // USE_API_CUSTOM_SERVICES

}  // namespace waterfurnace
}  // namespace esphome
