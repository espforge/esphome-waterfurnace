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
CONF_SUBCOOLING = "subcooling"
CONF_SUPERHEAT = "superheat"

# Register address, register type, is_32bit
# register_type: "signed_tenths", "tenths", "unsigned", "uint32", "int32"
SENSOR_TYPES = {
    CONF_ENTERING_WATER_TEMPERATURE: (1111, "signed_tenths", False),
    CONF_LEAVING_WATER_TEMPERATURE: (1110, "signed_tenths", False),
    CONF_OUTDOOR_TEMPERATURE: (742, "signed_tenths", False),
    CONF_ENTERING_AIR_TEMPERATURE: (740, "signed_tenths", False),
    CONF_LEAVING_AIR_TEMPERATURE: (900, "signed_tenths", False),
    CONF_SUCTION_TEMPERATURE: (1113, "signed_tenths", False),
    CONF_DHW_TEMPERATURE: (1114, "signed_tenths", False),
    CONF_DISCHARGE_PRESSURE: (1115, "tenths", False),
    CONF_SUCTION_PRESSURE: (1116, "tenths", False),
    CONF_LOOP_PRESSURE: (1119, "tenths", False),
    CONF_WATERFLOW: (1117, "tenths", False),
    CONF_COMPRESSOR_POWER: (1146, "uint32", True),
    CONF_BLOWER_POWER: (1148, "uint32", True),
    CONF_AUX_HEAT_POWER: (1150, "uint32", True),
    CONF_TOTAL_POWER: (1152, "uint32", True),
    CONF_PUMP_POWER: (1164, "uint32", True),
    CONF_LINE_VOLTAGE: (16, "unsigned", False),
    CONF_COMPRESSOR_AMPS: (1107, "tenths", False),
    CONF_BLOWER_AMPS: (1105, "tenths", False),
    CONF_RELATIVE_HUMIDITY: (741, "unsigned", False),
    CONF_COMPRESSOR_SPEED: (3001, "unsigned", False),
    CONF_HEAT_OF_EXTRACTION: (1154, "int32", True),
    CONF_HEAT_OF_REJECTION: (1156, "int32", True),
    CONF_AMBIENT_TEMPERATURE: (747, "signed_tenths", False),
    CONF_FP1_TEMPERATURE: (19, "signed_tenths", False),
    CONF_FP2_TEMPERATURE: (20, "signed_tenths", False),
    CONF_SUBCOOLING: (1124, "signed_tenths", False),
    CONF_SUPERHEAT: (1125, "signed_tenths", False),
}

# Default sensor schemas with device class and units
SENSOR_DEFAULTS = {
    CONF_ENTERING_WATER_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_LEAVING_WATER_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_OUTDOOR_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_ENTERING_AIR_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_LEAVING_AIR_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_SUCTION_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_DHW_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_DISCHARGE_PRESSURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_PSI,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_PRESSURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_SUCTION_PRESSURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_PSI,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_PRESSURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_LOOP_PRESSURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_PSI,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_PRESSURE,
        state_class=STATE_CLASS_MEASUREMENT,
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
    ),
    CONF_BLOWER_POWER: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_AUX_HEAT_POWER: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_TOTAL_POWER: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_PUMP_POWER: sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_LINE_VOLTAGE: sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_COMPRESSOR_AMPS: sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_CURRENT,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_BLOWER_AMPS: sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_CURRENT,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_RELATIVE_HUMIDITY: sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_HUMIDITY,
        state_class=STATE_CLASS_MEASUREMENT,
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
    ),
    CONF_FP1_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_FP2_TEMPERATURE: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_SUBCOOLING: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    CONF_SUPERHEAT: sensor.sensor_schema(
        unit_of_measurement=UNIT_FAHRENHEIT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
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

    for key, (register, reg_type, is_32bit) in SENSOR_TYPES.items():
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
