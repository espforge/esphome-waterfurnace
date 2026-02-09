#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/uart/uart.h"
#include "protocol.h"
#include "registers.h"

#ifdef USE_API_CUSTOM_SERVICES
#include "esphome/components/api/custom_api_device.h"
#endif

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace esphome {
namespace waterfurnace {

struct RegisterListener {
  uint16_t address;
  std::function<void(uint16_t)> callback;
};

class WaterFurnace : public PollingComponent, public uart::UARTDevice
#ifdef USE_API_CUSTOM_SERVICES
    , public api::CustomAPIDevice
#endif
{
 public:
  void setup() override;
  void update() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Listener registration (called by child entities during their setup)
  void register_listener(uint16_t register_addr, std::function<void(uint16_t)> callback);

  // Write interface (called by climate/switch entities)
  void write_register(uint16_t addr, uint16_t value);

  // Configuration
  void set_flow_control_pin(GPIOPin *pin) { flow_control_pin_ = pin; }
  void set_connected_sensor(binary_sensor::BinarySensor *sensor) { connected_sensor_ = sensor; }
  void set_connected_timeout(uint32_t timeout) { connected_timeout_ = timeout; }

  // Accessors for detected capabilities
  bool has_thermostat() const { return has_thermostat_; }
  bool has_iz2() const { return has_iz2_; }
  bool has_axb() const { return has_axb_; }
  bool has_vs_drive() const { return has_vs_drive_; }
  bool has_energy_monitoring() const { return has_energy_monitoring_; }
  uint8_t iz2_zone_count() const { return iz2_zone_count_; }

  // System info
  const std::string &model_number() const { return model_number_; }
  const std::string &serial_number() const { return serial_number_; }
  const std::string &abc_program() const { return abc_program_; }

  // Register cache access (for entities that need multi-register values)
  bool get_register(uint16_t addr, uint16_t &value) const;

 protected:
  // Protocol communication
  void send_frame_(const std::vector<uint8_t> &frame);
  bool read_frame_(std::vector<uint8_t> &frame);
  void process_response_(const std::vector<uint8_t> &frame);

  // Polling
  void poll_next_group_();
  void process_pending_writes_();

  // Setup phases
  void read_system_id_();
  void detect_components_();
  void build_poll_groups_();

  // Dispatch register values to listeners
  void dispatch_register_(uint16_t addr, uint16_t value);

  // Decode string from consecutive registers
  static std::string decode_string_(const std::map<uint16_t, uint16_t> &regs,
                                     uint16_t start, uint8_t num_regs);

  // Connectivity
  void update_connected_(bool connected);

#ifdef USE_API_CUSTOM_SERVICES
  // HA API service for modbus register write
  void on_write_register_service_(int32_t address, int32_t value);
#endif

  // State machine
  enum class State : uint8_t {
    SETUP_READ_ID,
    SETUP_DETECT_COMPONENTS,
    IDLE,
    WAITING_RESPONSE,
    ERROR_BACKOFF,
  };
  State state_{State::SETUP_READ_ID};
  uint8_t setup_phase_{0};

  // Polling groups
  struct PollGroup {
    std::vector<std::pair<uint16_t, uint16_t>> ranges;   // For func 65
    std::vector<uint16_t> individual;                      // For func 66
  };
  std::vector<PollGroup> poll_groups_;
  uint8_t current_poll_group_{0};

  // Track which addresses we expect in the current response
  std::vector<uint16_t> expected_addresses_;

  // System detection results
  bool has_thermostat_{false};
  bool has_iz2_{false};
  bool has_axb_{false};
  bool has_vs_drive_{false};
  bool has_energy_monitoring_{false};
  bool has_aoc_{false};
  bool has_moc_{false};
  uint8_t iz2_zone_count_{0};
  bool awl_thermostat_{false};
  bool awl_iz2_{false};
  bool awl_axb_{false};

  // System info
  std::string model_number_;
  std::string serial_number_;
  std::string abc_program_;

  // Register cache
  std::map<uint16_t, uint16_t> registers_;

  // Listeners
  std::vector<RegisterListener> listeners_;

  // Write queue
  std::vector<std::pair<uint16_t, uint16_t>> pending_writes_;

  // Hardware
  GPIOPin *flow_control_pin_{nullptr};

  // Connectivity monitoring
  binary_sensor::BinarySensor *connected_sensor_{nullptr};
  uint32_t connected_timeout_{30000};
  uint32_t last_successful_response_{0};
  bool connected_{false};

  // Timing
  uint32_t last_request_time_{0};
  uint32_t last_response_time_{0};
  uint32_t error_backoff_until_{0};

  // UART receive buffer
  std::vector<uint8_t> rx_buffer_;

  // Response timeout (ms)
  static constexpr uint32_t RESPONSE_TIMEOUT = 2000;
  // Error backoff time (ms)
  static constexpr uint32_t ERROR_BACKOFF_TIME = 5000;
  // Inter-frame delay for ModBus RTU at 19200 baud (1.75ms minimum, use 5ms for safety)
  static constexpr uint32_t INTER_FRAME_DELAY = 5;
};

}  // namespace waterfurnace
}  // namespace esphome
