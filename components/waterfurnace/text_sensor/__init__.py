import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import (
    CONF_ID,
    ENTITY_CATEGORY_DIAGNOSTIC,
)
from .. import waterfurnace_ns, WaterFurnace, CONF_WATERFURNACE_ID, WATERFURNACE_CLIENT_SCHEMA

DEPENDENCIES = ["waterfurnace"]

WaterFurnaceTextSensor = waterfurnace_ns.class_(
    "WaterFurnaceTextSensor", text_sensor.TextSensor, cg.Component
)

CONF_CURRENT_FAULT = "current_fault"
CONF_MODEL_NUMBER = "model_number"
CONF_SERIAL_NUMBER = "serial_number"
CONF_SYSTEM_MODE = "system_mode"

TEXT_SENSOR_TYPES = {
    CONF_CURRENT_FAULT: "fault",
    CONF_MODEL_NUMBER: "model",
    CONF_SERIAL_NUMBER: "serial",
    CONF_SYSTEM_MODE: "mode",
}

TEXT_SENSOR_SCHEMAS = {
    CONF_CURRENT_FAULT: text_sensor.text_sensor_schema(
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon="mdi:alert",
    ),
    CONF_MODEL_NUMBER: text_sensor.text_sensor_schema(
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon="mdi:information",
    ),
    CONF_SERIAL_NUMBER: text_sensor.text_sensor_schema(
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon="mdi:information",
    ),
    CONF_SYSTEM_MODE: text_sensor.text_sensor_schema(
        icon="mdi:hvac",
    ),
}

CONFIG_SCHEMA = cv.Schema(
    {
        **{
            cv.Optional(key): schema.extend(
                {cv.GenerateID(): cv.declare_id(WaterFurnaceTextSensor)}
            )
            for key, schema in TEXT_SENSOR_SCHEMAS.items()
        },
    }
).extend(WATERFURNACE_CLIENT_SCHEMA)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_WATERFURNACE_ID])

    for key, sensor_type in TEXT_SENSOR_TYPES.items():
        if key not in config:
            continue
        conf = config[key]
        var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(var, conf)
        await text_sensor.register_text_sensor(var, conf)
        cg.add(var.set_parent(parent))
        cg.add(var.set_sensor_type(sensor_type))
