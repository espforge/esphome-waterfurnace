// Unit tests for protocol.h/cpp and registers.h (converted to gtest)

#include <gtest/gtest.h>
#include "../../components/waterfurnace/protocol.cpp"
#include "../../components/waterfurnace/registers.h"

using namespace esphome::waterfurnace;

// ====== CRC16 ======

TEST(CRC16, KnownVector) {
  uint8_t data[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01};
  EXPECT_EQ(crc16(data, sizeof(data)), 0x0A84);
}

TEST(CRC16, Func65Request) {
  uint8_t data[] = {0x01, 0x41, 0x00, 0x58, 0x00, 0x04};
  EXPECT_EQ(crc16(data, sizeof(data)), 0xD5BD);
}

TEST(CRC16, Func66Request) {
  uint8_t data[] = {0x01, 0x42, 0x02, 0xE9, 0x02, 0xEA};
  EXPECT_EQ(crc16(data, sizeof(data)), 0x6629);
}

TEST(CRC16, MinimalFrame) {
  uint8_t data[] = {0x01, 0x41};
  EXPECT_EQ(crc16(data, sizeof(data)), 0x10C0);
}

// ====== Frame Building ======

TEST(FrameBuilding, ReadRangesBasic) {
  auto frame = build_read_ranges_request({{88, 4}});
  ASSERT_EQ(frame.size(), 8u);
  EXPECT_EQ(frame[0], SLAVE_ADDRESS);
  EXPECT_EQ(frame[1], FUNC_READ_RANGES);
  EXPECT_EQ(frame[2], 0x00);
  EXPECT_EQ(frame[3], 0x58);
  EXPECT_EQ(frame[4], 0x00);
  EXPECT_EQ(frame[5], 0x04);
  EXPECT_EQ(frame[6], 0xBD);
  EXPECT_EQ(frame[7], 0xD5);
}

TEST(FrameBuilding, ReadRangesMultiple) {
  auto frame = build_read_ranges_request({{19, 2}, {30, 2}});
  ASSERT_EQ(frame.size(), 12u);
  EXPECT_EQ(frame[0], 0x01);
  EXPECT_EQ(frame[1], FUNC_READ_RANGES);
  EXPECT_EQ(frame[2], 0x00);
  EXPECT_EQ(frame[3], 0x13);
  EXPECT_EQ(frame[4], 0x00);
  EXPECT_EQ(frame[5], 0x02);
  EXPECT_EQ(frame[6], 0x00);
  EXPECT_EQ(frame[7], 0x1E);
  EXPECT_EQ(frame[8], 0x00);
  EXPECT_EQ(frame[9], 0x02);
  EXPECT_TRUE(validate_frame_crc(frame.data(), frame.size()));
}

TEST(FrameBuilding, ReadRegistersRequest) {
  auto frame = build_read_registers_request({745, 746});
  ASSERT_EQ(frame.size(), 8u);
  EXPECT_EQ(frame[0], SLAVE_ADDRESS);
  EXPECT_EQ(frame[1], FUNC_READ_REGISTERS);
  EXPECT_EQ(frame[2], 0x02);
  EXPECT_EQ(frame[3], 0xE9);
  EXPECT_EQ(frame[4], 0x02);
  EXPECT_EQ(frame[5], 0xEA);
  EXPECT_TRUE(validate_frame_crc(frame.data(), frame.size()));
}

TEST(FrameBuilding, WriteRegistersRequest) {
  auto frame = build_write_registers_request({{12619, 700}, {12620, 730}});
  ASSERT_EQ(frame.size(), 12u);
  EXPECT_EQ(frame[0], SLAVE_ADDRESS);
  EXPECT_EQ(frame[1], FUNC_WRITE_REGISTERS);
  EXPECT_EQ(frame[2], 0x31);
  EXPECT_EQ(frame[3], 0x4B);
  EXPECT_EQ(frame[4], 0x02);
  EXPECT_EQ(frame[5], 0xBC);
  EXPECT_TRUE(validate_frame_crc(frame.data(), frame.size()));
}

TEST(FrameBuilding, WriteSingleRequest) {
  auto frame = build_write_single_request(400, 1);
  ASSERT_EQ(frame.size(), 8u);
  EXPECT_EQ(frame[0], SLAVE_ADDRESS);
  EXPECT_EQ(frame[1], FUNC_WRITE_SINGLE);
  EXPECT_EQ(frame[2], 0x01);
  EXPECT_EQ(frame[3], 0x90);
  EXPECT_EQ(frame[4], 0x00);
  EXPECT_EQ(frame[5], 0x01);
  EXPECT_TRUE(validate_frame_crc(frame.data(), frame.size()));
}

// ====== Frame Validation ======

TEST(FrameValidation, ValidCRC) {
  auto frame = build_read_ranges_request({{88, 4}});
  EXPECT_TRUE(validate_frame_crc(frame.data(), frame.size()));
}

TEST(FrameValidation, InvalidCRC) {
  auto frame = build_read_ranges_request({{88, 4}});
  frame[3] ^= 0xFF;
  EXPECT_FALSE(validate_frame_crc(frame.data(), frame.size()));
}

TEST(FrameValidation, TooShort) {
  uint8_t data[] = {0x01, 0x03};
  EXPECT_FALSE(validate_frame_crc(data, 2));
}

// ====== Response Parsing ======

TEST(ResponseParsing, BasicValues) {
  uint8_t data[] = {0x02, 0xBC, 0x02, 0xDA};
  auto values = parse_register_values(data, 4);
  ASSERT_EQ(values.size(), 2u);
  EXPECT_EQ(values[0], 700u);
  EXPECT_EQ(values[1], 730u);
}

TEST(ResponseParsing, Empty) {
  auto values = parse_register_values(nullptr, 0);
  EXPECT_EQ(values.size(), 0u);
}

TEST(ResponseParsing, OddBytes) {
  uint8_t data[] = {0x02, 0xBC, 0xFF};
  auto values = parse_register_values(data, 3);
  ASSERT_EQ(values.size(), 1u);
  EXPECT_EQ(values[0], 700u);
}

TEST(ResponseParsing, ErrorResponseTrue) {
  EXPECT_TRUE(is_error_response(0xC1));
  EXPECT_TRUE(is_error_response(0xC2));
  EXPECT_TRUE(is_error_response(0x83));
}

TEST(ResponseParsing, ErrorResponseFalse) {
  EXPECT_FALSE(is_error_response(0x41));
  EXPECT_FALSE(is_error_response(0x42));
  EXPECT_FALSE(is_error_response(0x03));
}

// ====== Response Header Size ======

TEST(ResponseHeader, ErrorResponse) {
  EXPECT_EQ(get_response_header_size(0xC1), 5u);
}

TEST(ResponseHeader, ReadRanges) {
  EXPECT_EQ(get_response_header_size(FUNC_READ_RANGES), 3u);
}

TEST(ResponseHeader, WriteSingle) {
  EXPECT_EQ(get_response_header_size(FUNC_WRITE_SINGLE), 8u);
}

TEST(ResponseHeader, WriteRegisters) {
  EXPECT_EQ(get_response_header_size(FUNC_WRITE_REGISTERS), 4u);
}

// ====== Register Type Conversions ======

TEST(RegisterConversion, Unsigned) {
  EXPECT_FLOAT_EQ(convert_register(240, RegisterType::UNSIGNED), 240.0f);
  EXPECT_FLOAT_EQ(convert_register(65535, RegisterType::UNSIGNED), 65535.0f);
}

TEST(RegisterConversion, Signed) {
  EXPECT_FLOAT_EQ(convert_register(0xFFFF, RegisterType::SIGNED), -1.0f);
  EXPECT_FLOAT_EQ(convert_register(0xFF9C, RegisterType::SIGNED), -100.0f);
  EXPECT_FLOAT_EQ(convert_register(100, RegisterType::SIGNED), 100.0f);
}

TEST(RegisterConversion, Tenths) {
  EXPECT_FLOAT_EQ(convert_register(700, RegisterType::TENTHS), 70.0f);
  EXPECT_FLOAT_EQ(convert_register(735, RegisterType::TENTHS), 73.5f);
}

TEST(RegisterConversion, SignedTenths) {
  EXPECT_FLOAT_EQ(convert_register(700, RegisterType::SIGNED_TENTHS), 70.0f);
  uint16_t neg = static_cast<uint16_t>(static_cast<int16_t>(-105));
  EXPECT_FLOAT_EQ(convert_register(neg, RegisterType::SIGNED_TENTHS), -10.5f);
}

TEST(RegisterConversion, Hundredths) {
  EXPECT_FLOAT_EQ(convert_register(705, RegisterType::HUNDREDTHS), 7.05f);
  EXPECT_FLOAT_EQ(convert_register(200, RegisterType::HUNDREDTHS), 2.0f);
}

TEST(RegisterConversion, Boolean) {
  EXPECT_FLOAT_EQ(convert_register(0, RegisterType::BOOLEAN), 0.0f);
  EXPECT_FLOAT_EQ(convert_register(1, RegisterType::BOOLEAN), 1.0f);
  EXPECT_FLOAT_EQ(convert_register(42, RegisterType::BOOLEAN), 1.0f);
}

// ====== 32-bit Registers ======

TEST(Register32Bit, Uint32) {
  EXPECT_EQ(to_uint32(0x0001, 0x0000), 0x00010000u);
  EXPECT_EQ(to_uint32(0, 500), 500u);
  EXPECT_EQ(to_uint32(1, 500), 65536u + 500u);
}

TEST(Register32Bit, Int32Negative) {
  EXPECT_EQ(to_int32(0xFFFF, 0xFFFF), -1);
  int32_t val = -1000;
  uint32_t uval = static_cast<uint32_t>(val);
  EXPECT_EQ(to_int32(uval >> 16, uval & 0xFFFF), -1000);
}

// ====== IZ2 Zone Extraction ======

TEST(IZ2, FanModeAuto) {
  EXPECT_EQ(iz2_extract_fan_mode(0x0000), FAN_AUTO);
  EXPECT_EQ(iz2_extract_fan_mode(0x0001), FAN_AUTO);
}

TEST(IZ2, FanModeContinuous) {
  EXPECT_EQ(iz2_extract_fan_mode(0x0080), FAN_CONTINUOUS);
  EXPECT_EQ(iz2_extract_fan_mode(0x00FF), FAN_CONTINUOUS);
}

TEST(IZ2, FanModeIntermittent) {
  EXPECT_EQ(iz2_extract_fan_mode(0x0100), FAN_INTERMITTENT);
  EXPECT_EQ(iz2_extract_fan_mode(0x0101), FAN_INTERMITTENT);
}

TEST(IZ2, CoolingSetpoint) {
  EXPECT_EQ(iz2_extract_cooling_setpoint(0x4E), 75u);
  EXPECT_EQ(iz2_extract_cooling_setpoint(0x0000), 36u);
  EXPECT_EQ(iz2_extract_cooling_setpoint(0x007E), 99u);
}

TEST(IZ2, HeatingSetpoint) {
  EXPECT_EQ(iz2_extract_heating_setpoint(0x0001, 0x0000), 68u);
  EXPECT_EQ(iz2_extract_heating_setpoint(0x0001, 0x2000), 72u);
}

TEST(IZ2, ExtractMode) {
  EXPECT_EQ(iz2_extract_mode(0x0000), MODE_OFF);
  EXPECT_EQ(iz2_extract_mode(0x0100), MODE_AUTO);
  EXPECT_EQ(iz2_extract_mode(0x0200), MODE_COOL);
  EXPECT_EQ(iz2_extract_mode(0x0300), MODE_HEAT);
  EXPECT_EQ(iz2_extract_mode(0x0400), MODE_EHEAT);
  // Real E-Heat config2 value from device: 0x2410
  EXPECT_EQ(iz2_extract_mode(0x2410), MODE_EHEAT);
}

TEST(IZ2, DamperOpen) {
  EXPECT_TRUE(iz2_damper_open(0x0010));
  EXPECT_FALSE(iz2_damper_open(0x0000));
  EXPECT_FALSE(iz2_damper_open(0x0020));
}

// ====== Fault Codes ======

TEST(FaultCodes, Known) {
  EXPECT_STREQ(fault_code_to_string(1), "Input Error");
  EXPECT_STREQ(fault_code_to_string(2), "High Pressure");
  EXPECT_STREQ(fault_code_to_string(99), "System Reset");
}

TEST(FaultCodes, Unknown) {
  EXPECT_STREQ(fault_code_to_string(50), "Unknown Fault");
  EXPECT_STREQ(fault_code_to_string(0), "Unknown Fault");
}

// ====== Register Groups ======

TEST(RegisterGroups, SystemIdCount) {
  auto ranges = get_system_id_ranges();
  size_t total = 0;
  for (const auto &r : ranges) total += r.second;
  EXPECT_EQ(total, 27u);
}

TEST(RegisterGroups, ComponentDetectCount) {
  auto ranges = get_component_detect_ranges();
  size_t total = 0;
  for (const auto &r : ranges) total += r.second;
  EXPECT_EQ(total, 22u);
}

TEST(RegisterGroups, IZ2ZoneConstants) {
  // Verify IZ2 zone register layout constants
  EXPECT_EQ(REG_IZ2_ZONE_BASE, 31007u);
  EXPECT_EQ(REG_IZ2_ZONE_CONFIG3_BASE, 31200u);
  EXPECT_EQ(REG_IZ2_OUTDOOR_TEMP, 31003u);
  EXPECT_EQ(REG_IZ2_DEMAND, 31005u);
  // Zone 1: base + (1-1)*3 = 31007
  // Zone 3: base + (3-1)*3 = 31013
  EXPECT_EQ(REG_IZ2_ZONE_BASE + 6, 31013u);
}

// ====== Constants ======

TEST(Constants, FunctionCodes) {
  EXPECT_EQ(FUNC_READ_RANGES, 65);
  EXPECT_EQ(FUNC_READ_REGISTERS, 66);
  EXPECT_EQ(FUNC_WRITE_REGISTERS, 67);
  EXPECT_EQ(FUNC_WRITE_SINGLE, 6);
}

TEST(Constants, SlaveAddress) {
  EXPECT_EQ(SLAVE_ADDRESS, 1);
}
