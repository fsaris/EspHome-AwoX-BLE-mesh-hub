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

CONF_MESH_NAME = "mesh_name"
CONF_MESH_PASSWORD = "mesh_password"
CONF_ADDRESS_PREFIX = "address_prefix"
CONF_MIN_RSSI = "min_rssi"
CONF_CONNECTIONS = "connections"
CONF_MAX_CONNECTIONS = "max_connections"
CONF_DEVICE_INFO = "device_info"
CONF_ALLOWED_MESH_IDS = "allowed_mesh_ids"
CONF_ALLOWED_ADDRESSES = "allowed_mac_addresses"
MAX_CONNECTIONS = 3

DEVICE_TYPES = {
    "RGB": 0x01,
    "DIM": 0x02,
    "WHITE_TEMP": 0x03,
    "PLUG": 0x04,
}

def validate_connections(config):
    max_connections = MAX_CONNECTIONS
    if CONF_MAX_CONNECTIONS in config:
        max_connections = config[CONF_MAX_CONNECTIONS]

    conf = config.copy()
    conf[CONF_CONNECTIONS] = [
        CONNECTION_SCHEMA({}) for _ in range(max_connections)
    ]
    return conf


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Awox),
            cv.Required(CONF_MESH_NAME): cv.All(
                cv.string_strict,
                cv.Length(max=8),
            ),
            cv.Required(CONF_MESH_PASSWORD): cv.All(
                cv.string_strict,
                cv.Length(max=8),
            ),
            cv.Optional(CONF_ADDRESS_PREFIX): cv.string_strict,
            cv.Optional(CONF_MAX_CONNECTIONS, default=2): cv.int_range(min=1, max=MAX_CONNECTIONS),
            cv.Optional(CONF_MIN_RSSI): cv.int_range(min=-100, max=-10),
            cv.Optional(CONF_ALLOWED_MESH_IDS, default=[]): cv.ensure_list(cv.int_),
            cv.Optional(CONF_ALLOWED_ADDRESSES, default=[]): cv.ensure_list(cv.mac_address),
            cv.Optional(CONF_DEVICE_INFO, default=[]): cv.ensure_list(
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

    cg.add(var.set_mesh_name(config[CONF_MESH_NAME]))
    cg.add(var.set_mesh_password(config[CONF_MESH_PASSWORD]))

    for device in config.get(CONF_DEVICE_INFO, []):
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

    for mesh_id in config.get(CONF_ALLOWED_MESH_IDS, []):
        cg.add(var.add_allowed_mesh_id(mesh_id))

    for mac_address in config.get(CONF_ALLOWED_ADDRESSES, []):
        cg.add(var.add_allowed_mac_address(mac_address.as_hex))


    if config.get(CONF_ADDRESS_PREFIX):
        cg.add(var.set_address_prefix(config[CONF_ADDRESS_PREFIX]))

    if config.get(CONF_MIN_RSSI):
        cg.add(var.set_min_rssi(config[CONF_MIN_RSSI]))

    for connection_conf in config.get(CONF_CONNECTIONS, []):
        connection_var = cg.new_Pvariable(connection_conf[CONF_ID])
        await cg.register_component(connection_var, connection_conf)
        cg.add(var.register_connection(connection_var))
        await esp32_ble_tracker.register_client(connection_var, connection_conf)

