import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32_ble_tracker, esp32_ble_client

from esphome.const import CONF_ID

AUTO_LOAD = ["esp32_ble_client", "esp32_ble_tracker"]
DEPENDENCIES = ["mqtt", "esp32"]

awox_ns = cg.esphome_ns.namespace("awox_mesh")

Awox = awox_ns.class_("AwoxMesh", esp32_ble_tracker.ESPBTDeviceListener, cg.Component)
MeshDevice = awox_ns.class_("MeshDevice", esp32_ble_client.BLEClientBase)

CONNECTION_SCHEMA = esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(MeshDevice),
    }
).extend(cv.COMPONENT_SCHEMA)

DEVICE_TYPES = {
    "RGB": 0x01,
    "DIM": 0x02,
    "WHITE_TEMP": 0x03,
    "PLUG": 0x04,
}

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Awox),
            cv.Required("mesh_name"): cv.string_strict,
            cv.Required("mesh_password"): cv.string_strict,
            cv.Optional("connection", {}): CONNECTION_SCHEMA,
            cv.Optional("device_info", default=[]): cv.ensure_list(
                cv.Schema(
                    {
                        cv.Required("product_id"): cv.hex_int_range(min=0x01, max=0xFF),
                        cv.Required("device_type"): cv.enum(DEVICE_TYPES),
                        cv.Required("name"): cv.string,
                        cv.Required("model"): cv.string,
                        cv.Required("manufacturer"): cv.string,
                        cv.Optional("icon", default=""): cv.icon,
                    }
                )
            ),
        }
    )
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)

    connection_var = cg.new_Pvariable(config["connection"][CONF_ID])
    cg.add(connection_var.set_mesh_name(config["mesh_name"]))
    cg.add(connection_var.set_mesh_password(config["mesh_password"]))

    for device in config.get("device_info", []):
        cg.add(
            connection_var.register_device(
                device["device_type"],
                device["product_id"],
                device["name"],
                device["model"],
                device["manufacturer"],
                device["icon"],
            )
        )

    await cg.register_component(connection_var, config["connection"])
    cg.add(var.register_connection(connection_var))
    await esp32_ble_tracker.register_client(connection_var, config["connection"])

    # Crypto
    cg.add_library("rweather/Crypto", "0.4.0")
