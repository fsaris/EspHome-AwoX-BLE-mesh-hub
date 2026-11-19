import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import (
    esp32_ble_client,
    esp32_ble_tracker,
    light,
    sensor,
    switch,
    text_sensor,
)
from esphome.const import (
    CONF_AREA_ID,
    CONF_DEFAULT_TRANSITION_LENGTH,
    CONF_DEVICE_ID,
    CONF_DEVICES,
    CONF_EFFECTS,
    CONF_ESPHOME,
    CONF_GAMMA_CORRECT,
    CONF_ICON,
    CONF_ID,
    CONF_INITIAL_STATE,
    CONF_NAME,
    CONF_RESTORE_MODE,
    CONF_RESTORE_STATE,
    CONF_TYPE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_COUNTER,
    STATE_CLASS_MEASUREMENT,
)
from esphome.core.config import FRIENDLY_NAME_MAX_LEN, Area, Device
import esphome.final_validate as fv
from esphome.types import ConfigType

# Sub-device relation added in ESPHome 2026.11; older versions ignore the key
CONF_RELATION = "relation"
DEVICE_RELATION_VIA = "via"

AUTO_LOAD = [
    "esp32_ble_client",
    "esp32_ble_tracker",
    "light",
    "switch",
    "sensor",
    "text_sensor",
]
DEPENDENCIES = ["esp32"]

awox_ns = cg.esphome_ns.namespace("awox_mesh")

Awox = awox_ns.class_("AwoxMesh", esp32_ble_tracker.ESPBTDeviceListener, cg.Component)
MeshConnection = awox_ns.class_("MeshConnection", esp32_ble_client.BLEClientBase)
AwoxMeshLight = awox_ns.class_("AwoxMeshLight", light.LightOutput)
AwoxMeshSwitch = awox_ns.class_("AwoxMeshSwitch", switch.Switch)

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
CONF_GROUPS = "groups"
CONF_MESH_ID = "mesh_id"
CONF_GROUP_ID = "group_id"
CONF_DEVICE_NAME = "device_name"
CONF_ACTIVE_CONNECTIONS = "active_connections"
CONF_ONLINE_DEVICES = "online_devices"
CONF_UNCONFIGURED_DEVICES = "unconfigured_devices"
CONF_LAST_UNCONFIGURED_DEVICE = "last_unconfigured_device"
MAX_CONNECTIONS = 3
# Group commands are sent to the group id with the high bit set
GROUP_DEST_OFFSET = 0x8000

DEVICE_TYPES = {
    "RGB": 0x01,
    "DIM": 0x02,
    "WHITE_TEMP": 0x03,
    "PLUG": 0x04,
}


def validate_connections(config: ConfigType) -> ConfigType:
    conf = config.copy()
    conf[CONF_CONNECTIONS] = [
        CONNECTION_SCHEMA({}) for _ in range(config[CONF_MAX_CONNECTIONS])
    ]
    return conf


# Accepted by the light or switch schema, but without effect here
_UNSUPPORTED_OPTIONS = {
    CONF_RESTORE_MODE: "the state is always taken from the mesh",
    CONF_RESTORE_STATE: "the state is always taken from the mesh",
    CONF_INITIAL_STATE: "the state is always taken from the mesh",
    CONF_GAMMA_CORRECT: "brightness is sent to the mesh without gamma correction",
}


def _light_entity_schema(light_type: light.LightType) -> cv.Schema:
    return light.light_schema(AwoxMeshLight, light_type).extend(
        {
            # Removes the default, so no unused gamma table ends up in the firmware
            cv.Optional(CONF_GAMMA_CORRECT): cv.invalid(
                _UNSUPPORTED_OPTIONS[CONF_GAMMA_CORRECT]
            ),
            # The bulbs fade themselves, an ESPHome transition would flood the mesh with commands
            cv.Optional(
                CONF_DEFAULT_TRANSITION_LENGTH, default="0s"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_EFFECTS): cv.invalid(
                "Effects are provided by the mesh (Candle, Color loop) and can not be configured"
            ),
        }
    )


_ENTITY_SCHEMAS = {
    "RGB": _light_entity_schema(light.LightType.RGB),
    "WHITE_TEMP": _light_entity_schema(light.LightType.BRIGHTNESS_ONLY),
    "DIM": _light_entity_schema(light.LightType.BRIGHTNESS_ONLY),
    "PLUG": switch.switch_schema(
        AwoxMeshSwitch, block_inverted=True, default_restore_mode="DISABLED"
    ),
}


def _mesh_entity_schema(
    id_key: str, id_validator, device_id_prefix: str, default_icon: str | None = None
):
    """One sub-device with one light or switch entity, named after the sub-device."""
    extra = {
        cv.Required(id_key): id_validator,
        cv.Required(CONF_DEVICE_NAME): cv.All(
            cv.string_no_slash, cv.ByteLength(max=FRIENDLY_NAME_MAX_LEN)
        ),
        cv.Required(CONF_DEVICE_ID): cv.declare_id(Device),
        cv.Optional(CONF_AREA_ID): cv.use_id(Area),
    }
    typed = cv.typed_schema(
        {key: schema.extend(extra) for key, schema in _ENTITY_SCHEMAS.items()},
        upper=True,
    )

    def validator(config: ConfigType) -> ConfigType:
        config = cv.Schema(
            {cv.Required(CONF_NAME): cv.string, cv.Required(id_key): id_validator},
            extra=cv.ALLOW_EXTRA,
        )(config)
        for key, reason in _UNSUPPORTED_OPTIONS.items():
            if key in config:
                raise cv.Invalid(f"'{key}' is not supported: {reason}", path=[key])
        # `name` names the sub-device; the entity itself has no name so HA shows the device name
        config[CONF_DEVICE_NAME] = config[CONF_NAME]
        config[CONF_NAME] = "None"
        # Derived from the mesh id so HA keeps the same device when the list is reordered
        config.setdefault(CONF_DEVICE_ID, f"{device_id_prefix}{config[id_key]}")
        if default_icon is not None:
            config.setdefault(CONF_ICON, default_icon)
        return typed(config)

    return validator


DEVICE_SCHEMA = _mesh_entity_schema(
    CONF_MESH_ID, cv.int_range(min=1, max=GROUP_DEST_OFFSET - 1), "awox_mesh_"
)
GROUP_SCHEMA = _mesh_entity_schema(
    CONF_GROUP_ID,
    cv.int_range(min=1, max=0xFF),
    "awox_mesh_group_",
    default_icon="mdi:lightbulb-group",
)


def _validate_unique(key: str):
    def validator(entries: list[ConfigType]) -> list[ConfigType]:
        seen = set()
        for entry in entries:
            if entry[key] in seen:
                raise cv.Invalid(f"Duplicate {key} {entry[key]}")
            seen.add(entry[key])
        return entries

    return validator


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
            cv.Optional(CONF_MAX_CONNECTIONS, default=2): cv.int_range(
                min=1, max=MAX_CONNECTIONS
            ),
            cv.Optional(CONF_MIN_RSSI): cv.int_range(min=-100, max=-10),
            cv.Optional(CONF_ALLOWED_MESH_IDS, default=[]): cv.ensure_list(cv.int_),
            cv.Optional(CONF_ALLOWED_ADDRESSES, default=[]): cv.ensure_list(
                cv.mac_address
            ),
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
            cv.Optional(CONF_DEVICES, default=[]): cv.All(
                cv.ensure_list(DEVICE_SCHEMA), _validate_unique(CONF_MESH_ID)
            ),
            cv.Optional(CONF_GROUPS, default=[]): cv.All(
                cv.ensure_list(GROUP_SCHEMA), _validate_unique(CONF_GROUP_ID)
            ),
            cv.Optional(CONF_LAST_UNCONFIGURED_DEVICE): text_sensor.text_sensor_schema(
                icon="mdi:lightbulb-question-outline",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_ACTIVE_CONNECTIONS): sensor.sensor_schema(
                icon="mdi:connection",
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_ONLINE_DEVICES): sensor.sensor_schema(
                icon=ICON_COUNTER,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_UNCONFIGURED_DEVICES): sensor.sensor_schema(
                icon="mdi:lightbulb-question-outline",
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    )
    .extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
    validate_connections,
    cv.require_esphome_version(2026, 10, 1),
)


def _final_validate(config: ConfigType) -> None:
    # Sub-devices can only be created by the core, so add ours to esphome -> devices
    core_devices = fv.full_config.get()[CONF_ESPHOME].setdefault(CONF_DEVICES, [])
    for entry in config[CONF_DEVICES] + config[CONF_GROUPS]:
        device = {
            CONF_ID: entry[CONF_DEVICE_ID],
            CONF_NAME: entry[CONF_DEVICE_NAME],
            CONF_RELATION: DEVICE_RELATION_VIA,
        }
        if (area_id := entry.get(CONF_AREA_ID)) is not None:
            device[CONF_AREA_ID] = area_id
        core_devices.append(device)


FINAL_VALIDATE_SCHEMA = _final_validate


async def _register_entity(var: cg.MockObj, dest: int, entry: ConfigType) -> None:
    if entry[CONF_TYPE] == "PLUG":
        entity = await switch.new_switch(entry, var, dest)
    else:
        entity = await light.new_light(entry, var, dest, DEVICE_TYPES[entry[CONF_TYPE]])
    cg.add(var.register_entity(entity))


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    await esp32_ble_tracker.register_ble_device(var, config)

    cg.add(var.set_mesh_name(config[CONF_MESH_NAME]))
    cg.add(var.set_mesh_password(config[CONF_MESH_PASSWORD]))

    for device in config[CONF_DEVICE_INFO]:
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

    for mesh_id in config[CONF_ALLOWED_MESH_IDS]:
        cg.add(var.add_allowed_mesh_id(mesh_id))

    for mac_address in config[CONF_ALLOWED_ADDRESSES]:
        cg.add(var.add_allowed_mac_address(mac_address.as_hex))

    if address_prefix := config.get(CONF_ADDRESS_PREFIX):
        cg.add(var.set_address_prefix(address_prefix))

    if (min_rssi := config.get(CONF_MIN_RSSI)) is not None:
        cg.add(var.set_min_rssi(min_rssi))

    for entry in config[CONF_DEVICES]:
        await _register_entity(var, entry[CONF_MESH_ID], entry)
    for entry in config[CONF_GROUPS]:
        await _register_entity(var, entry[CONF_GROUP_ID] + GROUP_DEST_OFFSET, entry)

    sensors = sensor.sub_sensors(config)
    await sensors(CONF_ACTIVE_CONNECTIONS, var.set_active_connections_sensor)
    await sensors(CONF_ONLINE_DEVICES, var.set_online_devices_sensor)
    await sensors(CONF_UNCONFIGURED_DEVICES, var.set_unconfigured_devices_sensor)
    text_sensors = text_sensor.sub_text_sensors(config)
    await text_sensors(CONF_LAST_UNCONFIGURED_DEVICE, var.set_last_unconfigured_sensor)

    for connection_conf in config[CONF_CONNECTIONS]:
        connection_var = cg.new_Pvariable(connection_conf[CONF_ID])
        await cg.register_component(connection_var, connection_conf)
        cg.add(var.register_connection(connection_var))
        await esp32_ble_tracker.register_client(connection_var, connection_conf)
