# Protocol & Register Reference

Complete register map for the ESPHome WaterFurnace component, verified
register-by-register against the [waterfurnace_aurora](https://github.com/ccutrer/waterfurnace_aurora)
Ruby reference project.

Reference files examined:
- `lib/aurora/registers.rb` — all register definitions, converters, formats
- `lib/aurora/abc_client.rb` — core polling, component detection, mode logic
- `lib/aurora/compressor.rb` — GenericCompressor and VSDrive
- `lib/aurora/blower.rb` — PSC, FiveSpeed, ECM
- `lib/aurora/pump.rb` — GenericPump, VSPump
- `lib/aurora/humidistat.rb` — humidity control
- `lib/aurora/dhw.rb` — domestic hot water
- `lib/aurora/thermostat.rb` / `iz2_zone.rb` — zone control

---

## Register Address Verification

All sensor register addresses match the reference exactly.

| ESPHome Sensor | Register | Reference Name | Match |
|---|---|---|---|
| entering_water_temperature | 1111 | Entering Water | Yes |
| leaving_water_temperature | 1110 | Leaving Water | Yes |
| outdoor_temperature | 742 | Outdoor Temperature | Yes |
| entering_air_temperature | 740 | Entering Air | Yes |
| leaving_air_temperature | 900 | Leaving Air | Yes |
| suction_temperature | 1113 | Suction Temperature | Yes |
| dhw_temperature | 1114 | DHW Temperature | Yes |
| discharge_pressure | 1115 | Discharge Pressure | Yes |
| suction_pressure | 1116 | Suction Pressure | Yes |
| loop_pressure | 1119 | Loop Pressure | Yes |
| waterflow | 1117 | Waterflow | Yes |
| compressor_power | 1146 | Compressor Watts | Yes |
| blower_power | 1148 | Blower Watts | Yes |
| aux_heat_power | 1150 | Aux Watts | Yes |
| total_power | 1152 | Total Watts | Yes |
| pump_power | 1164 | Pump Watts | Yes |
| line_voltage | 16 | Line Voltage | Yes |
| compressor_amps | 1107 | Compressor 1 Amps | Yes |
| blower_amps | 1105 | Blower Amps | Yes |
| aux_heat_amps | 1106 | Aux Amps | Yes |
| compressor_2_amps | 1108 | Compressor 2 Amps | Yes |
| relative_humidity | 741 | Relative Humidity | Yes |
| compressor_speed | 3001 | Compressor Speed Actual | Yes |
| heat_of_extraction | 1154 | Heat of Extraction | Yes |
| heat_of_rejection | 1156 | Heat of Rejection | Yes |
| ambient_temperature | 502 | Ambient Temperature | Yes |
| fp1_temperature | 19 | Cooling Liquid Line Temperature (FP1) | Yes |
| fp2_temperature | 20 | Air Coil Temperature (FP2) | Yes |
| sat_evap_temperature | 1124 | Saturated Evaporator Temperature | Yes |
| superheat | 1125 | SuperHeat | Yes |
| sat_cond_temperature | 1134 | Saturated Condensor Discharge Temperature | Yes |
| subcooling_heating | 1135 | SubCooling (Heating) | Yes |
| subcooling_cooling | 1136 | SubCooling (Cooling) | Yes |
| heating_liquid_line_temperature | 1109 | Heating Liquid Line Temperature | Yes |
| refrigerant_leaving_air_temperature | 1112 | Leaving Air Temperature | Yes |
| ecm_speed | 344 | ECM Speed | Yes |
| vs_drive_temperature | 3327 | VS Drive Temperature | Yes |
| vs_line_voltage | 3331 | VS Drive Line Voltage | Yes |
| vs_thermo_power | 3332 | VS Drive Thermo Power | Yes |
| vs_compressor_power | 3422 | VS Drive Compressor Power | Yes |
| vs_supply_voltage | 3424 | VS Drive Supply Voltage | Yes |
| vs_udc_voltage | 3523 | VS Drive UDC Voltage | Yes |
| vs_compressor_speed_requested | 3027 | Compressor Speed | Yes |
| vs_eev2_open | 3808 | VS Drive EEV2 % Open | Yes |
| vs_discharge_pressure | 3322 | VS Drive Discharge Pressure | Yes |
| vs_suction_pressure | 3323 | VS Drive Suction Pressure | Yes |
| vs_discharge_temperature | 3325 | VS Drive Discharge Temperature | Yes |
| vs_compressor_ambient_temperature | 3326 | VS Drive Compressor Ambient Temperature | Yes |
| vs_entering_water_temperature | 3330 | VS Drive Entering Water Temperature | Yes |
| vs_inverter_temperature | 3522 | VS Drive Inverter Temperature | Yes |
| vs_fan_speed | 3524 | VS Drive Fan Speed | Yes |
| vs_suction_temperature | 3903 | VS Drive Suction Temperature | Yes |
| vs_sat_evap_discharge_temperature | 3905 | VS Drive Saturated Evaporator Discharge Temperature | Yes |
| vs_superheat_temperature | 3906 | VS Drive SuperHeat Temperature | Yes |
| iz2_outdoor_temperature | 31003 | IZ2 Outdoor Temperature | Yes |
| iz2_demand | 31005 | IZ2 Demand | Yes |

---

## Register Data Types / Scaling

All conversion types match the reference's `REGISTER_CONVERTERS` mapping.

| ESPHome Type | Reference Converter | Registers Verified |
|---|---|---|
| signed_tenths | TO_SIGNED_TENTHS | 19, 20, 502, 567, 740, 742, 747, 900, 1109-1114, 1124-1125, 1134-1136, 3325-3327, 3330, 3522, 3903, 3905-3906, 31003 |
| tenths | TO_TENTHS | 401, 1105-1108, 1115-1117, 1119, 3322-3323 |
| unsigned | (no converter) | 16, 344, 741, 3001, 3027, 3331, 3332, 3523, 3524, 3808, 31005 |
| uint32 | to_uint32 | 1146, 1148, 1150, 1152, 1164, 3422, 3424 |
| int32 | to_int32 | 1154, 1156 |
| hundredths | TO_HUNDREDTHS | 2 (ABC version, setup only) |

---

## Capability Gating

Traced every capability assignment to the reference's component architecture.

| ESPHome Capability | Reference Condition | Source | Match |
|---|---|---|---|
| `awl_thermostat` | `awl_thermostat?` (thermostat v >= 3.0) | abc_client.rb:375-376 | Yes |
| `awl_axb` | `awl_axb?` (AXB v >= 2.0) | abc_client.rb:385-387 | Yes |
| `awl_communicating` | `awl_communicating?` (tstat v>=3.0 OR iz2 v>=2.0) | abc_client.rb:370-372 | Yes |
| `axb` | `axb?` / `performance_monitoring?` | abc_client.rb:333-335, 350 | Yes |
| `refrigeration` | `refrigeration_monitoring?` (energy_monitor >= 1) | abc_client.rb:337-339, compressor.rb:31 | Yes |
| `energy` | `energy_monitoring?` (energy_monitor == 2) | abc_client.rb:341-343, :205 | Yes |
| `vs_drive` | VSDrive class / program name check | abc_client.rb:175, 215 | Yes |
| `iz2` | `iz2? && iz2_version >= 2.0` | abc_client.rb:165-166 | Yes |

Active dehumidify (reg 362) gated to `vs_drive`: Confirmed correct per
`abc_client.rb:215` — `@registers_to_read.push(362) if compressor.is_a?(Compressor::VSDrive)`.

---

## Bitmask Definitions

### System Outputs (Register 30)

| Bit | ESPHome | Reference | Match |
|---|---|---|---|
| 0x01 | OUTPUT_CC | :cc | Yes |
| 0x02 | OUTPUT_CC2 | :cc2 | Yes |
| 0x04 | OUTPUT_RV | :rv | Yes |
| 0x08 | OUTPUT_BLOWER | :blower | Yes |
| 0x10 | OUTPUT_EH1 | :eh1 | Yes |
| 0x20 | OUTPUT_EH2 | :eh2 | Yes |
| 0x200 | OUTPUT_ACCESSORY | :accessory | Yes |
| 0x400 | OUTPUT_LOCKOUT | :lockout | Yes |
| 0x800 | OUTPUT_ALARM | :alarm | Yes |

### AXB Outputs (Register 1104)

| Bit | ESPHome | Reference | Match |
|---|---|---|---|
| 0x01 | axb_dhw | :dhw | Yes |
| 0x02 | axb_loop_pump | :loop_pump | Yes |
| 0x04 | axb_diverting_valve | :diverting_valve | Yes |
| 0x08 | axb_dehumidifier | :dehumidifier_reheat | Yes |
| 0x10 | axb_accessory2 | :accessory2 | Yes |

### System Inputs (Register 31)

| Bit | ESPHome | Reference | Match |
|---|---|---|---|
| 0x01 | INPUT_Y1 | :y1 | Yes |
| 0x02 | INPUT_Y2 | :y2 | Yes |
| 0x04 | INPUT_W | :w | Yes |
| 0x08 | INPUT_O | :o | Yes |
| 0x10 | INPUT_G | :g | Yes |
| 0x20 | INPUT_DH_RH | :dh_rh | Yes |
| 0x40 | INPUT_EMERGENCY_SHUTDOWN | :emergency_shutdown | Yes |
| 0x80 | INPUT_LPS | :lps (in status()) | Yes |
| 0x100 | INPUT_HPS | :hps (in status()) | Yes |
| 0x200 | INPUT_LOAD_SHED | :load_shed | Yes |

---

## Fault Codes

All 41 fault codes match the reference's `FAULTS` hash exactly.

Codes present: 1-5, 7-19, 21-25, 41-49, 51-59, 61, 71-74, 99.
No codes missing, no extra codes.

---

## IZ2 Zone Parsing

| Function | ESPHome | Reference | Match |
|---|---|---|---|
| Cooling SP | `((config1 & 0x7E) >> 1) + 36` | `((value & 0x7e) >> 1) + 36` | Yes |
| Heating SP | `((carry << 5) \| ((config2 & 0xF800) >> 11)) + 36` | Same | Yes |
| Fan mode | `0x80=continuous, 0x100=intermittent, else=auto` | Same | Yes |
| Zone mode | `(config2 >> 8) & 0x07` | `(v >> 8) & 0x03` | **ESPHome improved** |

ESPHome uses a 3-bit mask (0x07) for zone mode extraction vs the reference's 2-bit
mask (0x03). The reference's mask cannot represent E-Heat (mode 4 = `0b100`).
ESPHome's fix is correct and was confirmed in commit d3e389c.

---

## Protocol

| Detail | ESPHome | Reference | Match |
|---|---|---|---|
| Func 65 (read ranges) | FUNC_READ_RANGES = 65 | Custom 'A' (0x41) | Yes |
| Func 66 (read individual) | FUNC_READ_REGISTERS = 66 | Custom 'B' (0x42) | Yes |
| Func 67 (write multi) | FUNC_WRITE_REGISTERS = 67 | (implied) | Yes |
| Func 6 (write single) | FUNC_WRITE_SINGLE = 6 | Standard ModBus | Yes |
| Breakpoints | 12100, 12500 | 12100, 12500 | Yes |
| Baud / parity | 19200, even | 19200, even | Yes |
| Slave address | 1 | 1 | Yes |
| Max regs/request | 100 | 100 | Yes |

---

## Component Detection

| Register | ESPHome | Reference | Match |
|---|---|---|---|
| 800 (thermostat) | check_component() | `!= 3` | Yes |
| 806 (AXB) | check_component() | `!= 3` | Yes |
| 812 (IZ2) | check_component() | `!= 3` | Yes |
| 815 (AOC) | check_component() | `!= 3` | Yes |
| 818 (MOC) | check_component() | `!= 3` | Yes |
| 824 (EEV2) | check_component() | `!= 3` | Yes |
| 827 (AWL) | check_component() | `!= 3` | Yes |
| AWL thermostat | v >= 3.0 | `thermostat_version >= 3.0` | Yes |
| AWL AXB | v >= 2.0 | `axb_version >= 2.0` | Yes |
| AWL IZ2 | v >= 2.0 | `iz2_version >= 2.0` | Yes |
| VS drive | program in [ABCVSP, ABCVSPR, ABCSPLVS] | Same | Yes |

---

## Entering Air Fallback

Entering air temperature is available from two registers depending on hardware:
- **Register 740** — available on AWL AXB systems (AXB v2.0+)
- **Register 567** — available on all systems (ABC board fallback)

On non-AWL-AXB systems, register 740 is not populated. ESPHome polls register 567
instead and forwards its value to all register 740 listeners, so sensors always
subscribe to 740 regardless of hardware.

Reference (`abc_client.rb:200`): `@entering_air_register = awl_axb? ? 740 : 567`

---

## System Mode Logic

ESPHome's `compute_system_mode_()` matches `abc_client.rb:251-271`:

| Priority | ESPHome | Reference |
|---|---|---|
| 1 | OUTPUT_LOCKOUT set -> "Lockout" | `:lockout` |
| 2 | active_dehumidify != 0 -> "Dehumidify" | `registers[362]` -> `:dehumidify` |
| 3 | CC/CC2 + RV -> "Cooling" | `:cooling` |
| 4 | CC/CC2 + EH1/EH2 -> "Heating with Aux" | `:heating_with_aux` |
| 5 | CC/CC2 -> "Heating" | `:heating` |
| 6 | EH1/EH2 only -> "Emergency Heat" | `:emergency_heat` |
| 7 | BLOWER -> "Fan Only" | `:blower` |
| 8 | compressor_delay != 0 -> "Waiting" | `!registers[6].zero?` -> `:waiting` |
| 9 | else -> "Standby" | `:standby` |

---

## Capability Gating Detail

Per-sensor capability verification against the reference's component architecture.

| ESPHome Sensor | Register | ESPHome Capability | Reference Source | Match |
|---|---|---|---|---|
| entering_water_temperature | 1111 | axb | `performance_monitoring?` (= `axb?`) | Yes |
| leaving_water_temperature | 1110 | axb | `performance_monitoring?` | Yes |
| outdoor_temperature | 742 | awl_communicating | `awl_communicating?` reads 741..742 | Yes |
| entering_air_temperature | 740 | none | Always read (567/740 fallback) | Yes |
| leaving_air_temperature | 900 | awl_axb | `awl_axb?` reads 900 | Yes |
| suction_temperature | 1113 | axb | AXB performance range | Yes |
| dhw_temperature | 1114 | axb | DHW component (requires AXB) | Yes |
| discharge_pressure | 1115 | axb | AXB performance range | Yes |
| suction_pressure | 1116 | axb | AXB performance range | Yes |
| loop_pressure | 1119 | axb | AXB performance range | Yes |
| waterflow | 1117 | axb | Pump component (requires AXB) `pump.rb:17` | Yes |
| compressor_power | 1146 | energy | `energy_monitoring?` `compressor.rb:32` | Yes |
| blower_power | 1148 | energy | `energy_monitoring?` `blower.rb:17` | Yes |
| aux_heat_power | 1150 | energy | `energy_monitoring?` `abc_client.rb:205` | Yes |
| total_power | 1152 | energy | `energy_monitoring?` `abc_client.rb:205` | Yes |
| pump_power | 1164 | energy | `energy_monitoring?` `pump.rb:18` | Yes |
| line_voltage | 16 | energy | `energy_monitoring?` `abc_client.rb:205` | Yes |
| compressor_amps | 1107 | axb | AXB performance range | Yes |
| blower_amps | 1105 | axb | AXB performance range | Yes |
| aux_heat_amps | 1106 | axb | AXB performance range | Yes |
| compressor_2_amps | 1108 | axb | AXB performance range | Yes |
| relative_humidity | 741 | awl_communicating | `awl_communicating?` reads 741..742 | Yes |
| compressor_speed | 3001 | vs_drive | VSDrive `compressor.rb:85` | Yes |
| heat_of_extraction | 1154 | refrigeration | `refrigeration_monitoring?` `compressor.rb:31` | Yes |
| heat_of_rejection | 1156 | refrigeration | `refrigeration_monitoring?` `compressor.rb:31` | Yes |
| ambient_temperature | 502 | awl_thermostat | `awl_thermostat?` reads 502 | Yes |
| fp1_temperature | 19 | none | Always read `abc_client.rb:201` | Yes |
| fp2_temperature | 20 | none | Always read `abc_client.rb:201` | Yes |
| sat_evap_temperature | 1124 | refrigeration | `refrigeration_monitoring?` | Yes |
| superheat | 1125 | refrigeration | `refrigeration_monitoring?` | Yes |
| sat_cond_temperature | 1134 | refrigeration | `refrigeration_monitoring?` `compressor.rb:31` | Yes |
| heating_liquid_line_temperature | 1109 | refrigeration | `refrigeration_monitoring?` `compressor.rb:31` | Yes |
| refrigerant_leaving_air_temperature | 1112 | axb | AXB performance range | Yes |
| subcooling_heating | 1135 | vs_drive | VSDrive `compressor.rb:85` | Yes |
| subcooling_cooling | 1136 | vs_drive | VSDrive `compressor.rb:85` | Yes |
| ecm_speed | 344 | none | Always read `abc_client.rb:201` | Yes |
| vs_drive_temperature | 3327 | vs_drive | VSDrive `compressor.rb:85` | Yes |
| vs_line_voltage | 3331 | vs_drive | VS Drive register range | Yes |
| vs_thermo_power | 3332 | vs_drive | VS Drive register range | Yes |
| vs_compressor_power | 3422 | vs_drive | VS Drive register range | Yes |
| vs_supply_voltage | 3424 | vs_drive | VS Drive register range | Yes |
| vs_udc_voltage | 3523 | vs_drive | VS Drive register range | Yes |
| vs_compressor_speed_requested | 3027 | vs_drive | VS Drive register range | Yes |
| vs_eev2_open | 3808 | vs_drive | VSDrive `compressor.rb:85` | Yes |
| vs_discharge_pressure | 3322 | vs_drive | VSDrive `compressor.rb:85` | Yes |
| vs_suction_pressure | 3323 | vs_drive | VSDrive `compressor.rb:85` | Yes |
| vs_discharge_temperature | 3325 | vs_drive | VSDrive `compressor.rb:85` | Yes |
| vs_compressor_ambient_temperature | 3326 | vs_drive | VSDrive `compressor.rb:85` | Yes |
| vs_entering_water_temperature | 3330 | vs_drive | VS Drive register range | Yes |
| vs_inverter_temperature | 3522 | vs_drive | VSDrive `compressor.rb:85` | Yes |
| vs_fan_speed | 3524 | vs_drive | VSDrive `compressor.rb:85` | Yes |
| vs_suction_temperature | 3903 | vs_drive | VSDrive `compressor.rb:85` | Yes |
| vs_sat_evap_discharge_temperature | 3905 | vs_drive | VSDrive `compressor.rb:85` | Yes |
| vs_superheat_temperature | 3906 | vs_drive | VSDrive `compressor.rb:85` | Yes |
| iz2_outdoor_temperature | 31003 | iz2 | IZ2 zone registers | Yes |
| iz2_demand | 31005 | iz2 | IZ2 zone registers | Yes |

---

## Missing Sensors (Polled by Reference, Not Polled by ESPHome)

Registers that the reference's components explicitly include in `registers_to_read`
but ESPHome does not poll or expose. The ESPHome component is listener-driven — it
only polls registers that have a configured entity (sensor, text sensor, etc.), so
these registers are never read from the device.

### VS Drive — `compressor.rb:85`

| Register | Name | Type | Capability | Notes |
|---|---|---|---|---|
| 3000 | VS Compressor Speed Desired | unsigned | vs_drive | Reference reads both 3000 (desired) and 3001 (actual). ESPHome only has 3001. Useful for monitoring ramp behavior. |

### VS Drive + IZ2 — `compressor.rb:86`

| Register | Name | Type | Capability | Notes |
|---|---|---|---|---|
| 564 | IZ2 Compressor Speed Desired | unsigned | vs_drive + iz2 | Would need combined capability or new gate. Read when VSDrive + IZ2. |

### ECM Blower — `blower.rb:66-67`

| Register | Name | Type | Capability | Notes |
|---|---|---|---|---|
| 340 | Blower Only Speed | unsigned | none (ECM systems) | Writable config, 1-12. Could be sensor or number entity. |
| 341 | Lo Compressor ECM Speed | unsigned | none (ECM systems) | Writable config, 1-12. |
| 342 | Hi Compressor ECM Speed | unsigned | none (ECM systems) | Writable config, 1-12. |
| 347 | Aux Heat ECM Speed | unsigned | none (ECM systems) | Writable config, 1-12. |
| 565 | IZ2 Blower % Desired | unsigned | iz2 | Reference converts: 1=25%, 2=40%, 3=55%, 4=70%, 5=85%, 6=100%. |

### VS Pump — `pump.rb:33-34`

| Register | Name | Type | Capability | Notes |
|---|---|---|---|---|
| 321 | VS Pump Min | unsigned (%) | (needs vs_pump gate or axb) | Config register. |
| 322 | VS Pump Max | unsigned (%) | (needs vs_pump gate or axb) | Config register. |
| 325 | VS Pump Output | unsigned (%) | (needs vs_pump + awl_axb) | Current pump speed. Only valid when AWL AXB (`pump.rb:42`). |

### Core — `abc_client.rb:201`

| Register | Name | Type | Capability | Notes |
|---|---|---|---|---|
| 112 | Line Voltage Setting | unsigned (V) | none | Configured line voltage (not actual — that's reg 16). Always read by reference. |

### Humidistat — `humidistat.rb:34-41`

| Register | Name | Type | Capability | Notes |
|---|---|---|---|---|
| 12309 | De/Humidifier Mode | bitmask | awl_communicating (no IZ2) | 0x4000=auto dehumidification, 0x8000=auto humidification. Would need its own component. |
| 12310 | De/Humidifier Setpoints | packed | awl_communicating (no IZ2) | High byte=humidification target (15-50%), low byte=dehumidification target (35-65%). |
| 31109 | De/Humidifier Mode | bitmask | iz2 | IZ2 variant of 12309. |
| 31110 | De/Humidifier Setpoints | packed | iz2 | IZ2 variant of 12310. |

---

## Named Registers (Not Polled by Reference or ESPHome)

Registers that have names and converters in `registers.rb` but aren't in any
component's `registers_to_read` in the reference, and are also not polled by
ESPHome. These are defined but unused by both projects.

| Register | Name | Type | Notes |
|---|---|---|---|
| 26 | Last Lockout | fault code | High bit = locked out; bits 0-14 = fault code. Could be a text sensor alongside current_fault. |
| 901 | Suction Pressure (EEV) | tenths, psi | Alternative source to 1116 (AXB) and 3323 (VS Drive). |
| 903 | SuperHeat Temperature (EEV) | signed_tenths, F | Alternative source to 1125 (AXB) and 3906 (VS Drive). |
| 908 | EEV Open % | unsigned, % | Non-VS version of 3808. From AXB EEV module. |
| 1126 | Vapor Injector Open % | unsigned, % | Named in reference, format `%d%%`. |
| 3904 | VS Drive Leaving Air Temperature? | unknown | Marked with "?" in reference — uncertain data. |
| 209 | Compressor Ambient Temperature | unknown format | Low-range alternative to 3326. Reference reads both; format unclear ("I can't figure out how this number is represented"). |

---

## ESPHome Improvements Over Reference

1. **IZ2 zone mode 3-bit mask** — ESPHome uses `(config2 >> 8) & 0x07` vs reference's
   `& 0x03`. The reference cannot detect E-Heat (mode 4) on IZ2 zones. This is a bug
   in `registers.rb:466` (`zone_configuration2`).

2. **Entering air forwarding listener** — On non-AWL-AXB systems, register 740 is
   not populated; entering air temperature comes from register 567 instead. ESPHome
   reads 567 and forwards its value to all register 740 listeners, so sensors always
   subscribe to 740 regardless of hardware configuration.

3. **Sentinel value handling** — ESPHome converts -999.9 and 999.9 to NaN, which the
   reference doesn't do (it passes through raw converted values).
