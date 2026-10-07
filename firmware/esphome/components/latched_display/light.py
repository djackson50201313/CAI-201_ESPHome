import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_OUTPUT_ID

from . import LatchedDisplay, latched_display_ns

DEPENDENCIES = ["latched_display"]

LatchedDisplayLight = latched_display_ns.class_("LatchedDisplayLight", light.LightOutput)

CONF_LATCHED_DISPLAY_ID = "latched_display_id"

CONFIG_SCHEMA = light.RGB_LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(LatchedDisplayLight),
        cv.GenerateID(CONF_LATCHED_DISPLAY_ID): cv.use_id(LatchedDisplay),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)

    panel = await cg.get_variable(config[CONF_LATCHED_DISPLAY_ID])
    cg.add(var.set_panel(panel))
