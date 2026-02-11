#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace esphome {
namespace waterfurnace {

// --- Register data type conversions ---

enum class RegisterType : uint8_t {
  UNSIGNED,         // Raw uint16
  SIGNED,           // int16
  TENTHS,           // uint16 / 10.0
  SIGNED_TENTHS,    // int16 / 10.0
  HUNDREDTHS,       // uint16 / 100.0
  BOOLEAN,          // 0 or 1
  UINT32,           // Two consecutive registers: (hi << 16) | lo
  INT32,            // Two consecutive registers, signed
};

/// Convert a raw register value to float based on its type
inline float convert_register(uint16_t raw, RegisterType type) {
  switch (type) {
    case RegisterType::UNSIGNED:
      return static_cast<float>(raw);
    case RegisterType::SIGNED:
      return static_cast<float>(static_cast<int16_t>(raw));
    case RegisterType::TENTHS:
      return raw / 10.0f;
    case RegisterType::SIGNED_TENTHS:
      return static_cast<int16_t>(raw) / 10.0f;
    case RegisterType::HUNDREDTHS:
      return raw / 100.0f;
    case RegisterType::BOOLEAN:
      return (raw != 0) ? 1.0f : 0.0f;
    default:
      return static_cast<float>(raw);
  }
}

/// Convert two consecutive registers to uint32
inline uint32_t to_uint32(uint16_t hi, uint16_t lo) {
  return (static_cast<uint32_t>(hi) << 16) | lo;
}

/// Convert two consecutive registers to int32
inline int32_t to_int32(uint16_t hi, uint16_t lo) {
  return static_cast<int32_t>(to_uint32(hi, lo));
}

// --- Component detection registers ---

static constexpr uint16_t REG_THERMOSTAT_STATUS = 800;
static constexpr uint16_t REG_AXB_STATUS = 806;
static constexpr uint16_t REG_IZ2_STATUS = 812;
static constexpr uint16_t REG_AOC_STATUS = 815;
static constexpr uint16_t REG_MOC_STATUS = 818;
static constexpr uint16_t REG_EEV2_STATUS = 824;
static constexpr uint16_t REG_AWL_STATUS = 827;

// Component status values
static constexpr uint16_t COMPONENT_ACTIVE = 1;
static constexpr uint16_t COMPONENT_ADDED = 2;
static constexpr uint16_t COMPONENT_REMOVED = 3;
static constexpr uint16_t COMPONENT_MISSING = 0xFFFF;

// Version registers (status_reg + 1), divided by 100.0
static constexpr uint16_t REG_THERMOSTAT_VERSION = 801;
static constexpr uint16_t REG_AXB_VERSION = 807;
static constexpr uint16_t REG_IZ2_VERSION = 813;

// --- System identification registers ---

static constexpr uint16_t REG_ABC_VERSION = 2;       // HUNDREDTHS
static constexpr uint16_t REG_ABC_PROGRAM = 88;      // 4-char string (2 registers)
static constexpr uint16_t REG_MODEL_NUMBER = 92;     // 12-char string (6 registers)
static constexpr uint16_t REG_SERIAL_NUMBER = 105;   // 5-char string (3 registers)
static constexpr uint16_t REG_IZ2_ZONE_COUNT = 483;

// --- Blower/pump/compressor type registers ---

static constexpr uint16_t REG_BLOWER_TYPE = 404;
static constexpr uint16_t REG_PUMP_TYPE = 413;
static constexpr uint16_t REG_COMPRESSOR_HZ = 412;

// Blower type values
static constexpr uint16_t BLOWER_PSC = 0;
static constexpr uint16_t BLOWER_ECM_230 = 1;
static constexpr uint16_t BLOWER_ECM_277 = 2;
static constexpr uint16_t BLOWER_5SPD_460 = 3;

// --- Status registers ---

static constexpr uint16_t REG_LINE_VOLTAGE = 16;
static constexpr uint16_t REG_FP1_TEMP = 19;           // SIGNED_TENTHS - Cooling liquid line
static constexpr uint16_t REG_FP2_TEMP = 20;           // SIGNED_TENTHS - Air coil temp
static constexpr uint16_t REG_LAST_FAULT = 25;         // Bit 15 = lockout flag, bits 0-14 = fault code
static constexpr uint16_t REG_LAST_LOCKOUT = 26;
static constexpr uint16_t REG_SYSTEM_OUTPUTS = 30;     // Bitmask
static constexpr uint16_t REG_SYSTEM_INPUTS = 31;      // Bitmask
static constexpr uint16_t REG_STATUS1 = 344;
static constexpr uint16_t REG_STATUS2 = 362;
static constexpr uint16_t REG_DEMAND = 502;            // SIGNED_TENTHS

// System output bits (register 30)
static constexpr uint16_t OUTPUT_CC = 0x01;             // Compressor stage 1
static constexpr uint16_t OUTPUT_CC2 = 0x02;            // Compressor stage 2
static constexpr uint16_t OUTPUT_RV = 0x04;             // Reversing valve (cooling)
static constexpr uint16_t OUTPUT_BLOWER = 0x08;
static constexpr uint16_t OUTPUT_EH1 = 0x10;            // Aux/emergency heat stage 1
static constexpr uint16_t OUTPUT_EH2 = 0x20;            // Aux/emergency heat stage 2
static constexpr uint16_t OUTPUT_ACCESSORY = 0x200;
static constexpr uint16_t OUTPUT_LOCKOUT = 0x400;
static constexpr uint16_t OUTPUT_ALARM = 0x800;

// --- Thermostat registers (single zone, AWL) ---

static constexpr uint16_t REG_ENTERING_AIR = 740;       // SIGNED_TENTHS
static constexpr uint16_t REG_HUMIDITY = 741;            // UNSIGNED (%)
static constexpr uint16_t REG_OUTDOOR_TEMP = 742;        // SIGNED_TENTHS
static constexpr uint16_t REG_HEATING_SETPOINT = 745;    // TENTHS
static constexpr uint16_t REG_COOLING_SETPOINT = 746;    // TENTHS
static constexpr uint16_t REG_AMBIENT_TEMP = 747;        // SIGNED_TENTHS

// Thermostat config registers (read)
static constexpr uint16_t REG_FAN_CONFIG = 12005;        // Bit-packed fan mode
static constexpr uint16_t REG_MODE_CONFIG = 12006;       // Bit-packed heating mode

// Thermostat write registers (single zone)
static constexpr uint16_t REG_WRITE_MODE = 12606;
static constexpr uint16_t REG_WRITE_HEATING_SP = 12619;  // value * 10
static constexpr uint16_t REG_WRITE_COOLING_SP = 12620;  // value * 10
static constexpr uint16_t REG_WRITE_FAN_MODE = 12621;
static constexpr uint16_t REG_WRITE_FAN_ON_TIME = 12622;
static constexpr uint16_t REG_WRITE_FAN_OFF_TIME = 12623;

// --- AXB registers ---

static constexpr uint16_t REG_AXB_INPUTS = 1103;        // Bitmask
static constexpr uint16_t REG_AXB_OUTPUTS = 1104;       // Bitmask
static constexpr uint16_t REG_BLOWER_AMPS = 1105;       // TENTHS
static constexpr uint16_t REG_AUX_AMPS = 1106;          // TENTHS
static constexpr uint16_t REG_COMPRESSOR_AMPS = 1107;   // TENTHS
static constexpr uint16_t REG_AIR_COIL_AMPS = 1108;     // TENTHS

// Performance registers (AXB)
static constexpr uint16_t REG_LEAVING_AIR = 900;         // SIGNED_TENTHS
static constexpr uint16_t REG_LEAVING_WATER = 1110;      // SIGNED_TENTHS
static constexpr uint16_t REG_ENTERING_WATER = 1111;     // SIGNED_TENTHS
static constexpr uint16_t REG_OUTDOOR_TEMP2 = 1109;      // SIGNED_TENTHS
static constexpr uint16_t REG_SUPERHEAT_TEMP = 1112;     // SIGNED_TENTHS
static constexpr uint16_t REG_SUCTION_TEMP = 1113;       // SIGNED_TENTHS
static constexpr uint16_t REG_DHW_TEMP = 1114;           // SIGNED_TENTHS
static constexpr uint16_t REG_DISCHARGE_PRESSURE = 1115; // TENTHS (psi)
static constexpr uint16_t REG_SUCTION_PRESSURE = 1116;   // TENTHS (psi)
static constexpr uint16_t REG_WATERFLOW = 1117;          // TENTHS (gpm)
static constexpr uint16_t REG_LOOP_PRESSURE = 1119;      // TENTHS (psi)
static constexpr uint16_t REG_SUBCOOLING = 1124;         // SIGNED_TENTHS
static constexpr uint16_t REG_SUPERHEAT = 1125;          // SIGNED_TENTHS
static constexpr uint16_t REG_APPROACH = 1134;           // SIGNED_TENTHS
static constexpr uint16_t REG_EEV_OPEN = 1135;           // SIGNED_TENTHS
static constexpr uint16_t REG_EEV_CALC = 1136;           // SIGNED_TENTHS

// --- Power/energy registers ---

static constexpr uint16_t REG_COMPRESSOR_WATTS_HI = 1146;
static constexpr uint16_t REG_COMPRESSOR_WATTS_LO = 1147;
static constexpr uint16_t REG_BLOWER_WATTS_HI = 1148;
static constexpr uint16_t REG_BLOWER_WATTS_LO = 1149;
static constexpr uint16_t REG_AUX_HEAT_WATTS_HI = 1150;
static constexpr uint16_t REG_AUX_HEAT_WATTS_LO = 1151;
static constexpr uint16_t REG_TOTAL_WATTS_HI = 1152;
static constexpr uint16_t REG_TOTAL_WATTS_LO = 1153;
static constexpr uint16_t REG_HEAT_EXTRACTION_HI = 1154;
static constexpr uint16_t REG_HEAT_EXTRACTION_LO = 1155;
static constexpr uint16_t REG_HEAT_REJECTION_HI = 1156;
static constexpr uint16_t REG_HEAT_REJECTION_LO = 1157;
static constexpr uint16_t REG_PUMP_WATTS_HI = 1164;
static constexpr uint16_t REG_PUMP_WATTS_LO = 1165;

// --- VS Drive registers ---

static constexpr uint16_t REG_VS_SPEED_DESIRED = 3000;
static constexpr uint16_t REG_VS_SPEED_ACTUAL = 3001;
static constexpr uint16_t REG_VS_DRIVE_STATUS = 3220;
static constexpr uint16_t REG_VS_INVERTER_TEMP = 3522;  // SIGNED_TENTHS
static constexpr uint16_t REG_VS_FAN_SPEED = 3524;
static constexpr uint16_t REG_VS_DISCHARGE_TEMP = 3325; // SIGNED_TENTHS
static constexpr uint16_t REG_VS_DISCHARGE_PRESS = 3322; // TENTHS
static constexpr uint16_t REG_VS_SUCTION_PRESS = 3323;   // TENTHS

// --- IZ2 zone registers ---

// Read registers: base + (zone-1)*3
static constexpr uint16_t REG_IZ2_ZONE_BASE = 31007;
// Per zone: +0 = ambient temp, +1 = config1 (fan/cooling SP), +2 = config2 (mode/heating SP)
static constexpr uint16_t REG_IZ2_ZONE_CONFIG3_BASE = 31200;
// Per zone: +0 = zone priority/size

// Write registers: base + (zone-1)*9
static constexpr uint16_t REG_IZ2_WRITE_BASE = 21202;
// Per zone: +0 = mode, +1 = heating SP, +2 = cooling SP, +3 = fan mode,
//           +4 = fan on time, +5 = fan off time

// --- DHW register ---

static constexpr uint16_t REG_DHW_SETPOINT = 401;       // TENTHS
static constexpr uint16_t REG_DHW_ENABLE = 400;

// --- Heating mode values ---

static constexpr uint16_t MODE_OFF = 0;
static constexpr uint16_t MODE_AUTO = 1;
static constexpr uint16_t MODE_COOL = 2;
static constexpr uint16_t MODE_HEAT = 3;
static constexpr uint16_t MODE_EHEAT = 4;

// --- Fan mode values ---

static constexpr uint16_t FAN_AUTO = 0;
static constexpr uint16_t FAN_CONTINUOUS = 1;
static constexpr uint16_t FAN_INTERMITTENT = 2;

// --- VS Drive program names ---
// Register 88 decoded: "ABCVSP", "ABCVSPR", "ABCSPLVS" indicate VS drive

// --- Fault codes ---

struct FaultInfo {
  uint8_t code;
  const char *description;
};

static constexpr FaultInfo FAULT_TABLE[] = {
    {1, "Input Error"},
    {2, "High Pressure"},
    {3, "Low Pressure"},
    {4, "Freeze Detect FP2"},
    {5, "Freeze Detect FP1"},
    {7, "Condensate Overflow"},
    {8, "Over/Under Voltage"},
    {9, "AirF/RPM"},
    {10, "Compressor Monitor"},
    {11, "FP1/2 Sensor Error"},
    {12, "RefPerfrm Error"},
    {13, "Non-Critical AXB Sensor Error"},
    {14, "Critical AXB Sensor Error"},
    {15, "Hot Water Limit"},
    {16, "VS Pump Error"},
    {17, "Communicating Thermostat Error"},
    {18, "Non-Critical Comms Error"},
    {19, "Critical Comms Error"},
    {21, "Low Loop Pressure"},
    {22, "Communicating ECM Error"},
    {23, "HA Alarm 1"},
    {24, "HA Alarm 2"},
    {25, "AxbEev Error"},
    {41, "High Drive Temp"},
    {42, "High Discharge Temp"},
    {99, "System Reset"},
};

static constexpr size_t FAULT_TABLE_SIZE = sizeof(FAULT_TABLE) / sizeof(FAULT_TABLE[0]);

inline const char *fault_code_to_string(uint8_t code) {
  for (size_t i = 0; i < FAULT_TABLE_SIZE; i++) {
    if (FAULT_TABLE[i].code == code)
      return FAULT_TABLE[i].description;
  }
  return "Unknown Fault";
}

// --- Polling register groups ---

// Group 0: System ID (read once at setup)
inline std::vector<std::pair<uint16_t, uint16_t>> get_system_id_ranges() {
  return {
      {2, 1},        // ABC version
      {88, 4},       // ABC program (8 chars = 4 registers)
      {92, 12},      // Model number (24 chars = 12 registers)
      {105, 5},      // Serial number (10 chars = 5 registers)
      {400, 2},      // DHW enable, DHW setpoint
      {404, 1},      // Blower type
      {412, 2},      // Compressor Hz, pump type
  };
}

// Component detection registers (read once at setup)
inline std::vector<std::pair<uint16_t, uint16_t>> get_component_detect_ranges() {
  return {
      {800, 3},      // Thermostat status, version, revision
      {806, 3},      // AXB status, version, revision
      {812, 3},      // IZ2 status, version, revision
      {815, 3},      // AOC status
      {818, 3},      // MOC status
      {824, 3},      // EEV2 status
      {827, 3},      // AWL status
      {483, 1},      // IZ2 zone count
  };
}

// Group 1: Thermostat/status (always poll)
inline std::vector<std::pair<uint16_t, uint16_t>> get_thermostat_ranges() {
  return {
      {19, 2},       // FP1, FP2 temps
      {25, 2},       // Last fault, last lockout
      {30, 2},       // System outputs, system inputs
      {502, 1},      // Demand
      {740, 3},      // Entering air, humidity, outdoor temp
      {745, 3},      // Heating SP, cooling SP, ambient
  };
}

// Group 1b: Thermostat config (if AWL thermostat)
inline std::vector<uint16_t> get_thermostat_config_registers() {
  return {12005, 12006};
}

// Group 2: AXB performance (if AXB present)
inline std::vector<std::pair<uint16_t, uint16_t>> get_axb_ranges() {
  return {
      {400, 2},      // DHW enable, DHW setpoint
      {900, 1},      // Leaving air temp
      {1103, 6},     // AXB inputs through air coil amps
      {1109, 11},    // Outdoor2, LWT, EWT, superheat, suction, DHW, discharge/suction press, waterflow, 1118, loop press
      {1124, 2},     // Subcooling, superheat
      {1134, 3},     // Approach, EEV open, EEV calc
  };
}

// Group 3: Power (if energy monitoring)
inline std::vector<std::pair<uint16_t, uint16_t>> get_power_ranges() {
  return {
      {16, 1},       // Line voltage
      {1146, 12},    // Compressor/blower/aux/total watts, heat extraction/rejection
      {1164, 2},     // Pump watts
  };
}

// Group 4: VS Drive (if VS)
inline std::vector<std::pair<uint16_t, uint16_t>> get_vs_drive_ranges() {
  return {
      {3000, 2},     // Speed desired, actual
      {3220, 8},     // VS drive status block
      {3322, 9},     // VS pressures and temps
      {3522, 1},     // Inverter temp
      {3524, 1},     // Fan speed
  };
}

// IZ2 zone read registers for N zones
inline std::vector<std::pair<uint16_t, uint16_t>> get_iz2_ranges(uint8_t zone_count) {
  std::vector<std::pair<uint16_t, uint16_t>> ranges;
  if (zone_count > 0) {
    // Zone ambient/config registers: 31007 through 31007 + zone_count*3 - 1
    ranges.push_back({31007, static_cast<uint16_t>(zone_count * 3)});
    // Zone config3 registers: 31200 through 31200 + zone_count*3 - 1
    ranges.push_back({31200, static_cast<uint16_t>(zone_count * 3)});
  }
  return ranges;
}

// --- IZ2 zone register extraction helpers ---

// Extract mode from zone_configuration2 register
inline uint8_t iz2_extract_mode(uint16_t config2) {
  return (config2 >> 8) & 0x03;
}

// Extract fan mode from zone_configuration1 register
inline uint8_t iz2_extract_fan_mode(uint16_t config1) {
  if (config1 & 0x80)
    return FAN_CONTINUOUS;
  if (config1 & 0x100)
    return FAN_INTERMITTENT;
  return FAN_AUTO;
}

// Extract cooling setpoint from zone_configuration1 (°F, no decimal)
inline uint8_t iz2_extract_cooling_setpoint(uint16_t config1) {
  return ((config1 & 0x7E) >> 1) + 36;
}

// Extract heating setpoint from zone_configuration1 + zone_configuration2
// Requires carry bit from config1 bit 0, and bits 11-15 from config2
inline uint8_t iz2_extract_heating_setpoint(uint16_t config1, uint16_t config2) {
  uint8_t carry = config1 & 0x01;
  return ((carry << 5) | ((config2 & 0xF800) >> 11)) + 36;
}

// Extract damper state from zone_configuration2
inline bool iz2_damper_open(uint16_t config2) {
  return (config2 & 0x10) != 0;
}

}  // namespace waterfurnace
}  // namespace esphome
