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

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Awox),
            cv.Required("mesh_name"): cv.string_strict,
            cv.Required("mesh_password"): cv.string_strict,
            cv.Optional("connection", {}): CONNECTION_SCHEMA,
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

    await cg.register_component(connection_var, config["connection"])
    cg.add(var.register_connection(connection_var))
    await esp32_ble_tracker.register_client(connection_var, config["connection"])

    # Crypto
    cg.add_library("rweather/Crypto", "0.4.0")
