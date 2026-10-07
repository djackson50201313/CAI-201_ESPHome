import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import time as time_
from esphome.const import CONF_ID, CONF_NUMBER, PLATFORM_ESP32

CODEOWNERS = ["@djackson50201313"]
ESP_PLATFORMS = [PLATFORM_ESP32]

latched_display_ns = cg.esphome_ns.namespace("latched_display")
LatchedDisplay = latched_display_ns.class_("LatchedDisplay", cg.Component)

CONF_LE_PIN = "le_pin"
CONF_CLK_PIN = "clk_pin"
CONF_SDI_PIN = "sdi_pin"
CONF_D1_PIN = "d1_pin"
CONF_D0_PIN = "d0_pin"
CONF_EN_PIN = "en_pin"
CONF_TIME_ID = "time_id"
CONF_TWELVE_HOUR = "twelve_hour"
CONF_LATCHED_DISPLAY_ID = "latched_display_id"


def _low_bank_pin(value):
    """All six display pins must be GPIO0-GPIO31 for fast register writes."""
    pin = value[CONF_NUMBER]
    if pin >= 32:
        raise cv.Invalid(
            f"GPIO{pin} is above GPIO31. Pick a pin below GPIO32."
        )
    return value


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(LatchedDisplay),
        cv.Required(CONF_LE_PIN): cv.All(
            pins.internal_gpio_output_pin_schema, _low_bank_pin
        ),
        cv.Required(CONF_CLK_PIN): cv.All(
            pins.internal_gpio_output_pin_schema, _low_bank_pin
        ),
        cv.Required(CONF_SDI_PIN): cv.All(
            pins.internal_gpio_output_pin_schema, _low_bank_pin
        ),
        cv.Required(CONF_D1_PIN): cv.All(
            pins.internal_gpio_output_pin_schema, _low_bank_pin
        ),
        cv.Required(CONF_D0_PIN): cv.All(
            pins.internal_gpio_output_pin_schema, _low_bank_pin
        ),
        cv.Required(CONF_EN_PIN): cv.All(
            pins.internal_gpio_output_pin_schema, _low_bank_pin
        ),
        cv.Optional(CONF_TIME_ID): cv.use_id(time_.RealTimeClock),
        cv.Optional(CONF_TWELVE_HOUR, default=False): cv.boolean,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    le = await cg.gpio_pin_expression(config[CONF_LE_PIN])
    clk = await cg.gpio_pin_expression(config[CONF_CLK_PIN])
    sdi = await cg.gpio_pin_expression(config[CONF_SDI_PIN])
    d1 = await cg.gpio_pin_expression(config[CONF_D1_PIN])
    d0 = await cg.gpio_pin_expression(config[CONF_D0_PIN])
    en = await cg.gpio_pin_expression(config[CONF_EN_PIN])

    cg.add(var.set_pins(le, clk, sdi, d1, d0, en))
    cg.add(var.set_twelve_hour(config[CONF_TWELVE_HOUR]))

    if CONF_TIME_ID in config:
        time_ = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time(time_))

    if CONF_LATCHED_DISPLAY_ID in config:
        light = await cg.get_variable(config[CONF_LATCHED_DISPLAY_ID])
        cg.add(var.set_panel(light))
