import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor as binary_sensor_comp
from esphome.components import uart
from esphome import pins
from esphome.const import (
    CONF_ID,
    CONF_UPDATE_INTERVAL,
    CONF_FLOW_CONTROL_PIN,
    DEVICE_CLASS_CONNECTIVITY,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

CONF_CONNECTED = "connected"

DEPENDENCIES = ["uart"]
MULTI_CONF = False

CONF_WATERFURNACE_ID = "waterfurnace_id"
CONF_CONNECTED_TIMEOUT = "connected_timeout"

waterfurnace_ns = cg.esphome_ns.namespace("waterfurnace")
WaterFurnace = waterfurnace_ns.class_(
    "WaterFurnace", cg.PollingComponent, uart.UARTDevice
)

WATERFURNACE_CLIENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_WATERFURNACE_ID): cv.use_id(WaterFurnace),
    }
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(WaterFurnace),
            cv.Optional(CONF_FLOW_CONTROL_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_CONNECTED): binary_sensor_comp.binary_sensor_schema(
                device_class=DEVICE_CLASS_CONNECTIVITY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:lan-connect",
            ),
            cv.Optional(
                CONF_CONNECTED_TIMEOUT, default="30s"
            ): cv.positive_time_period_milliseconds,
        }
    )
    .extend(cv.polling_component_schema("10s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_FLOW_CONTROL_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_FLOW_CONTROL_PIN])
        cg.add(var.set_flow_control_pin(pin))

    if CONF_CONNECTED in config:
        sens = await binary_sensor_comp.new_binary_sensor(config[CONF_CONNECTED])
        cg.add(var.set_connected_sensor(sens))

    cg.add(var.set_connected_timeout(config[CONF_CONNECTED_TIMEOUT]))
