import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_PROBLEM,
    DEVICE_CLASS_RUNNING,
    ENTITY_CATEGORY_DIAGNOSTIC,
)
from .. import waterfurnace_ns, WaterFurnace, CONF_WATERFURNACE_ID, WATERFURNACE_CLIENT_SCHEMA

DEPENDENCIES = ["waterfurnace"]

WaterFurnaceBinarySensor = waterfurnace_ns.class_(
    "WaterFurnaceBinarySensor", binary_sensor.BinarySensor, cg.Component
)

CONF_COMPRESSOR = "compressor"
CONF_COMPRESSOR_STAGE2 = "compressor_stage2"
CONF_REVERSING_VALVE = "reversing_valve"
CONF_BLOWER = "blower"
CONF_AUX_HEAT_STAGE1 = "aux_heat_stage1"
CONF_AUX_HEAT_STAGE2 = "aux_heat_stage2"
CONF_LOCKOUT = "lockout"
CONF_ALARM = "alarm"
CONF_ACCESSORY = "accessory"
CONF_AXB_DHW = "axb_dhw"
CONF_AXB_LOOP_PUMP = "axb_loop_pump"
CONF_AXB_DIVERTING_VALVE = "axb_diverting_valve"
CONF_AXB_DEHUMIDIFIER = "axb_dehumidifier"
CONF_AXB_ACCESSORY2 = "axb_accessory2"

# (register_address, bitmask, capability)
BINARY_SENSOR_TYPES = {
    CONF_COMPRESSOR: (30, 0x01, "none"),
    CONF_COMPRESSOR_STAGE2: (30, 0x02, "none"),
    CONF_REVERSING_VALVE: (30, 0x04, "none"),
    CONF_BLOWER: (30, 0x08, "none"),
    CONF_AUX_HEAT_STAGE1: (30, 0x10, "none"),
    CONF_AUX_HEAT_STAGE2: (30, 0x20, "none"),
    CONF_ACCESSORY: (30, 0x200, "none"),
    CONF_LOCKOUT: (30, 0x400, "none"),
    CONF_ALARM: (30, 0x800, "none"),
    CONF_AXB_DHW: (1104, 0x01, "axb"),
    CONF_AXB_LOOP_PUMP: (1104, 0x02, "axb"),
    CONF_AXB_DIVERTING_VALVE: (1104, 0x04, "axb"),
    CONF_AXB_DEHUMIDIFIER: (1104, 0x08, "axb"),
    CONF_AXB_ACCESSORY2: (1104, 0x10, "axb"),
}

BINARY_SENSOR_SCHEMAS = {
    CONF_COMPRESSOR: binary_sensor.binary_sensor_schema(
        icon="mdi:heat-pump",
    ),
    CONF_COMPRESSOR_STAGE2: binary_sensor.binary_sensor_schema(
        icon="mdi:heat-pump",
    ),
    CONF_REVERSING_VALVE: binary_sensor.binary_sensor_schema(
        icon="mdi:valve",
    ),
    CONF_BLOWER: binary_sensor.binary_sensor_schema(
        icon="mdi:fan",
    ),
    CONF_AUX_HEAT_STAGE1: binary_sensor.binary_sensor_schema(
        icon="mdi:fire",
    ),
    CONF_AUX_HEAT_STAGE2: binary_sensor.binary_sensor_schema(
        icon="mdi:fire",
    ),
    CONF_ACCESSORY: binary_sensor.binary_sensor_schema(
        icon="mdi:toggle-switch",
    ),
    CONF_LOCKOUT: binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon="mdi:alert-circle",
    ),
    CONF_ALARM: binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon="mdi:alert-circle",
    ),
    CONF_AXB_DHW: binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_RUNNING,
        icon="mdi:water-boiler",
    ),
    CONF_AXB_LOOP_PUMP: binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_RUNNING,
        icon="mdi:pump",
    ),
    CONF_AXB_DIVERTING_VALVE: binary_sensor.binary_sensor_schema(
        icon="mdi:valve",
    ),
    CONF_AXB_DEHUMIDIFIER: binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_RUNNING,
        icon="mdi:air-humidifier",
    ),
    CONF_AXB_ACCESSORY2: binary_sensor.binary_sensor_schema(
        icon="mdi:toggle-switch",
    ),
}

CONFIG_SCHEMA = cv.Schema(
    {
        **{
            cv.Optional(key): BINARY_SENSOR_SCHEMAS[key].extend(
                {cv.GenerateID(): cv.declare_id(WaterFurnaceBinarySensor)}
            )
            for key in BINARY_SENSOR_TYPES
        },
    }
).extend(WATERFURNACE_CLIENT_SCHEMA)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_WATERFURNACE_ID])

    for key, (register, bitmask, capability) in BINARY_SENSOR_TYPES.items():
        if key not in config:
            continue
        conf = config[key]
        var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(var, conf)
        await binary_sensor.register_binary_sensor(var, conf)
        cg.add(var.set_parent(parent))
        cg.add(var.set_register_address(register))
        cg.add(var.set_bitmask(bitmask))
        cg.add(var.set_capability(capability))
