#pragma once

// Stub implementations of WaterFurnace hub methods for unit testing.
// Child components call parent_->register_listener() and parent_->write_register().
// Tests use dispatch_register() to simulate register updates from the ModBus bus.

#include "../../components/waterfurnace/waterfurnace.h"

namespace esphome {
namespace waterfurnace {

// Track writes for test assertions
inline std::vector<std::pair<uint16_t, uint16_t>> written_registers;
inline void clear_written_registers() { written_registers.clear(); }

// --- Hub method stubs ---

void WaterFurnace::setup() {}
void WaterFurnace::update() {}
void WaterFurnace::loop() {}
void WaterFurnace::dump_config() {}

void WaterFurnace::register_listener(uint16_t register_addr,
                                      std::function<void(uint16_t)> callback) {
  listeners_.push_back({register_addr, std::move(callback)});
}

void WaterFurnace::write_register(uint16_t addr, uint16_t value) {
  written_registers.push_back({addr, value});
}

void WaterFurnace::dispatch_register_(uint16_t addr, uint16_t value) {
  registers_[addr] = value;
  for (auto &listener : listeners_) {
    if (listener.address == addr) {
      listener.callback(value);
    }
  }
}

bool WaterFurnace::get_register(uint16_t addr, uint16_t &value) const {
  auto it = registers_.find(addr);
  if (it != registers_.end()) {
    value = it->second;
    return true;
  }
  return false;
}

// Stubs for protocol methods (not used by child component tests)
void WaterFurnace::send_frame_(const std::vector<uint8_t> &) {}
bool WaterFurnace::read_frame_(std::vector<uint8_t> &) { return false; }
void WaterFurnace::process_response_(const std::vector<uint8_t> &) {}
void WaterFurnace::poll_next_group_() {}
void WaterFurnace::process_pending_writes_() {}
void WaterFurnace::read_system_id_() {}
void WaterFurnace::detect_components_() {}
void WaterFurnace::build_poll_groups_() {}
void WaterFurnace::update_connected_(bool) {}

std::string WaterFurnace::decode_string_(const std::map<uint16_t, uint16_t> &,
                                          uint16_t, uint8_t) {
  return "";
}

// Testable subclass to expose protected dispatch_register_
class TestableHub : public WaterFurnace {
 public:
  using WaterFurnace::dispatch_register_;
};

}  // namespace waterfurnace
}  // namespace esphome
