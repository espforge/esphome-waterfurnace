import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.const import CONF_ID
from .. import waterfurnace_ns, WaterFurnace, CONF_WATERFURNACE_ID, WATERFURNACE_CLIENT_SCHEMA

DEPENDENCIES = ["waterfurnace"]

CONF_ZONE = "zone"

WaterFurnaceClimate = waterfurnace_ns.class_(
    "WaterFurnaceClimate", climate.Climate, cg.Component
)

CONFIG_SCHEMA = climate.climate_schema(WaterFurnaceClimate).extend(
    {
        cv.Optional(CONF_ZONE, default=0): cv.int_range(min=0, max=6),
    }
).extend(WATERFURNACE_CLIENT_SCHEMA).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    parent = await cg.get_variable(config[CONF_WATERFURNACE_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_zone(config[CONF_ZONE]))
