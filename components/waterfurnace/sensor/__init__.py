import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_PRESSURE,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_HUMIDITY,
    STATE_CLASS_MEASUREMENT,
    UNIT_WATT,
    UNIT_VOLT,
    UNIT_AMPERE,
    UNIT_PERCENT,
)
from .. import waterfurnace_ns, WaterFurnace, CONF_WATERFURNACE_ID, WATERFURNACE_CLIENT_SCHEMA

DEPENDENCIES = ["waterfurnace"]

WaterFurnaceSensor = waterfurnace_ns.class_(
    "WaterFurnaceSensor", sensor.Sensor, cg.Component
)

UNIT_PSI = "psi"
UNIT_GPM = "gpm"
UNIT_BTU_H = "BTU/h"
UNIT_FAHRENHEIT = "°F"

# Sensor configuration keys
CONF_ENTERING_WATER_TEMPERATURE = "entering_water_temperature"
CONF_LEAVING_WATER_TEMPERATURE = "leaving_water_temperature"
CONF_OUTDOOR_TEMPERATURE = "outdoor_temperature"
CONF_ENTERING_AIR_TEMPERATURE = "entering_air_temperature"
CONF_LEAVING_AIR_TEMPERATURE = "leaving_air_temperature"
CONF_SUCTION_TEMPERATURE = "suction_temperature"
CONF_DHW_TEMPERATURE = "dhw_temperature"
CONF_DISCHARGE_PRESSURE = "discharge_pressure"
CONF_SUCTION_PRESSURE = "suction_pressure"
CONF_LOOP_PRESSURE = "loop_pressure"
CONF_WATERFLOW = "waterflow"
CONF_COMPRESSOR_POWER = "compressor_power"
CONF_BLOWER_POWER = "blower_power"
CONF_AUX_HEAT_POWER = "aux_heat_power"
CONF_TOTAL_POWER = "total_power"
CONF_PUMP_POWER = "pump_power"
CONF_LINE_VOLTAGE = "line_voltage"
CONF_COMPRESSOR_AMPS = "compressor_amps"
CONF_BLOWER_AMPS = "blower_amps"
CONF_RELATIVE_HUMIDITY = "relative_humidity"
CONF_COMPRESSOR_SPEED = "compressor_speed"
CONF_HEAT_OF_EXTRACTION = "heat_of_extraction"
CONF_HEAT_OF_REJECTION = "heat_of_rejection"
CONF_AMBIENT_TEMPERATURE = "ambient_temperature"
CONF_FP1_TEMPERATURE = "fp1_temperature"
CONF_FP2_TEMPERATURE = "fp2_temperature"
CONF_SAT_EVAP_TEMPERATURE = "sat_evap_temperature"
CONF_SUPERHEAT = "superheat"
CONF_VS_DRIVE_TEMPERATURE = "vs_drive_temperature"
CONF_VS_LINE_VOLTAGE = "vs_line_voltage"
CONF_VS_THERMO_POWER = "vs_thermo_power"
CONF_VS_COMPRESSOR_POWER = "vs_compressor_power"
CONF_VS_SUPPLY_VOLTAGE = "vs_supply_voltage"
CONF_VS_UDC_VOLTAGE = "vs_udc_voltage"
CONF_VS_COMPRESSOR_SPEED_REQUESTED = "vs_compressor_speed_requested"
CONF_VS_EEV2_OPEN = "vs_eev2_open"
CONF_HEATING_LIQUID_LINE_TEMPERATURE = "heating_liquid_line_temperature"
CONF_REFRIGERANT_LEAVING_AIR_TEMPERATURE = "refrigerant_leaving_air_temperature"
CONF_AUX_HEAT_AMPS = "aux_heat_amps"
CONF_COMPRESSOR_2_AMPS = "compressor_2_amps"
CONF_SAT_COND_TEMPERATURE = "sat_cond_temperature"
CONF_SUBCOOLING_HEATING = "subcooling_heating"
CONF_SUBCOOLING_COOLING = "subcooling_cooling"
CONF_ECM_SPEED = "ecm_speed"
CONF_VS_DISCHARGE_PRESSURE = "vs_discharge_pressure"
CONF_VS_SUCTION_PRESSURE = "vs_suction_pressure"
CONF_VS_DISCHARGE_TEMPERATURE = "vs_discharge_temperature"
CONF_VS_COMPRESSOR_AMBIENT_TEMPERATURE = "vs_compressor_ambient_temperature"
CONF_VS_ENTERING_WATER_TEMPERATURE = "vs_entering_water_temperature"
CONF_VS_INVERTER_TEMPERATURE = "vs_inverter_temperature"
CONF_VS_FAN_SPEED = "vs_fan_speed"
CONF_VS_SUCTION_TEMPERATURE = "vs_suction_temperature"
CONF_VS_SAT_EVAP_DISCHARGE_TEMPERATURE = "vs_sat_evap_discharge_temperature"
CONF_VS_SUPERHEAT_TEMPERATURE = "vs_superheat_temperature"
CONF_IZ2_OUTDOOR_TEMPERATURE = "iz2_outdoor_temperature"
CONF_IZ2_DEMAND = "iz2_demand"

# Register address, register type, is_32bit, capability
# register_type: "signed_tenths", "tenths", "unsigned", "uint32", "int32"
# capability: "none", "awl_thermostat", "awl_axb", "awl_communicating",
#             "axb", "refrigeration", "energy", "vs_drive", "iz2"
SENSOR_TYPES = {
    CONF_ENTERING_WATER_TEMPERATURE: (1111, "signed_tenths", False, "axb"),
    CONF_LEAVING_WATER_TEMPERATURE: (1110, "signed_tenths", False, "axb"),
    CONF_OUTDOOR_TEMPERATURE: (742, "signed_tenths", False, "awl_communicating"),
    CONF_ENTERING_AIR_TEMPERATURE: (740, "signed_tenths", False, "none"),
    CONF_LEAVING_AIR_TEMPERATURE: (900, "signed_tenths", False, "awl_axb"),
    CONF_SUCTION_TEMPERATURE: (1113, "signed_tenths", False, "axb"),
    CONF_DHW_TEMPERATURE: (1114, "signed_tenths", False, "axb"),
    CONF_DISCHARGE_PRESSURE: (1115, "tenths", False, "axb"),
    CONF_SUCTION_PRESSURE: (1116, "tenths", False, "axb"),
    CONF_LOOP_PRESSURE: (1119, "tenths", False, "axb"),
    CONF_WATERFLOW: (1117, "tenths", False, "axb"),
    CONF_COMPRESSOR_POWER: (1146, "uint32", True, "energy"),
    CONF_BLOWER_POWER: (1148, "uint32", True, "energy"),
    CONF_AUX_HEAT_POWER: (1150, "uint32", True, "energy"),
    CONF_TOTAL_POWER: (1152, "uint32", True, "energy"),
    CONF_PUMP_POWER: (1164, "uint32", True, "energy"),
    CONF_LINE_VOLTAGE: (16, "unsigned", False, "energy"),
    CONF_COMPRESSOR_AMPS: (1107, "tenths", False, "axb"),
    CONF_BLOWER_AMPS: (1105, "tenths", False, "axb"),
    CONF_RELATIVE_HUMIDITY: (741, "unsigned", False, "awl_communicating"),
    CONF_COMPRESSOR_SPEED: (3001, "unsigned", False, "vs_drive"),
    CONF_HEAT_OF_EXTRACTION: (1154, "int32", True, "refrigeration"),
    CONF_HEAT_OF_REJECTION: (1156, "int32", True, "refrigeration"),
    CONF_AMBIENT_TEMPERATURE: (502, "signed_tenths", False, "awl_thermostat"),
    CONF_FP1_TEMPERATURE: (19, "signed_tenths", False, "none"),
    CONF_FP2_TEMPERATURE: (20, "signed_tenths", False, "none"),
    CONF_SAT_EVAP_TEMPERATURE: (1124, "signed_tenths", False, "refrigeration"),
    CONF_SUPERHEAT: (1125, "signed_tenths", False, "refrigeration"),
    CONF_VS_DRIVE_TEMPERATURE: (3327, "signed_tenths", False, "vs_drive"),
    CONF_VS_LINE_VOLTAGE: (3331, "unsigned", False, "vs_drive"),
    CONF_VS_THERMO_POWER: (3332, "unsigned", False, "vs_drive"),
    CONF_VS_COMPRESSOR_POWER: (3422, "uint32", True, "vs_drive"),
    CONF_VS_SUPPLY_VOLTAGE: (3424, "uint32", True, "vs_drive"),
    CONF_VS_UDC_VOLTAGE: (3523, "unsigned", False, "vs_drive"),
    CONF_VS_COMPRESSOR_SPEED_REQUESTED: (3027, "unsigned", False, "vs_drive"),
    CONF_VS_EEV2_OPEN: (3808, "unsigned", False, "vs_drive"),
    CONF_VS_DISCHARGE_PRESSURE: (3322, "tenths", False, "vs_drive"),
    CONF_VS_SUCTION_PRESSURE: (3323, "tenths", False, "vs_drive"),
    CONF_VS_DISCHARGE_TEMPERATURE: (3325, "signed_tenths", False, "vs_drive"),
    CONF_VS_COMPRESSOR_AMBIENT_TEMPERATURE: (3326, "signed_tenths", False, "vs_drive"),
    CONF_VS_ENTERING_WATER_TEMPERATURE: (3330, "signed_tenths", False, "vs_drive"),
    CONF_VS_INVERTER_TEMPERATURE: (3522, "signed_tenths", False, "vs_drive"),
    CONF_VS_FAN_SPEED: (3524, "unsigned", False, "vs_drive"),
    CONF_VS_SUCTION_TEMPERATURE: (3903, "signed_tenths", False, "vs_drive"),
    CONF_VS_SAT_EVAP_DISCHARGE_TEMPERATURE: (3905, "signed_tenths", False, "vs_drive"),
    CONF_VS_SUPERHEAT_TEMPERATURE: (3906, "signed_tenths", False, "vs_drive"),
    CONF_HEATING_LIQUID_LINE_TEMPERATURE: (1109, "signed_tenths", False, "refrigeration"),
    CONF_REFRIGERANT_LEAVING_AIR_TEMPERATURE: (1112, "signed_tenths", False, "axb"),
    CONF_AUX_HEAT_AMPS: (1106, "tenths", False, "axb"),
    CONF_COMPRESSOR_2_AMPS: (1108, "tenths", False, "axb"),
    CONF_SAT_COND_TEMPERATURE: (1134, "signed_tenths", False, "refrigeration"),
    CONF_SUBCOOLING_HEATING: (1135, "signed_tenths", False, "vs_drive"),
    CONF_SUBCOOLING_COOLING: (1136, "signed_tenths", False, "vs_drive"),
    CONF_ECM_SPEED: (344, "unsigned", False, "none"),
    CONF_IZ2_OUTDOOR_TEMPERATURE: (31003, "signed_tenths", False, "iz2"),
    CONF_IZ2_DEMAND: (31005, "unsigned", False, "iz2"),
}

# Default sensor schemas with device class and units
SENSOR_DEFAULTS = {
    CONF_ENTERING_WATER_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer",
    ),
    CONF_LEAVING_WATER_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer",
    ),
    CONF_OUTDOOR_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:sun-thermometer",
    ),
    CONF_ENTERING_AIR_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer",
    ),
    CONF_LEAVING_AIR_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer",
    ),
    CONF_SUCTION_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer",
    ),
    CONF_DHW_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:water-boiler",
    ),
    CONF_DISCHARGE_PRESSURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_PSI,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_PRESSURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:gauge",
    ),
    CONF_SUCTION_PRESSURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_PSI,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_PRESSURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:gauge",
    ),
    CONF_LOOP_PRESSURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_PSI,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_PRESSURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:gauge",
    ),
    CONF_WATERFLOW: sensor.sensor_schema(
        unit_of_measurement=UNIT_GPM,
        accuracy_decimals=1,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:water",
    ),
    CONF_COMPRESSOR_POWER: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:heat-pump",
    ),
    CONF_BLOWER_POWER: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:fan",
    ),
    CONF_AUX_HEAT_POWER: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:fire",
    ),
    CONF_TOTAL_POWER: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:flash",
    ),
    CONF_PUMP_POWER: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:pump",
    ),
    CONF_LINE_VOLTAGE: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:sine-wave",
    ),
    CONF_COMPRESSOR_AMPS: sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_CURRENT,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:current-ac",
    ),
    CONF_BLOWER_AMPS: sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_CURRENT,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:current-ac",
    ),
    CONF_RELATIVE_HUMIDITY: sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_HUMIDITY,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:water-percent",
    ),
    CONF_COMPRESSOR_SPEED: sensor.sensor_schema(
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:speedometer",
    ),
    CONF_HEAT_OF_EXTRACTION: sensor.sensor_schema(
        unit_of_measurement=UNIT_BTU_H,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:fire",
    ),
    CONF_HEAT_OF_REJECTION: sensor.sensor_schema(
        unit_of_measurement=UNIT_BTU_H,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:fire",
    ),
    CONF_AMBIENT_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:home-thermometer",
    ),
    CONF_FP1_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer",
    ),
    CONF_FP2_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer",
    ),
    CONF_SAT_EVAP_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer",
    ),
    CONF_SUPERHEAT: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer-chevron-up",
    ),
    CONF_VS_DRIVE_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer",
    ),
    CONF_VS_LINE_VOLTAGE: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:sine-wave",
    ),
    CONF_VS_THERMO_POWER: sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:flash",
    ),
    CONF_VS_COMPRESSOR_POWER: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:heat-pump",
    ),
    CONF_VS_SUPPLY_VOLTAGE: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:flash",
    ),
    CONF_VS_UDC_VOLTAGE: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:flash",
    ),
    CONF_VS_COMPRESSOR_SPEED_REQUESTED: sensor.sensor_schema(
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:speedometer",
    ),
    CONF_VS_EEV2_OPEN: sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:valve",
    ),
    CONF_VS_DISCHARGE_PRESSURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_PSI,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_PRESSURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:gauge",
    ),
    CONF_VS_SUCTION_PRESSURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_PSI,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_PRESSURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:gauge",
    ),
    CONF_VS_DISCHARGE_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer",
    ),
    CONF_VS_COMPRESSOR_AMBIENT_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer",
    ),
    CONF_VS_ENTERING_WATER_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer",
    ),
    CONF_VS_INVERTER_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer",
    ),
    CONF_VS_FAN_SPEED: sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:fan",
    ),
    CONF_VS_SUCTION_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer",
    ),
    CONF_VS_SAT_EVAP_DISCHARGE_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer",
    ),
    CONF_VS_SUPERHEAT_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer-chevron-up",
    ),
    CONF_HEATING_LIQUID_LINE_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer",
    ),
    CONF_REFRIGERANT_LEAVING_AIR_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer",
    ),
    CONF_AUX_HEAT_AMPS: sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_CURRENT,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:current-ac",
    ),
    CONF_COMPRESSOR_2_AMPS: sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_CURRENT,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:current-ac",
    ),
    CONF_SAT_COND_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer",
    ),
    CONF_SUBCOOLING_HEATING: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer-chevron-down",
    ),
    CONF_SUBCOOLING_COOLING: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermometer-chevron-down",
    ),
    CONF_ECM_SPEED: sensor.sensor_schema(
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:fan",
    ),
    CONF_IZ2_OUTDOOR_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:sun-thermometer",
    ),
    CONF_IZ2_DEMAND: sensor.sensor_schema(
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:thermostat",
    ),
}

CONFIG_SCHEMA = cv.Schema(
    {
        **{
            cv.Optional(key): schema.extend(
                {cv.GenerateID(): cv.declare_id(WaterFurnaceSensor)}
            )
            for key, schema in SENSOR_DEFAULTS.items()
        },
    }
).extend(WATERFURNACE_CLIENT_SCHEMA)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_WATERFURNACE_ID])

    for key, (register, reg_type, is_32bit, capability) in SENSOR_TYPES.items():
        if key not in config:
            continue
        conf = config[key]
        var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(var, conf)
        await sensor.register_sensor(var, conf)
        cg.add(var.set_parent(parent))
        cg.add(var.set_register_address(register))
        cg.add(var.set_register_type(reg_type))
        cg.add(var.set_is_32bit(is_32bit))
        cg.add(var.set_capability(capability))
