import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32_ble_tracker, esp32_ble_client
from esphome.components.esp32 import add_idf_sdkconfig_option

from esphome.const import CONF_ID

AUTO_LOAD = ["esp32_ble_client", "esp32_ble_tracker"]
DEPENDENCIES = ["mqtt", "esp32"]

awox_ns = cg.esphome_ns.namespace("awox_mesh")

Awox = awox_ns.class_("AwoxMesh", esp32_ble_tracker.ESPBTDeviceListener, cg.Component)
MeshConnection = awox_ns.class_("MeshConnection", esp32_ble_client.BLEClientBase)

CONNECTION_SCHEMA = esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(MeshConnection),
    }
).extend(cv.COMPONENT_SCHEMA)

CONF_CONNECTIONS = "connections"
MAX_CONNECTIONS = 2

DEVICE_TYPES = {
    "RGB": 0x01,
    "DIM": 0x02,
    "WHITE_TEMP": 0x03,
    "PLUG": 0x04,
}

def validate_connections(config):
    if CONF_CONNECTIONS not in config:
        conf = config.copy()
        conf[CONF_CONNECTIONS] = [
            CONNECTION_SCHEMA({}) for _ in range(MAX_CONNECTIONS)
        ]
        return conf
    return config

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Awox),
            cv.Required("mesh_name"): cv.string_strict,
            cv.Required("mesh_password"): cv.string_strict,
            cv.Optional("address_prefix"): cv.string_strict,
            cv.Optional(CONF_CONNECTIONS): cv.All(
                cv.ensure_list(CONNECTION_SCHEMA),
                cv.Length(min=1, max=MAX_CONNECTIONS),
            ),
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
    .extend(cv.COMPONENT_SCHEMA),
    validate_connections,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    await esp32_ble_tracker.register_ble_device(var, config)

    cg.add(var.set_mesh_name(config["mesh_name"]))
    cg.add(var.set_mesh_password(config["mesh_password"]))

    for device in config.get("device_info", []):
        cg.add(
            var.register_device(
                device["device_type"],
                device["product_id"],
                device["name"],
                device["model"],
                device["manufacturer"],
                device["icon"],
            )
        )

    if config.get("address_prefix"):
        cg.add(var.set_address_prefix(config["address_prefix"]))

    for connection_conf in config.get(CONF_CONNECTIONS, []):
        connection_var = cg.new_Pvariable(connection_conf[CONF_ID])
        await cg.register_component(connection_var, connection_conf)
        cg.add(var.register_connection(connection_var))
        await esp32_ble_tracker.register_client(connection_var, connection_conf)


    add_idf_sdkconfig_option("CONFIG_BT_GATTC_CACHE_NVS_FLASH", True)
