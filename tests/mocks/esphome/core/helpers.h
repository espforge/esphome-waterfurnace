// Minimal ESPHome helpers mock for unit testing
#pragma once
#include <string>
#include <cstdint>

namespace esphome {

// Mirrors esphome/core/helpers.h — reference-counted high-frequency loop
// request. Tests can observe it via the static is_high_frequency().
class HighFrequencyLoopRequester {
 public:
  // Test isolation: unlike the real class, release any held request on
  // destruction so a hub destroyed mid-transmit in one TEST_CASE cannot
  // pollute the global counter seen by later cases.
  ~HighFrequencyLoopRequester() { this->stop(); }

  void start() {
    if (this->started_)
      return;
    num_requests_++;
    this->started_ = true;
  }
  void stop() {
    if (!this->started_)
      return;
    num_requests_--;
    this->started_ = false;
  }
  static bool is_high_frequency() { return num_requests_ > 0; }

 protected:
  bool started_{false};
  static inline uint8_t num_requests_{0};
};

}  // namespace esphome
