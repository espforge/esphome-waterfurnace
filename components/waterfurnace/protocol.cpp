#include "protocol.h"

namespace esphome {
namespace waterfurnace {

uint16_t crc16(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

static void append_crc(std::vector<uint8_t> &frame) {
  uint16_t crc = crc16(frame.data(), frame.size());
  frame.push_back(crc & 0xFF);         // CRC low byte first (ModBus convention)
  frame.push_back((crc >> 8) & 0xFF);  // CRC high byte second
}

std::vector<uint8_t> build_read_ranges_request(
    const std::vector<std::pair<uint16_t, uint16_t>> &ranges) {
  std::vector<uint8_t> frame;
  frame.push_back(SLAVE_ADDRESS);
  frame.push_back(FUNC_READ_RANGES);

  for (const auto &range : ranges) {
    // Big-endian: address high, address low, quantity high, quantity low
    frame.push_back((range.first >> 8) & 0xFF);
    frame.push_back(range.first & 0xFF);
    frame.push_back((range.second >> 8) & 0xFF);
    frame.push_back(range.second & 0xFF);
  }

  append_crc(frame);
  return frame;
}

std::vector<uint8_t> build_read_registers_request(
    const std::vector<uint16_t> &addresses) {
  std::vector<uint8_t> frame;
  frame.push_back(SLAVE_ADDRESS);
  frame.push_back(FUNC_READ_REGISTERS);

  for (uint16_t addr : addresses) {
    frame.push_back((addr >> 8) & 0xFF);
    frame.push_back(addr & 0xFF);
  }

  append_crc(frame);
  return frame;
}

std::vector<uint8_t> build_write_registers_request(
    const std::vector<std::pair<uint16_t, uint16_t>> &writes) {
  std::vector<uint8_t> frame;
  frame.push_back(SLAVE_ADDRESS);
  frame.push_back(FUNC_WRITE_REGISTERS);

  for (const auto &w : writes) {
    // Address big-endian
    frame.push_back((w.first >> 8) & 0xFF);
    frame.push_back(w.first & 0xFF);
    // Value big-endian
    frame.push_back((w.second >> 8) & 0xFF);
    frame.push_back(w.second & 0xFF);
  }

  append_crc(frame);
  return frame;
}

std::vector<uint8_t> build_write_single_request(uint16_t address, uint16_t value) {
  std::vector<uint8_t> frame;
  frame.push_back(SLAVE_ADDRESS);
  frame.push_back(FUNC_WRITE_SINGLE);
  frame.push_back((address >> 8) & 0xFF);
  frame.push_back(address & 0xFF);
  frame.push_back((value >> 8) & 0xFF);
  frame.push_back(value & 0xFF);

  append_crc(frame);
  return frame;
}

bool validate_frame_crc(const uint8_t *data, size_t len) {
  if (len < MIN_FRAME_SIZE)
    return false;

  uint16_t received_crc = data[len - 2] | (data[len - 1] << 8);
  uint16_t calculated_crc = crc16(data, len - 2);
  return received_crc == calculated_crc;
}

bool is_error_response(uint8_t function_code) {
  return (function_code & ERROR_MASK) != 0;
}

size_t get_response_header_size(uint8_t function_code) {
  if (is_error_response(function_code)) {
    // Error response: slave_addr + func + error_code + CRC(2) = 5
    return 5;
  }

  switch (function_code) {
    case FUNC_READ_RANGES:
    case FUNC_READ_REGISTERS:
      // Variable length: slave_addr + func + byte_count, then data + CRC
      // Return header before we know total length
      return 3;

    case FUNC_WRITE_REGISTERS:
      // Echo response: slave_addr + func + CRC(2) = 4
      return 4;

    case FUNC_WRITE_SINGLE:
      // Echo: slave_addr + func + addr(2) + value(2) + CRC(2) = 8
      return 8;

    default:
      return 3;  // Conservative: try to read byte_count
  }
}

std::vector<uint16_t> parse_register_values(const uint8_t *data, size_t data_len) {
  std::vector<uint16_t> values;
  // data_len is the number of data bytes (from byte_count field)
  // Each register is 2 bytes, big-endian
  for (size_t i = 0; i + 1 < data_len; i += 2) {
    values.push_back((data[i] << 8) | data[i + 1]);
  }
  return values;
}

}  // namespace waterfurnace
}  // namespace esphome
