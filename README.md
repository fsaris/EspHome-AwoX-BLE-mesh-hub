# ESPHome component: AwoX BLE mesh hub

A ESPhome component (https://esphome.io/components/external_components.html#git) to create a hub for your AwoX BLE mesh devices. The devices are made available to Home Assistant through the native ESPHome API.

You will need your mesh credentials, easiest way to find/get these is to use [this form](https://fsaris.github.io/EspHome-AwoX-BLE-mesh-hub/awoxh-mesh-credentials-tool/) to read them from your AwoX Cloud account.

### Devices

Each mesh device you want in Home Assistant is listed under [`devices`](#devices-list-optional) in the YAML. Every entry becomes its own device in Home Assistant (connected through the hub) with one light or switch.

The native API can only offer entities that are known when the firmware is compiled, so devices can not be added automatically like with MQTT discovery. To make this easy the hub still finds all mesh devices, and for each device that is not in your YAML it writes a ready to copy entry to the log:

```
[I][awox.mesh]: Mesh device 12345 is not configured. Add it to awox_mesh -> devices:
  - {mesh_id: 12345, name: "Device 12345", type: }  # product 0x32, mac A4:C1:38:8A:BA:11
```

The same entries are also published one by one to the optional [`last_unconfigured_device`](#last_unconfigured_device-text-sensor-optional) sensor, so you can copy them from its history in Home Assistant.

So the first time: flash the hub, copy the entries from the sensor history or the log into your YAML, give them a nice name and flash again. The list is printed again every time you open the logs. The optional [`unconfigured_devices`](#active_connections-online_devices-unconfigured_devices-sensor-optional) sensor shows how many devices are still missing.

The `type` in these entries is empty: fill it in before you flash again, ESPHome will not accept the entry without it. The type depends on the product, shown as `product 0x..` at the end of the line. You can:

- paste the lines into the [helper](https://fsaris.github.io/EspHome-AwoX-BLE-mesh-hub/awoxh-mesh-credentials-tool/#devices), which fills in the `type`, a name and an icon for all known products, or
- look up the product id in the list of known products below.

<details>
  <summary>Known products</summary>

| Product id | `type` | Name | Model | Manufacturer | Suggested `icon` |
|---|---|---|---|---|---|
| `0x13` | `RGB` | SmartLIGHT Color Mesh 9 | SMLm_C9 | AwoX |  |
| `0x14` | `WHITE_TEMP` | SmartLIGHT White Mesh 13W | SMLm_W13 | AwoX |  |
| `0x15` | `RGB` | SmartLIGHT Color Mesh 13W | SMLm_C13 | AwoX |  |
| `0x16` | `WHITE_TEMP` | SmartLIGHT White Mesh 15W | SMLm_W15 | AwoX |  |
| `0x17` | `RGB` | SmartLIGHT Color Mesh 15W | SMLm_C15 | AwoX |  |
| `0x21` | `WHITE_TEMP` | SmartLIGHT White Mesh 9W | SSMLm_w9 | AwoX |  |
| `0x22` | `RGB` | SmartLIGHT Color Mesh 9W | SSMLm_c9 | AwoX |  |
| `0x23` | `RGB` | EGLOBulb A60 9W | ESMLm_c9 | EGLO |  |
| `0x24` | `RGB` | Keria SmartLIGHT Color Mesh 9W | KSMLm_c9 | KERIA |  |
| `0x25` | `RGB` | EGLOPanel 30X30 | EPanel_300 | EGLO |  |
| `0x26` | `RGB` | EGLOPanel 60X60 | EPanel_600 | EGLO |  |
| `0x27` | `RGB` | EGLO Ceiling DOWNLIGHT | EMod_Ceil | EGLO |  |
| `0x29` | `RGB` | EGLOBulb G95 13W | ESMLm_c13g | EGLO |  |
| `0x2A` | `RGB` | Keria SmartLIGHT Color Mesh 13W Globe | KSMLm_c13g | KERIA |  |
| `0x2B` | `RGB` | SmartLIGHT Color Mesh 13W Globe | SMLm_c13g | AwoX |  |
| `0x30` | `RGB` | EGLOPanel 30X120 | EPanel_120 | EGLO |  |
| `0x32` | `RGB` | Spot 120 | EGLOSpot 120/w | EGLO | `mdis:wall-sconce-flat` |
| `0x33` | `RGB` | Spot 170 | EGLOSpot 170/w | EGLO | `mdi:wall-sconce-flat` |
| `0x34` | `RGB` | Spot 225 | EGLOSpot 225/w | EGLO | `mdi:wall-sconce-flat` |
| `0x35` | `RGB` | Giron-C 17W | EGLO 32589 | EGLO | `mdi:wall-sconce-flat` |
| `0x36` | `RGB` | EGLO Ceiling GIRON 30 | ECeil_g38 | EGLO |  |
| `0x37` | `RGB` | SmartLIGHT Color Mesh 5W GU10 | SMLm_c5_GU10 | AwoX |  |
| `0x38` | `RGB` | SmartLIGHT Color Mesh 5W E14 | SMLm_c5_E14 | AwoX |  |
| `0x3A` | `RGB` | Keria SmartLIGHT Color Mesh 5W GU10 | KSMLm_c5_GU10 | KERIA |  |
| `0x3B` | `RGB` | Keria SmartLIGHT Color Mesh 5W E14 | KSMLm_c5_E14 | KERIA |  |
| `0x3C` | `RGB` | SmartLIGHT Color Mesh 5W GU10 | ESMLm_c5_GU10 | EGLO |  |
| `0x3D` | `RGB` | SmartLIGHT Color Mesh 5W E14 | ESMLm_c5_E14 | EGLO |  |
| `0x3F` | `RGB` | EGLO Surface round | EFueva_225r | EGLO |  |
| `0x40` | `RGB` | EGLO Surface square | EFueva_225s | EGLO |  |
| `0x41` | `RGB` | EGLO Surface round | EFueva_300r | EGLO |  |
| `0x42` | `RGB` | EGLO Surface square | EFueva_300s | EGLO |  |
| `0x43` | `RGB` | SmartLIGHT Color Mesh 9W | SMLm_c9s | AwoX |  |
| `0x44` | `RGB` | SmartLIGHT Color Mesh 13W | SMLm_c13gs | AwoX |  |
| `0x45` | `RGB` | EGLOBulb A60 9W | ESMLm_c9s | EGLO |  |
| `0x46` | `RGB` | EGLOBulb G95 13W | ESMLm_c13gs | EGLO |  |
| `0x47` | `RGB` | Keria SmartLIGHT Color Mesh 9W | KSMLm_c9s | KERIA |  |
| `0x48` | `RGB` | Keria SmartLIGHT Color Mesh 13W Glob | KSMLm_c13gs | KERIA |  |
| `0x49` | `DIM` | EGLOBulb A60 Warm | ESMLm_w9w | EGLO |  |
| `0x4A` | `DIM` | EGLOBulb A60 Neutral | ESMLm_w9n | EGLO |  |
| `0x4B` | `RGB` | EGLO Ceiling | ECeiling_30 | EGLO |  |
| `0x4C` | `RGB` | EGLO Pendant | EPendant_30 | EGLO |  |
| `0x4D` | `RGB` | EGLO Pendant | EPendant_20 | EGLO |  |
| `0x4E` | `RGB` | EGLO Stripled 3m | EStrip_3m | EGLO |  |
| `0x4F` | `RGB` | EGLO Stripled 5m | EStrip_5m | EGLO |  |
| `0x50` | `DIM` | Outdoor | EOutdoor_w14w | EGLO |  |
| `0x51` | `RGB` | EGLOSpot | ETriSpot_85 | EGLO |  |
| `0x53` | `RGB` | SmartLIGHT Color Mesh 9W | SMLm_c9i | AwoX |  |
| `0x54` | `RGB` | EGLOBulb A60 9W | ESMLm_c9i | EGLO |  |
| `0x55` | `RGB` | Keria SmartLIGHT Color Mesh 9W | KSMLm_c9i | KERIA |  |
| `0x56` | `RGB` | EGLOPanel 62X62 | EPanel_620 | EGLO |  |
| `0x57` | `RGB` | EGLOPanel 45X45 | EPanel_450 | EGLO |  |
| `0x59` | `RGB` | SmartLIGHT Color Mesh 13W Globe | SMLm_c13gi | AwoX |  |
| `0x5A` | `RGB` | EGLOBulb G95 13W | ESMLm_c13gi | EGLO |  |
| `0x5B` | `RGB` | Keria SmartLIGHT Color Mesh 13W Globe | KSMLm_c13gi | KERIA |  |
| `0x5C` | `RGB` | SmartLIGHT Color Mesh 9W | SSMLm_c9i | AwoX |  |
| `0x62` | `PLUG` | EGLO PLUG | ESMP-Bm10-FR | EGLO | `mdi:power-socket-fr` |
| `0x63` | `PLUG` | EGLO PLUG | ESMP-Bm10-GE | EGLO | `mdi:power-socket-de` |
| `0x64` | `WHITE_TEMP` | SmartLIGHT White Mesh 9W | SMLm_w9 | AwoX |  |
| `0x65` | `WHITE_TEMP` | SmartLIGHT White Mesh 9W | ESMLm_w9 | EGLO |  |
| `0x67` | `PLUG` | EGLO PLUG PLUS | SMPWBm10FR | EGLO | `mdi:power-socket-fr` |
| `0x68` | `PLUG` | EGLO PLUG PLUS | SMPWBm10GE | EGLO | `mdi:power-socket-de` |
| `0x69` | `RGB` | Ceiling GIRON 60 | ECeil_g60 | EGLO |  |
| `0x6A` | `WHITE_TEMP` | SmartLIGHT Bulb A60 Warm | SMLm_w9w | AwoX |  |
| `0x6F` | `WHITE_TEMP` | EGLOBulb Filament G80 | ESMLFm-w6-G80 | EGLO |  |
| `0x71` | `WHITE_TEMP` | EGLOBulb Filament ST64 | ESMLFm-w6-ST64 | EGLO |  |
| `0x75` | `WHITE_TEMP` | EGLOBulb Filament G95 | ESMLFm-w6-G95 | EGLO |  |
| `0x77` | `RGB` | EGLO Spot | ESpot_c5 | EGLO |  |
| `0x78` | `RGB` | EGLO Fraioli | EFraioli_c17 | EGLO |  |
| `0x79` | `RGB` | EGLO Frattina | EFrattina_c18 | EGLO |  |
| `0x7A` | `RGB` | EGLO Frattina | EFrattina_c27 | EGLO |  |
| `0x7B` | `RGB` | EGLOPanel 30 Round | EPanel_r300 | EGLO |  |
| `0x7C` | `RGB` | EGLOPanel 45 Round | EPanel_r450 | EGLO |  |
| `0x7D` | `RGB` | EGLOPanel 60 Round | EPanel_r600 | EGLO |  |
| `0x7E` | `RGB` | EGLOPanel 10X120 | EPanel_120_10 | EGLO |  |
| `0x80` | `WHITE_TEMP` | EPanel white round | EPanel_w_round | EGLO |  |
| `0x81` | `WHITE_TEMP` | EPanel white square | EPanel_w_square | EGLO |  |
| `0x82` | `WHITE_TEMP` | EPanel white rectangle | EPanel_w_rect | EGLO |  |
| `0x83` | `WHITE_TEMP` | ECeiling white round | ECeiling-w | EGLO |  |
| `0x84` | `PLUG` | EGLO PLUG PLUS | SMPWBm10FRa | EGLO | `mdi:power-socket-fr` |
| `0x85` | `PLUG` | EGLO PLUG PLUS | SMPWBm10GEa | EGLO | `mdi:power-socket-de` |
| `0x87` | `WHITE_TEMP` | EGLO Tunable White | EDoubleWhite | EGLO |  |
| `0x88` | `RGB` | EGLO Ceiling GIRON 80 | ECeil_g80 | EGLO |  |
| `0x89` | `WHITE_TEMP` | Outdoor Marchesa-C | EMarchesa_C | EGLO |  |
| `0x8A` | `WHITE_TEMP` | Outdoor Francari-C | EFrancari_C | EGLO |  |
| `0x8B` | `PLUG` | EGLO PLUG | ESMP-Bm10-AUS | EGLO | `mdi:power-socket-au` |
| `0x8C` | `PLUG` | EGLO PLUG | ESMP-Bm10-UK | EGLO | `mdi:power-socket-uk` |
| `0x8D` | `PLUG` | EGLO PLUG | ESMP-Bm10-CH | EGLO | `mdi:power-socket-ch` |
| `0x8F` | `PLUG` | EGLO PLUG PLUS | SMPWBm10UK | EGLO | `mdi:power-socket-uk` |
| `0x90` | `PLUG` | EGLO PLUG PLUS | SMPWBm10CH | EGLO | `mdi:power-socket-ch` |
| `0x92` | `RGB` | EPanel square | EPanel_36W_square | EGLO |  |
| `0x94` | `DIM` | EGLOBulb Filament ST64 | ESMLFm-w6w-ST64 | EGLO |  |
| `0x95` | `DIM` | EGLOBulb Filament G95 | ESMLFm-w6w-G95 | EGLO |  |
| `0x96` | `RGB` | EGLO RGB+TW | EGLO-RGB-TW | EGLO |  |
| `0x97` | `PLUG` | EGLO PLUG PLUS | SMPWBm10AUS | EGLO | `mdi:power-socket-au` |
| `0x99` | `RGB` | EGLO RGB+TW | EGLO-RGB-TW | EGLO |  |
| `0x9A` | `WHITE_TEMP` | EGLO Tunable White | JBT_Gen_CCT_1 | EGLO |  |
| `0x9B` | `DIM` | EGLO Tunable White | JBT_Gen_Dim_1 | EGLO |  |
| `0x9C` | `PLUG` | EGLO PLUG PLUS | SMPWBm10FRb | EGLO | `mdi:power-socket-fr` |
| `0x9D` | `PLUG` | EGLO PLUG PLUS | SMPWBm10GEb | EGLO | `mdi:power-socket-de` |
| `0x9E` | `PLUG` | EGLO PLUG PLUS | SMPWBm10AUSb | EGLO | `mdi:power-socket-au` |
| `0x9F` | `PLUG` | EGLO PLUG PLUS | SMPWBm10UKb | EGLO | `mdi:power-socket-uk` |
| `0xA0` | `PLUG` | EGLO PLUG PLUS | SMPWBm10CHb | EGLO | `mdi:power-socket-ch` |
| `0xA1` | `RGB` | EGLOLed Relax | ELedRelax | EGLO |  |
| `0xA2` | `RGB` | EGLOLed Stripe | ELedStripe | EGLO |  |
| `0xA3` | `RGB` | EGLOLed Plus | ELedPlus | EGLO |  |
| `0xA4` | `WHITE_TEMP` | EGLOLed Plus TW | ELedPlus-TW | EGLO |  |
| `0xA5` | `DIM` | EGLOLed Plus Dimmable | ELedPlus-Dimm | EGLO |  |
| `0xA6` | `WHITE_TEMP` | EGLOBulb | ESMLFm-w6-TW | EGLO |  |
| `0xA7` | `DIM` | EGLOBulb | ESMLFm-w6-Dimm | EGLO |  |
| `0xA8` | `RGB` | ECeiling square | ECeiling-24W-square | EGLO |  |
| `0xA9` | `RGB` | EGLO RGB+TW | EGLO-RGB-TW-IPSU | EGLO |  |
| `0xAA` | `WHITE_TEMP` | EGLO Tunable White | EGLO-TW-IPSU | EGLO |  |
| `0xAC` | `RGB` | EGLO frameless | EPanel-Frameless | EGLO |  |
| `0xAD` | `WHITE_TEMP` | EGLO Tunable White | EDoubleWhite-ipsu | EGLO |  |

When you add a product here, also add it to `PRODUCTS` in [awoxh-mesh-credentials-tool/index.html](awoxh-mesh-credentials-tool/index.html).
Is your product missing? Please open an issue with the product id, the model and the type that works.
</details>

### Groups/zones

The groups (called zone in AwoxHomeControl) are configured like devices, under [`groups`](#groups-list-optional). Groups that are found in the mesh but not configured are also written to the log, with the group id as name (example: `Group 3`).

When 1 light in a group is `on` the group will be shown as `on`. If 1 light in the group is `online` the group will be shown `online`.

### YAML options

For an example yaml see [`awox-ble-mesh-hub.yaml`](awox-ble-mesh-hub.yaml). For a full getting started see [#71](https://github.com/fsaris/EspHome-AwoX-BLE-mesh-hub/discussions/71).

The available `awox_mesh:` config options:

#### `mesh_name` _(string)_

To connect to you device you need the `mesh_name` and `mesh_password` easiest way to find/get these is to use [this form](https://fsaris.github.io/EspHome-AwoX-BLE-mesh-hub/awoxh-mesh-credentials-tool/) to read them from your AwoX Cloud account.


#### `mesh_password` _(string)_

See [`mesh_name`](#mesh_name-string)


#### `address_prefix` _(string - OPTIONAL)_

The device will scan for available BLE devices and will use this prefix to filter discovered devices by MAC address.

###### _Default value: `A4:C1`_


#### `min_rssi` _(number - OPTIONAL)_

The device will scan for available BLE devices and will use this value to filter discovered devices by the RSSI value.

###### _Default value: -90_


#### `allowed_mac_addresses` _(list of MAC addresses - OPTIONAL)_

Instead of the `address_prefix` option you can configure a list of mac addresses that the hub is allowed to connect to. This enables you to exclude some devices that are often offline or to create multiple "mesh" networks when combined with the `allowed_mesh_ids` option.

###### _Example:_
```yaml
  allowed_mac_addresses:
    - A4:C1:38:8A:BA:11
    - A4:C1:38:65:AD:22
```


#### `allowed_mesh_ids` _(list of numbers - OPTIONAL)_

You can give a list of mesh_ids that only should be handled by this hub (ESPHome device).
For example when you want to setup multiple ESP modules to only control a part of the mesh. Be sure to also set `allowed_mac_addresses` option to prevent that 1 device tries to connect to a mesh device that's not part of your `allowed_mesh_ids` list.

###### _Example:_
```yaml
 allowed_mesh_ids:
   - 12345
   - 23456
   - 6758
```


#### `max_connections` _(number, min: 1, max: 3 - OPTIONAL)_

In some situations the device that the hub connects to isn't able to reach the next device in the mesh. Then it can help that the hub connects to a different/additional device to be able to reach all device.

###### _Default value: 2_


#### `devices` _(list - OPTIONAL)_

The mesh devices to make available in Home Assistant. Each entry becomes a device in Home Assistant with one entity: a light, or a switch for `PLUG`.

###### _Example:_
```yaml
  devices:
    - mesh_id: 12345
      name: Kitchen spot
      type: RGB
    - mesh_id: 23456
      name: Coffee machine
      type: PLUG
      area_id: kitchen # an area from esphome -> areas
      power:
        name: Power
      voltage:
        name: Voltage
```

- `mesh_id` _(number)_ - The id of the device in the mesh, shown in the log.
- `name` _(string)_ - The name of the device in Home Assistant.
- `type` _(RGB, DIM, WHITE_TEMP, PLUG)_ - What kind of device it is, see the [known products](#devices):
  - `RGB` - light with color, white temperature and brightness
  - `WHITE_TEMP` - light with white temperature and brightness
  - `DIM` - light with brightness only
  - `PLUG` - plug or other on/off device, shown as a switch
- `area_id` _(OPTIONAL)_ - The id of an area defined under [`esphome: areas:`](https://esphome.io/components/esphome/#areas).
- `power`, `voltage` _(sensor - OPTIONAL, `PLUG` only)_ - Power (W) and voltage (V) measured by plugs with power monitoring, like the EGLO PLUG PLUS. When at least one is set, the hub asks the plug for a new reading every 5 seconds. All [sensor](https://esphome.io/components/sensor/#base-sensor-configuration) options can be used, for example `filters` to send fewer updates. The sensors are added to the same device as the switch.
- All options of an [ESPHome light](https://esphome.io/components/light/#base-light-configuration) or [switch](https://esphome.io/components/switch/#base-switch-configuration) can be used, for example `icon`, `on_turn_on` or `disabled_by_default`. Not available:
  - `effects` - the mesh effects `Candle` and `Color loop` are always added for RGB lights.
  - `restore_mode`, `restore_state` and `initial_state` - the state is always taken from the mesh.
  - `gamma_correct` - brightness is sent to the mesh without gamma correction.

The hub creates an ESPHome [sub-device](https://esphome.io/components/esphome/#sub-devices) with id `awox_mesh_<mesh_id>` for each entry, so do not use these ids yourself.


#### `groups` _(list - OPTIONAL)_

The mesh groups to make available in Home Assistant. Same options as [`devices`](#devices-list-optional), with `group_id` instead of `mesh_id`. The default `icon` of a group is `mdi:lightbulb-group`.

###### _Example:_
```yaml
  groups:
    - group_id: 3
      name: Living room
      type: RGB
```

When 1 light in a group is `on` the group will be shown as `on`.


#### `last_unconfigured_device` _(text sensor - OPTIONAL)_

Shows one device or group that is not in your YAML, as a line you can paste under `devices:` or `groups:`. The next one is shown every 5 seconds until all have been shown, and again each time Home Assistant connects to the hub. Open the history of the sensor in Home Assistant to see all of them. All options of an [ESPHome text sensor](https://esphome.io/components/text_sensor/#base-text-sensor-configuration) can be used.

###### _Example:_
```yaml
  last_unconfigured_device:
    name: Last unconfigured device
```


#### `active_connections`, `online_devices`, `unconfigured_devices` _(sensor - OPTIONAL)_

Diagnostic sensors of the hub: the number of BLE connections to the mesh, the number of mesh devices reachable through these connections, and the number of mesh devices and groups that are found but not in your YAML. All options of an [ESPHome sensor](https://esphome.io/components/sensor/#base-sensor-configuration) can be used.

###### _Example:_
```yaml
  active_connections:
    name: Active connections
  online_devices:
    name: Online devices
  unconfigured_devices:
    name: Unconfigured mesh devices
```


### Migrating from `device_info`

The `device_info` option is no longer used. It only described products, so the hub could suggest the `type` and `icon` of new devices. Everything a device needs is now set on its own entry under [`devices`](#devices-list-optional), and the [helper](https://fsaris.github.io/EspHome-AwoX-BLE-mesh-hub/awoxh-mesh-credentials-tool/#devices) or the [known products](#devices) list gives you the type of a product.

A YAML that still has `device_info` is not accepted any more: the config check stops with an error that links here. This way you notice it when you update, instead of getting devices with a wrong type.

| Old `device_info` field | Where it goes now |
|---|---|
| `device_type` | `type` on each `devices` entry of that product, and on `groups` entries of those devices |
| `icon` | `icon` on each `devices` entry (optional) |
| `name`, `model`, `manufacturer` | Not used any more. Give each device its own `name` |
| `product_id` | Not needed in the YAML. It is shown in the log, to look up the type |

###### _Before:_
```yaml
awox_mesh:
  device_info:
    - product_id: 0x32
      device_type: RGB
      manufacturer: EGLO
      name: Spot 120
      model: EGLOSpot 120/w
      icon: mdi:wall-sconce-flat
  devices:
    - mesh_id: 12345
      name: Kitchen spot
      type: RGB
```

###### _After:_
```yaml
awox_mesh:
  devices:
    - mesh_id: 12345
      name: Kitchen spot
      type: RGB
      icon: mdi:wall-sconce-flat
```

Steps:

1. Check that every entry under `devices` and `groups` has the right `type` (the `device_type` of its product).
2. Copy the `icon` of the product to the `devices` entries, if you want it.
3. Remove the whole `device_info:` block and flash the hub.
4. Devices that are not in your YAML yet are written to the log with an empty `type:`. Paste these lines into the [helper](https://fsaris.github.io/EspHome-AwoX-BLE-mesh-hub/awoxh-mesh-credentials-tool/#devices). You can also paste your old `device_info` block there, so the helper knows the products that are not in the known products list.
5. Did you have a product in `device_info` that is not in the [known products](#devices) list? Please open an issue, so it can be added for everyone.


### General YAML / Setup remarks

- Enable the Home Assistant API (`api:`), or use MQTT instead (see [below](#mqtt-instead-of-the-native-api))
- Use the `esp-idf` as framework type

#### Hidden SSID WLAN
In case of using a WLAN with hidden SSID, mind to use the multiple network option, to define the hidden variable of the wifi network: [Connecting to Multiple Networks](https://esphome.io/components/wifi.html#connecting-to-multiple-networks)


#### Ethernet instead of Wi-Fi (ESP32-S3, Seeed XIAO W5500 Ethernet Adapter)

The component does not use Wi-Fi itself, only the ESPHome API, so the hub can run on a wired connection. A ready-to-use example for the
[Seeed Studio XIAO W5500 Ethernet Adapter](https://wiki.seeedstudio.com/xiao_w5500_ethernet_adapter/) (XIAO ESP32-S3 Plus + WIZnet W5500, powered by PoE or USB-C)
is in [awox-ble-mesh-hub-xiao-esp32s3-w5500.yaml](awox-ble-mesh-hub-xiao-esp32s3-w5500.yaml).

Differences from the default [awox-ble-mesh-hub.yaml](awox-ble-mesh-hub.yaml):

- `board: seeed_xiao_esp32s3` and `variant: esp32s3` (see the [PlatformIO board](https://docs.platformio.org/en/latest/boards/espressif32/seeed_xiao_esp32s3.html))
- the `wifi:` section is replaced by an `ethernet:` section of type `W5500` with the pins from the
  [Seeed ESPHome guide](https://wiki.seeedstudio.com/esphome_xiao_w5500_ethernet_adapter/):

  ```yaml
  ethernet:
    type: W5500
    clk_pin: GPIO7
    mosi_pin: GPIO9
    miso_pin: GPIO8
    cs_pin: GPIO2
    interrupt_pin: GPIO10
  ```

Remarks:

- Seeed documents the ESPHome setup only for boards produced after November 1st 2025 (hardware revision V1.2, which fixed the `INT` pin wiring). Check the pins against the schematic if the Ethernet link does not come up with an older board.
- The first flash has to be done over USB-C. Afterwards `esphome upload --device <ip-address>` works over Ethernet. The `.local` mDNS name is not resolved across VLANs/subnets, use the IP address.
- The W5500 sends untagged frames. If you use VLANs, plug the board into an access port of the VLAN where Home Assistant is reachable.
- Without PoE the board needs 5 V on USB-C; a plain Ethernet cable only carries data.
- Without a cable the log shows `Connecting failed; reconnecting` every 15 seconds. This is normal and does not mean the pins are wrong.
- Other boards with a W5500 use the same [`ethernet:`](https://esphome.io/components/ethernet/) component, with their own board, variant and pins.

#### MQTT instead of the native API

The lights and switches are normal ESPHome entities, so they also work with the [`mqtt:`](https://esphome.io/components/mqtt/) component. Replace `api:` with `mqtt:` in your YAML:

```yaml
mqtt:
  broker: 192.168.1.10
  username: !secret mqtt_username
  password: !secret mqtt_password
```

Differences from the native API:

- ESPHome MQTT discovery does not support sub-devices. In Home Assistant all lights and switches are entities of the hub device, not separate devices.
- When there is no `api:` in the YAML, each light or switch gets the `name` of its device or group as its own name, so the entities can be told apart in Home Assistant. Use a different `name` for each device and group: the name is also used in the MQTT topics.
- When both `api:` and `mqtt:` are used, the native API setup is kept: the entities have no name of their own, so over MQTT they all show the name of the hub.
- Use either `api:` or `mqtt:`, not both. The Bluetooth connections to the mesh already use a large part of the ESP32 memory, and running both on the same hub can leave too little free memory and make it unstable.
- An `ota:` with `encryption:` but no `key:` takes the key from `api:`. Without `api:`, add the key to `ota:` itself.

### Requirements
- ESP32 module (Wi-Fi or Ethernet; the ESP32-S3 with a W5500 is described [above](#ethernet-instead-of-wi-fi-esp32-s3-seeed-xiao-w5500-ethernet-adapter))
- ESPHome 2026.10.1 or newer

### Recomendations
- HomeAssistant with the ESPHome integration (sub-device support, 2025.7.0 or newer)
- ESPHome add-on for HomeAssistant


### References
- https://github.com/fsaris/home-assistant-awox
- https://github.com/Leiaz/python-awox-mesh-light
- https://github.com/vpaeder/telinkpp
