#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <utility>

namespace esphome {
namespace waterfurnace {

// ModBus RTU custom function codes for WaterFurnace Aurora
static constexpr uint8_t FUNC_READ_RANGES = 65;       // Read multiple discontiguous register ranges
static constexpr uint8_t FUNC_READ_REGISTERS = 66;    // Read multiple discontiguous registers
static constexpr uint8_t FUNC_WRITE_REGISTERS = 67;   // Write multiple discontiguous registers
static constexpr uint8_t FUNC_WRITE_SINGLE = 6;       // Standard ModBus write single register

static constexpr uint8_t SLAVE_ADDRESS = 1;
static constexpr uint8_t ERROR_MASK = 0x80;

// Maximum registers per request (WaterFurnace limit)
static constexpr size_t MAX_REGISTERS_PER_REQUEST = 100;

// Frame size limits
static constexpr size_t MIN_FRAME_SIZE = 4;            // slave + func + 2 CRC bytes minimum
static constexpr size_t MAX_FRAME_SIZE = 256;

/// Calculate ModBus CRC16 using polynomial 0xA001
uint16_t crc16(const uint8_t *data, size_t len);

/// Build a function 65 request: read multiple register ranges
/// Each pair is (start_address, quantity)
/// Returns complete RTU frame with CRC
std::vector<uint8_t> build_read_ranges_request(
    const std::vector<std::pair<uint16_t, uint16_t>> &ranges);

/// Build a function 66 request: read individual discontiguous registers
/// Returns complete RTU frame with CRC
std::vector<uint8_t> build_read_registers_request(
    const std::vector<uint16_t> &addresses);

/// Build a function 67 request: write multiple discontiguous registers
/// Each pair is (address, value)
/// Returns complete RTU frame with CRC
std::vector<uint8_t> build_write_registers_request(
    const std::vector<std::pair<uint16_t, uint16_t>> &writes);

/// Build a function 6 request: write single holding register
/// Returns complete RTU frame with CRC
std::vector<uint8_t> build_write_single_request(uint16_t address, uint16_t value);

/// Validate a received frame's CRC
/// Returns true if CRC is valid
bool validate_frame_crc(const uint8_t *data, size_t len);

/// Check if a response indicates an error (function code has bit 7 set)
bool is_error_response(uint8_t function_code);

/// Get the expected minimum response size for a given function code
/// For variable-length responses (func 65/66), returns the header size before byte count
size_t get_response_header_size(uint8_t function_code);

/// Parse a function 65/66 response payload into register values
/// The response data should start after slave_addr and function_code bytes
/// Returns vector of uint16_t values in order
std::vector<uint16_t> parse_register_values(const uint8_t *data, size_t data_len);

}  // namespace waterfurnace
}  // namespace esphome
