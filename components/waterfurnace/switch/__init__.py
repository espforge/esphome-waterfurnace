import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import (
    CONF_ID,
    ENTITY_CATEGORY_CONFIG,
)
from .. import waterfurnace_ns, WaterFurnace, CONF_WATERFURNACE_ID, WATERFURNACE_CLIENT_SCHEMA

DEPENDENCIES = ["waterfurnace"]

WaterFurnaceSwitch = waterfurnace_ns.class_(
    "WaterFurnaceSwitch", switch.Switch, cg.Component
)

CONF_DHW_ENABLE = "dhw_enable"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_DHW_ENABLE): switch.switch_schema(
            WaterFurnaceSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:water-boiler",
        ).extend(cv.COMPONENT_SCHEMA),
    }
).extend(WATERFURNACE_CLIENT_SCHEMA)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_WATERFURNACE_ID])

    if CONF_DHW_ENABLE in config:
        conf = config[CONF_DHW_ENABLE]
        var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(var, conf)
        await switch.register_switch(var, conf)
        cg.add(var.set_parent(parent))
        cg.add(var.set_register_address(400))
        cg.add(var.set_write_address(400))
        cg.add(var.set_capability("axb"))
