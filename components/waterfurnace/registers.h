#pragma once

#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

namespace esphome {
namespace waterfurnace {

// --- Capability gating ---
// Each sensor declares a capability requirement; the hub filters listeners
// whose capability is not met by the detected hardware.

enum class RegisterCapability : uint8_t {
  NONE,               // Always pollable (base system registers)
  AWL_THERMOSTAT,     // Thermostat v3.0+ (502, 745-747, 12005-12006)
  AWL_AXB,            // AXB v2.0+ (900)
  AWL_COMMUNICATING,  // Thermostat v3.0+ OR IZ2 v2.0+ (741-742)
  AXB,                // AXB present (1103-1119, 400-401, DHW)
  REFRIGERATION,      // AXB + energy_monitor >= 1 (1109, 1124-1125, 1134, 1154-1157)
  ENERGY,             // AXB + energy_monitor == 2 (16, 1146-1165)
  VS_DRIVE,           // VS drive present (362, 1135-1136, 3000-3906)
  IZ2,                // IZ2 with AWL v2.0+ (31000+)
};

/// Convert a Python-provided capability string to enum
inline RegisterCapability capability_from_string(const char *str) {
  if (strcmp(str, "awl_thermostat") == 0) return RegisterCapability::AWL_THERMOSTAT;
  if (strcmp(str, "awl_axb") == 0) return RegisterCapability::AWL_AXB;
  if (strcmp(str, "awl_communicating") == 0) return RegisterCapability::AWL_COMMUNICATING;
  if (strcmp(str, "axb") == 0) return RegisterCapability::AXB;
  if (strcmp(str, "refrigeration") == 0) return RegisterCapability::REFRIGERATION;
  if (strcmp(str, "energy") == 0) return RegisterCapability::ENERGY;
  if (strcmp(str, "vs_drive") == 0) return RegisterCapability::VS_DRIVE;
  if (strcmp(str, "iz2") == 0) return RegisterCapability::IZ2;
  return RegisterCapability::NONE;
}

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
static constexpr uint16_t REG_ABC_PROGRAM = 88;      // 8-char string (4 registers)
static constexpr uint16_t REG_MODEL_NUMBER = 92;     // 24-char string (12 registers)
static constexpr uint16_t REG_SERIAL_NUMBER = 105;   // 10-char string (5 registers)
static constexpr uint16_t REG_IZ2_ZONE_COUNT = 483;

// --- Blower/pump/compressor type registers ---

static constexpr uint16_t REG_BLOWER_TYPE = 404;
static constexpr uint16_t REG_PUMP_TYPE = 413;
static constexpr uint16_t REG_ENERGY_MONITOR = 412;    // 0=None, 1=Compressor Monitor, 2=Energy Monitor

// Blower type values
static constexpr uint16_t BLOWER_PSC = 0;
static constexpr uint16_t BLOWER_ECM_230 = 1;
static constexpr uint16_t BLOWER_ECM_277 = 2;
static constexpr uint16_t BLOWER_5SPD_460 = 3;

// --- Status registers ---

static constexpr uint16_t REG_COMPRESSOR_DELAY = 6;    // Compressor anti-short cycle delay (seconds; non-zero = waiting)
static constexpr uint16_t REG_LINE_VOLTAGE = 16;
static constexpr uint16_t REG_FP1_TEMP = 19;           // SIGNED_TENTHS - Cooling liquid line
static constexpr uint16_t REG_FP2_TEMP = 20;           // SIGNED_TENTHS - Air coil temp
static constexpr uint16_t REG_LAST_FAULT = 25;         // Bit 15 = lockout flag, bits 0-14 = fault code
static constexpr uint16_t REG_LAST_LOCKOUT = 26;
static constexpr uint16_t REG_OUTPUTS_AT_LOCKOUT = 27;  // Bitmask: system outputs at last lockout
static constexpr uint16_t REG_INPUTS_AT_LOCKOUT = 28;   // Bitmask: system inputs at last lockout
static constexpr uint16_t REG_SYSTEM_OUTPUTS = 30;     // Bitmask
static constexpr uint16_t REG_STATUS = 31;             // Bitmask: system inputs + LPS (0x80) + HPS (0x100)
static constexpr uint16_t REG_ECM_SPEED = 344;         // Current blower ECM speed
static constexpr uint16_t REG_ACTIVE_DEHUMIDIFY = 362; // Boolean: any non-zero value = active dehumidify
static constexpr uint16_t REG_TSTAT_AMBIENT = 502;      // SIGNED_TENTHS - Ambient temperature
static constexpr uint16_t REG_ENTERING_AIR_ABC = 567;   // SIGNED_TENTHS - Entering air (ABC fallback when AXB not AWL)

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

// System input bits (register 31)
static constexpr uint16_t INPUT_Y1 = 0x01;
static constexpr uint16_t INPUT_Y2 = 0x02;
static constexpr uint16_t INPUT_W = 0x04;
static constexpr uint16_t INPUT_O = 0x08;
static constexpr uint16_t INPUT_G = 0x10;
static constexpr uint16_t INPUT_DH_RH = 0x20;
static constexpr uint16_t INPUT_EMERGENCY_SHUTDOWN = 0x40;
static constexpr uint16_t INPUT_LPS = 0x80;              // Low Pressure Switch closed
static constexpr uint16_t INPUT_HPS = 0x100;             // High Pressure Switch closed
static constexpr uint16_t INPUT_LOAD_SHED = 0x200;

// Bitmask-to-string helpers for lockout diagnostic registers
struct BitLabel {
  uint16_t mask;
  const char *label;
};

static constexpr BitLabel OUTPUT_BITS[] = {
    {OUTPUT_CC, "CC"},
    {OUTPUT_CC2, "CC2"},
    {OUTPUT_RV, "RV"},
    {OUTPUT_BLOWER, "Blower"},
    {OUTPUT_EH1, "EH1"},
    {OUTPUT_EH2, "EH2"},
    {OUTPUT_ACCESSORY, "Accessory"},
    {OUTPUT_LOCKOUT, "Lockout"},
    {OUTPUT_ALARM, "Alarm"},
};
static constexpr size_t OUTPUT_BITS_SIZE = sizeof(OUTPUT_BITS) / sizeof(OUTPUT_BITS[0]);

static constexpr BitLabel INPUT_BITS[] = {
    {INPUT_Y1, "Y1"},
    {INPUT_Y2, "Y2"},
    {INPUT_W, "W"},
    {INPUT_O, "O"},
    {INPUT_G, "G"},
    {INPUT_DH_RH, "DH/RH"},
    {INPUT_EMERGENCY_SHUTDOWN, "Emergency Shutdown"},
    {INPUT_LPS, "LPS"},
    {INPUT_HPS, "HPS"},
    {INPUT_LOAD_SHED, "Load Shed"},
};
static constexpr size_t INPUT_BITS_SIZE = sizeof(INPUT_BITS) / sizeof(INPUT_BITS[0]);

// --- Thermostat registers (single zone, AWL) ---

static constexpr uint16_t REG_ENTERING_AIR = 740;       // SIGNED_TENTHS
static constexpr uint16_t REG_HUMIDITY = 741;            // UNSIGNED (%)
static constexpr uint16_t REG_OUTDOOR_TEMP = 742;        // SIGNED_TENTHS
static constexpr uint16_t REG_HEATING_SETPOINT = 745;    // TENTHS
static constexpr uint16_t REG_COOLING_SETPOINT = 746;    // TENTHS
static constexpr uint16_t REG_AMBIENT_TEMP = 747;        // SIGNED_TENTHS - may read 0 when mode is OFF; prefer REG_TSTAT_AMBIENT (502)

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
static constexpr uint16_t REG_COMPRESSOR_1_AMPS = 1107; // TENTHS - Compressor stage 1 current
static constexpr uint16_t REG_COMPRESSOR_2_AMPS = 1108; // TENTHS - Compressor stage 2 current

// Performance registers (AXB)
static constexpr uint16_t REG_LEAVING_AIR = 900;         // SIGNED_TENTHS
static constexpr uint16_t REG_LEAVING_WATER = 1110;      // SIGNED_TENTHS
static constexpr uint16_t REG_ENTERING_WATER = 1111;     // SIGNED_TENTHS
static constexpr uint16_t REG_HEATING_LIQUID_LINE = 1109; // SIGNED_TENTHS - Heating liquid line temperature
static constexpr uint16_t REG_REFRIG_LEAVING_AIR = 1112; // SIGNED_TENTHS - Leaving air temperature (refrigeration)
static constexpr uint16_t REG_SUCTION_TEMP = 1113;       // SIGNED_TENTHS
static constexpr uint16_t REG_DHW_TEMP = 1114;           // SIGNED_TENTHS
static constexpr uint16_t REG_DISCHARGE_PRESSURE = 1115; // TENTHS (psi)
static constexpr uint16_t REG_SUCTION_PRESSURE = 1116;   // TENTHS (psi)
static constexpr uint16_t REG_WATERFLOW = 1117;          // TENTHS (gpm)
static constexpr uint16_t REG_LOOP_PRESSURE = 1119;      // TENTHS (psi)
static constexpr uint16_t REG_SAT_EVAP_TEMP = 1124;      // SIGNED_TENTHS - Saturated evaporator temperature
static constexpr uint16_t REG_SUPERHEAT = 1125;          // SIGNED_TENTHS
static constexpr uint16_t REG_SAT_COND_TEMP = 1134;      // SIGNED_TENTHS - Saturated Condensor Discharge Temperature
static constexpr uint16_t REG_SUBCOOLING_HEAT = 1135;    // SIGNED_TENTHS - SubCooling (heating mode)
static constexpr uint16_t REG_SUBCOOLING_COOL = 1136;    // SIGNED_TENTHS - SubCooling (cooling mode)

// --- Power/energy registers ---

static constexpr uint16_t REG_COMPRESSOR_WATTS_HI = 1146;
static constexpr uint16_t REG_COMPRESSOR_WATTS_LO = 1147;
static constexpr uint16_t REG_BLOWER_WATTS_HI = 1148;
static constexpr uint16_t REG_BLOWER_WATTS_LO = 1149;
static constexpr uint16_t REG_AUX_HEAT_WATTS_HI = 1150;
static constexpr uint16_t REG_AUX_HEAT_WATTS_LO = 1151;
static constexpr uint16_t REG_TOTAL_WATTS_HI = 1152;
static constexpr uint16_t REG_TOTAL_WATTS_LO = 1153;
static constexpr uint16_t REG_HEAT_OF_EXTRACTION_HI = 1154; // INT32 (Btuh)
static constexpr uint16_t REG_HEAT_OF_EXTRACTION_LO = 1155;
static constexpr uint16_t REG_HEAT_OF_REJECTION_HI = 1156;  // INT32 (Btuh)
static constexpr uint16_t REG_HEAT_OF_REJECTION_LO = 1157;
static constexpr uint16_t REG_PUMP_WATTS_HI = 1164;
static constexpr uint16_t REG_PUMP_WATTS_LO = 1165;

// --- VS Drive registers ---

static constexpr uint16_t REG_VS_SPEED_DESIRED = 3000;
static constexpr uint16_t REG_VS_SPEED_ACTUAL = 3001;
static constexpr uint16_t REG_VS_SPEED_REQUESTED = 3027; // Compressor speed (requested by ABC)
static constexpr uint16_t REG_VS_DRIVE_STATUS = 3220;
static constexpr uint16_t REG_VS_EEV2_OPEN = 3808;       // EEV2 % open (VS systems)
static constexpr uint16_t REG_VS_INVERTER_TEMP = 3522;  // SIGNED_TENTHS
static constexpr uint16_t REG_VS_FAN_SPEED = 3524;
static constexpr uint16_t REG_VS_DISCHARGE_PRESS = 3322;  // TENTHS
static constexpr uint16_t REG_VS_SUCTION_PRESS = 3323;    // TENTHS
static constexpr uint16_t REG_VS_DISCHARGE_TEMP = 3325;   // SIGNED_TENTHS
static constexpr uint16_t REG_VS_COMP_AMBIENT_TEMP = 3326; // SIGNED_TENTHS - Compressor ambient temperature
static constexpr uint16_t REG_VS_DRIVE_TEMP = 3327;       // SIGNED_TENTHS
static constexpr uint16_t REG_VS_ENTERING_WATER_TEMP = 3330; // SIGNED_TENTHS - Entering water temperature
static constexpr uint16_t REG_VS_LINE_VOLTAGE = 3331;     // UNSIGNED (V)
static constexpr uint16_t REG_VS_THERMO_POWER = 3332;     // UNSIGNED (%)
static constexpr uint16_t REG_VS_COMP_WATTS_HI = 3422;    // UINT32 (W)
static constexpr uint16_t REG_VS_COMP_WATTS_LO = 3423;
static constexpr uint16_t REG_VS_SUPPLY_VOLTAGE_HI = 3424; // UINT32 (V)
static constexpr uint16_t REG_VS_SUPPLY_VOLTAGE_LO = 3425;
static constexpr uint16_t REG_VS_UDC_VOLTAGE = 3523;      // UNSIGNED (V)

// VS Drive refrigerant sensors (3900 range)
static constexpr uint16_t REG_VS_SUCTION_TEMP = 3903;     // SIGNED_TENTHS - VS Drive suction temperature
static constexpr uint16_t REG_VS_SAT_EVAP_DISCHARGE_TEMP = 3905; // SIGNED_TENTHS - VS Drive saturated evaporator discharge temp
static constexpr uint16_t REG_VS_SUPERHEAT_TEMP = 3906;   // SIGNED_TENTHS - VS Drive superheat temperature

// --- IZ2 zone registers ---

static constexpr uint16_t REG_IZ2_OUTDOOR_TEMP = 31003;  // SIGNED_TENTHS - IZ2 outdoor temperature
static constexpr uint16_t REG_IZ2_DEMAND = 31005;        // UNSIGNED - High byte: fan demand, Low byte: unit demand

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

// --- Register query breakpoints ---
// The ABC requires separate queries across these boundaries
static constexpr uint16_t REGISTER_BREAKPOINT_1 = 12100;
static constexpr uint16_t REGISTER_BREAKPOINT_2 = 12500;

// --- Fault codes ---

struct FaultInfo {
  uint8_t code;
  const char *description;
};

static constexpr FaultInfo FAULT_TABLE[] = {
    // ABC/AXB faults
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
    {18, "Non-Critical Communications Error"},
    {19, "Critical Communications Error"},
    {21, "Low Loop Pressure"},
    {22, "Communicating ECM Error"},
    {23, "HA Alarm 1"},
    {24, "HA Alarm 2"},
    {25, "AxbEev Error"},
    // VS Drive faults
    {41, "High Drive Temp"},
    {42, "High Discharge Temp"},
    {43, "Low Suction Pressure"},
    {44, "Low Condensing Pressure"},
    {45, "High Condensing Pressure"},
    {46, "Output Power Limit"},
    {47, "EEV ID Comm Error"},
    {48, "EEV OD Comm Error"},
    {49, "Cabinet Temperature Sensor"},
    {51, "Discharge Temp Sensor"},
    {52, "Suction Pressure Sensor"},
    {53, "Condensing Pressure Sensor"},
    {54, "Low Supply Voltage"},
    {55, "Out of Envelope"},
    {56, "Drive Over Current"},
    {57, "Drive Over/Under Voltage"},
    {58, "High Drive Temp"},
    {59, "Internal Drive Error"},
    {61, "Multiple Safe Mode"},
    // EEV2 faults
    {71, "Loss of Charge"},
    {72, "Suction Temperature Sensor"},
    {73, "Leaving Air Temperature Sensor"},
    {74, "Maximum Operating Pressure"},
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
      {412, 2},      // Energy monitor type, pump type
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

// --- IZ2 zone register extraction helpers ---

// Extract mode from zone_configuration2 register
// Uses 3-bit mask to support E-Heat (mode 4) detection on IZ2 zones.
// DO NOT change to 0x03 — the 3-bit mask is required for E-Heat and was
// confirmed working in commit d3e389c.
inline uint8_t iz2_extract_mode(uint16_t config2) {
  return (config2 >> 8) & 0x07;
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
