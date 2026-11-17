# ESPHome component: AwoX BLE mesh hub

A ESPhome component (https://esphome.io/components/external_components.html#git) to create a hub for your AwoX BLE mesh devices. The devices are made available to Home Assistant through the native ESPHome API.

You will need your mesh credentials, easiest way to find/get these is to use [this form](https://fsaris.github.io/EspHome-AwoX-BLE-mesh-hub/awoxh-mesh-credentials-tool/) to read them from your AwoX Cloud account.

### Devices

Each mesh device you want in Home Assistant is listed under [`devices`](#devices-list-optional) in the YAML. Every entry becomes its own device in Home Assistant (connected through the hub) with one light or switch.

The native API can only offer entities that are known when the firmware is compiled, so devices can not be added automatically like with MQTT discovery. To make this easy the hub still finds all mesh devices, and for each device that is not in your YAML it writes a ready to copy entry to the log:

```
[I][awox.mesh]: Mesh device 12345 is not configured. Add it to awox_mesh -> devices:
  - {mesh_id: 12345, name: "Spot 120 12345", type: RGB, icon: "mdi:wall-sconce-flat"}  # product 0x32, mac A4:C1:38:8A:BA:11
```

The same entries are also published one by one to the optional [`last_unconfigured_device`](#last_unconfigured_device-text-sensor-optional) sensor, so you can copy them from its history in Home Assistant.

So the first time: flash the hub, copy the entries from the sensor history or the log into your YAML, give them a nice name and flash again. The list is printed again every time you open the logs. The optional [`unconfigured_devices`](#active_connections-online_devices-unconfigured_devices-sensor-optional) sensor shows how many devices are still missing.

The `type`, `name` and `icon` in the log are taken from the [`device_info`](#device_info) list, by `product_id`. Unknown products are suggested as `DIM`, check the type of these yourself.

<details>
  <summary>Config examples known devices</summary>

```yaml
device_info:
-  product_id: 0x13
   device_type: RGB
   name: SmartLIGHT Color Mesh 9
   model: SMLm_C9
   manufacturer: AwoX

-  product_id: 0x14
   device_type: WHITE_TEMP
   name: SmartLIGHT White Mesh 13W
   model: SMLm_W13
   manufacturer: AwoX

-  product_id: 0x15
   device_type: RGB
   name: SmartLIGHT Color Mesh 13W
   model: SMLm_C13
   manufacturer: AwoX

-  product_id: 0x16
   device_type: WHITE_TEMP
   name: SmartLIGHT White Mesh 15W
   model: SMLm_W15
   manufacturer: AwoX

-  product_id: 0x17
   device_type: RGB
   name: SmartLIGHT Color Mesh 15W
   model: SMLm_C15
   manufacturer: AwoX

-  product_id: 0x21
   device_type: WHITE_TEMP
   name: SmartLIGHT White Mesh 9W
   model: SSMLm_w9
   manufacturer: AwoX

-  product_id: 0x22
   device_type: RGB
   name: SmartLIGHT Color Mesh 9W
   model: SSMLm_c9
   manufacturer: AwoX

-  product_id: 0x23
   device_type: RGB
   name: EGLOBulb A60 9W
   model: ESMLm_c9
   manufacturer: EGLO

-  product_id: 0x24
   device_type: RGB
   name: Keria SmartLIGHT Color Mesh 9W
   model: KSMLm_c9
   manufacturer: KERIA

-  product_id: 0x25
   device_type: RGB
   name: EGLOPanel 30X30
   model: EPanel_300
   manufacturer: EGLO

-  product_id: 0x26
   device_type: RGB
   name: EGLOPanel 60X60
   model: EPanel_600
   manufacturer: EGLO

-  product_id: 0x27
   device_type: RGB
   name: EGLO Ceiling DOWNLIGHT
   model: EMod_Ceil
   manufacturer: EGLO

-  product_id: 0x29
   device_type: RGB
   name: EGLOBulb G95 13W
   model: ESMLm_c13g
   manufacturer: EGLO

-  product_id: 0x2A
   device_type: RGB
   name: Keria SmartLIGHT Color Mesh 13W Globe
   model: KSMLm_c13g
   manufacturer: KERIA

-  product_id: 0x2B
   device_type: RGB
   name: SmartLIGHT Color Mesh 13W Globe
   model: SMLm_c13g
   manufacturer: AwoX

-  product_id: 0x30
   device_type: RGB
   name: EGLOPanel 30X120
   model: EPanel_120
   manufacturer: EGLO

-  product_id: 0x32
   device_type: RGB
   name: Spot 120
   model: EGLOSpot 120/w
   manufacturer: EGLO
   icon: mdis:wall-sconce-flat

-  product_id: 0x33
   device_type: RGB
   name: Spot 170
   model: EGLOSpot 170/w
   manufacturer: EGLO
   icon: mdi:wall-sconce-flat

-  product_id: 0x34
   device_type: RGB
   name: Spot 225
   model: EGLOSpot 225/w
   manufacturer: EGLO
   icon: mdi:wall-sconce-flat

-  product_id: 0x35
   device_type: RGB
   name: Giron-C 17W
   model: EGLO 32589
   manufacturer: EGLO
   icon: mdi:wall-sconce-flat

-  product_id: 0x36
   device_type: RGB
   name: EGLO Ceiling GIRON 30
   model: ECeil_g38
   manufacturer: EGLO

-  product_id: 0x37
   device_type: RGB
   name: SmartLIGHT Color Mesh 5W GU10
   model: SMLm_c5_GU10
   manufacturer: AwoX

-  product_id: 0x38
   device_type: RGB
   name: SmartLIGHT Color Mesh 5W E14
   model: SMLm_c5_E14
   manufacturer: AwoX

-  product_id: 0x3A
   device_type: RGB
   name: Keria SmartLIGHT Color Mesh 5W GU10
   model: KSMLm_c5_GU10
   manufacturer: KERIA

-  product_id: 0x3B
   device_type: RGB
   name: Keria SmartLIGHT Color Mesh 5W E14
   model: KSMLm_c5_E14
   manufacturer: KERIA

-  product_id: 0x3C
   device_type: RGB
   name: SmartLIGHT Color Mesh 5W GU10
   model: ESMLm_c5_GU10
   manufacturer: EGLO

-  product_id: 0x3D
   device_type: RGB
   name: SmartLIGHT Color Mesh 5W E14
   model: ESMLm_c5_E14
   manufacturer: EGLO

-  product_id: 0x3F
   device_type: RGB
   name: EGLO Surface round
   model: EFueva_225r
   manufacturer: EGLO

-  product_id: 0x40
   device_type: RGB
   name: EGLO Surface square
   model: EFueva_225s
   manufacturer: EGLO

-  product_id: 0x41
   device_type: RGB
   name: EGLO Surface round
   model: EFueva_300r
   manufacturer: EGLO

-  product_id: 0x42
   device_type: RGB
   name: EGLO Surface square
   model: EFueva_300s
   manufacturer: EGLO

-  product_id: 0x43
   device_type: RGB
   name: SmartLIGHT Color Mesh 9W
   model: SMLm_c9s
   manufacturer: AwoX

-  product_id: 0x44
   device_type: RGB
   name: SmartLIGHT Color Mesh 13W
   model: SMLm_c13gs
   manufacturer: AwoX

-  product_id: 0x45
   device_type: RGB
   name: EGLOBulb A60 9W
   model: ESMLm_c9s
   manufacturer: EGLO

-  product_id: 0x46
   device_type: RGB
   name: EGLOBulb G95 13W
   model: ESMLm_c13gs
   manufacturer: EGLO

-  product_id: 0x47
   device_type: RGB
   name: Keria SmartLIGHT Color Mesh 9W
   model: KSMLm_c9s
   manufacturer: KERIA

-  product_id: 0x48
   device_type: RGB
   name: Keria SmartLIGHT Color Mesh 13W Glob
   model: KSMLm_c13gs
   manufacturer: KERIA

-  product_id: 0x49
   device_type: DIM
   name: EGLOBulb A60 Warm
   model: ESMLm_w9w
   manufacturer: EGLO

-  product_id: 0x4A
   device_type: DIM
   name: EGLOBulb A60 Neutral
   model: ESMLm_w9n
   manufacturer: EGLO

-  product_id: 0x4B
   device_type: RGB
   name: EGLO Ceiling
   model: ECeiling_30
   manufacturer: EGLO

-  product_id: 0x4C
   device_type: RGB
   name: EGLO Pendant
   model: EPendant_30
   manufacturer: EGLO

-  product_id: 0x4D
   device_type: RGB
   name: EGLO Pendant
   model: EPendant_20
   manufacturer: EGLO

-  product_id: 0x4E
   device_type: RGB
   name: EGLO Stripled 3m
   model: EStrip_3m
   manufacturer: EGLO

-  product_id: 0x4F
   device_type: RGB
   name: EGLO Stripled 5m
   model: EStrip_5m
   manufacturer: EGLO

-  product_id: 0x50
   device_type: DIM
   name: Outdoor
   model: EOutdoor_w14w
   manufacturer: EGLO

-  product_id: 0x51
   device_type: RGB
   name: EGLOSpot
   model: ETriSpot_85
   manufacturer: EGLO

-  product_id: 0x53
   device_type: RGB
   name: SmartLIGHT Color Mesh 9W
   model: SMLm_c9i
   manufacturer: AwoX

-  product_id: 0x54
   device_type: RGB
   name: EGLOBulb A60 9W
   model: ESMLm_c9i
   manufacturer: EGLO

-  product_id: 0x55
   device_type: RGB
   name: Keria SmartLIGHT Color Mesh 9W
   model: KSMLm_c9i
   manufacturer: KERIA

-  product_id: 0x56
   device_type: RGB
   name: EGLOPanel 62X62
   model: EPanel_620
   manufacturer: EGLO

-  product_id: 0x57
   device_type: RGB
   name: EGLOPanel 45X45
   model: EPanel_450
   manufacturer: EGLO

-  product_id: 0x59
   device_type: RGB
   name: SmartLIGHT Color Mesh 13W Globe
   model: SMLm_c13gi
   manufacturer: AwoX

-  product_id: 0x5A
   device_type: RGB
   name: EGLOBulb G95 13W
   model: ESMLm_c13gi
   manufacturer: EGLO

-  product_id: 0x5B
   device_type: RGB
   name: Keria SmartLIGHT Color Mesh 13W Globe
   model: KSMLm_c13gi
   manufacturer: KERIA

-  product_id: 0x5C
   device_type: RGB
   name: SmartLIGHT Color Mesh 9W
   model: SSMLm_c9i
   manufacturer: AwoX

-  product_id: 0x64
   device_type: WHITE_TEMP
   name: SmartLIGHT White Mesh 9W
   model: SMLm_w9
   manufacturer: AwoX

-  product_id: 0x65
   device_type: WHITE_TEMP
   name: SmartLIGHT White Mesh 9W
   model: ESMLm_w9
   manufacturer: EGLO

-  product_id: 0x69
   device_type: RGB
   name: Ceiling GIRON 60
   model: ECeil_g60
   manufacturer: EGLO

-  product_id: 0x6A
   device_type: WHITE_TEMP
   name: SmartLIGHT Bulb A60 Warm
   model: SMLm_w9w
   manufacturer: AwoX

-  product_id: 0x6F
   device_type: WHITE_TEMP
   name: EGLOBulb Filament G80
   model: ESMLFm-w6-G80
   manufacturer: EGLO

-  product_id: 0x71
   device_type: WHITE_TEMP
   name: EGLOBulb Filament ST64
   model: ESMLFm-w6-ST64
   manufacturer: EGLO

-  product_id: 0x75
   device_type: WHITE_TEMP
   name: EGLOBulb Filament G95
   model: ESMLFm-w6-G95
   manufacturer: EGLO

-  product_id: 0x77
   device_type: RGB
   name: EGLO Spot
   model: ESpot_c5
   manufacturer: EGLO

-  product_id: 0x78
   device_type: RGB
   name: EGLO Fraioli
   model: EFraioli_c17
   manufacturer: EGLO

-  product_id: 0x79
   device_type: RGB
   name: EGLO Frattina
   model: EFrattina_c18
   manufacturer: EGLO

-  product_id: 0x7A
   device_type: RGB
   name: EGLO Frattina
   model: EFrattina_c27
   manufacturer: EGLO

-  product_id: 0x7B
   device_type: RGB
   name: EGLOPanel 30 Round
   model: EPanel_r300
   manufacturer: EGLO

-  product_id: 0x7C
   device_type: RGB
   name: EGLOPanel 45 Round
   model: EPanel_r450
   manufacturer: EGLO

-  product_id: 0x7D
   device_type: RGB
   name: EGLOPanel 60 Round
   model: EPanel_r600
   manufacturer: EGLO

-  product_id: 0x7E
   device_type: RGB
   name: EGLOPanel 10X120
   model: EPanel_120_10
   manufacturer: EGLO

-  product_id: 0x80
   device_type: WHITE_TEMP
   name: EPanel white round
   model: EPanel_w_round
   manufacturer: EGLO

-  product_id: 0x81
   device_type: WHITE_TEMP
   name: EPanel white square
   model: EPanel_w_square
   manufacturer: EGLO

-  product_id: 0x82
   device_type: WHITE_TEMP
   name: EPanel white rectangle
   model: EPanel_w_rect
   manufacturer: EGLO

-  product_id: 0x83
   device_type: WHITE_TEMP
   name: ECeiling white round
   model: ECeiling-w
   manufacturer: EGLO

-  product_id: 0x87
   device_type: WHITE_TEMP
   name: EGLO Tunable White
   model: EDoubleWhite
   manufacturer: EGLO

-  product_id: 0x88
   device_type: RGB
   name: EGLO Ceiling GIRON 80
   model: ECeil_g80
   manufacturer: EGLO

-  product_id: 0x89
   device_type: WHITE_TEMP
   name: Outdoor Marchesa-C
   model: EMarchesa_C
   manufacturer: EGLO

-  product_id: 0x8A
   device_type: WHITE_TEMP
   name: Outdoor Francari-C
   model: EFrancari_C
   manufacturer: EGLO

-  product_id: 0x92
   device_type: RGB
   name: EPanel square
   model: EPanel_36W_square
   manufacturer: EGLO

-  product_id: 0x94
   device_type: DIM
   name: EGLOBulb Filament ST64
   model: ESMLFm-w6w-ST64
   manufacturer: EGLO

-  product_id: 0x95
   device_type: DIM
   name: EGLOBulb Filament G95
   model: ESMLFm-w6w-G95
   manufacturer: EGLO

-  product_id: 0x96
   device_type: RGB
   name: EGLO RGB+TW
   model: EGLO-RGB-TW
   manufacturer: EGLO

-  product_id: 0x97
   device_type: WHITE_TEMP
   name: EGLO Tunable White
   model: EGLO-TW
   manufacturer: EGLO

-  product_id: 0x99
   device_type: RGB
   name: EGLO RGB+TW
   model: EGLO-RGB-TW
   manufacturer: EGLO

-  product_id: 0x9A
   device_type: WHITE_TEMP
   name: EGLO Tunable White
   model: JBT_Gen_CCT_1
   manufacturer: EGLO

-  product_id: 0x9B
   device_type: DIM
   name: EGLO Tunable White
   model: JBT_Gen_Dim_1
   manufacturer: EGLO

-  product_id: 0xA1
   device_type: RGB
   name: EGLOLed Relax
   model: ELedRelax
   manufacturer: EGLO

-  product_id: 0xA2
   device_type: RGB
   name: EGLOLed Stripe
   model: ELedStripe
   manufacturer: EGLO

-  product_id: 0xA3
   device_type: RGB
   name: EGLOLed Plus
   model: ELedPlus
   manufacturer: EGLO

-  product_id: 0xA4
   device_type: WHITE_TEMP
   name: EGLOLed Plus TW
   model: ELedPlus-TW
   manufacturer: EGLO

-  product_id: 0xA5
   device_type: DIM
   name: EGLOLed Plus Dimmable
   model: ELedPlus-Dimm
   manufacturer: EGLO

-  product_id: 0xA6
   device_type: WHITE_TEMP
   name: EGLOBulb
   model: ESMLFm-w6-TW
   manufacturer: EGLO

-  product_id: 0xA7
   device_type: DIM
   name: EGLOBulb
   model: ESMLFm-w6-Dimm
   manufacturer: EGLO

-  product_id: 0xA8
   device_type: RGB
   name: ECeiling square
   model: ECeiling-24W-square
   manufacturer: EGLO

-  product_id: 0xA9
   device_type: RGB
   name: EGLO RGB+TW
   model: EGLO-RGB-TW-IPSU
   manufacturer: EGLO

-  product_id: 0xAA
   device_type: WHITE_TEMP
   name: EGLO Tunable White
   model: EGLO-TW-IPSU
   manufacturer: EGLO

-  product_id: 0xAC
   device_type: RGB
   name: EGLO frameless
   model: EPanel-Frameless
   manufacturer: EGLO

-  product_id: 0xAD
   device_type: WHITE_TEMP
   name: EGLO Tunable White
   model: EDoubleWhite-ipsu
   manufacturer: EGLO

-  product_id: 0x97
   device_type: PLUG
   name: EGLO PLUG PLUS
   model: SMPWBm10AUS
   manufacturer: EGLO
   icon: mdi:power-socket-au

-  product_id: 0x9E
   device_type: PLUG
   name: EGLO PLUG PLUS
   model: SMPWBm10AUSb
   manufacturer: EGLO
   icon: mdi:power-socket-au

-  product_id: 0x90
   device_type: PLUG
   name: EGLO PLUG PLUS
   model: SMPWBm10CH
   manufacturer: EGLO
   icon: mdi:power-socket-ch

-  product_id: 0xA0
   device_type: PLUG
   name: EGLO PLUG PLUS
   model: SMPWBm10CHb
   manufacturer: EGLO
   icon: mdi:power-socket-ch

-  product_id: 0x67
   device_type: PLUG
   name: EGLO PLUG PLUS
   model: SMPWBm10FR
   manufacturer: EGLO
   icon: mdi:power-socket-fr

-  product_id: 0x84
   device_type: PLUG
   name: EGLO PLUG PLUS
   model: SMPWBm10FRa
   manufacturer: EGLO
   icon: mdi:power-socket-fr

-  product_id: 0x9C
   device_type: PLUG
   name: EGLO PLUG PLUS
   model: SMPWBm10FRb
   manufacturer: EGLO
   icon: mdi:power-socket-fr

-  product_id: 0x68
   device_type: PLUG
   name: EGLO PLUG PLUS
   model: SMPWBm10GE
   manufacturer: EGLO
   icon: mdi:power-socket-de

-  product_id: 0x85
   device_type: PLUG
   name: EGLO PLUG PLUS
   model: SMPWBm10GEa
   manufacturer: EGLO
   icon: mdi:power-socket-de

-  product_id: 0x9D
   device_type: PLUG
   name: EGLO PLUG PLUS
   model: SMPWBm10GEb
   manufacturer: EGLO
   icon: mdi:power-socket-de

-  product_id: 0x8F
   device_type: PLUG
   name:  EGLO PLUG PLUS
   model:  SMPWBm10UK
   manufacturer: EGLO
   icon: mdi:power-socket-uk

-  product_id: 0x9F
   device_type: PLUG
   name: EGLO PLUG PLUS
   model: SMPWBm10UKb
   manufacturer: EGLO
   icon: mdi:power-socket-uk

-  product_id: 0x8B
   device_type: PLUG
   name: EGLO PLUG
   model: ESMP-Bm10-AUS
   manufacturer: EGLO
   icon: mdi:power-socket-au

-  product_id: 0x8D
   device_type: PLUG
   name: EGLO PLUG
   model: ESMP-Bm10-CH
   manufacturer: EGLO
   icon: mdi:power-socket-ch

-  product_id: 0x62
   device_type: PLUG
   name: EGLO PLUG
   model: ESMP-Bm10-FR
   manufacturer: EGLO
   icon: mdi:power-socket-fr

-  product_id: 0x63
   device_type: PLUG
   name: EGLO PLUG
   model: ESMP-Bm10-GE
   manufacturer: EGLO
   icon: mdi:power-socket-de

-  product_id: 0x8C
   device_type: PLUG
   name: EGLO PLUG
   model: ESMP-Bm10-UK
   manufacturer: EGLO
   icon: mdi:power-socket-uk

```
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
```

- `mesh_id` _(number)_ - The id of the device in the mesh, shown in the log.
- `name` _(string)_ - The name of the device in Home Assistant.
- `type` _(RGB, DIM, WHITE_TEMP, PLUG)_ - See [`device_type`](#device_type-rgb-dim-white_temp-plug).
- `area_id` _(OPTIONAL)_ - The id of an area defined under [`esphome: areas:`](https://esphome.io/components/esphome/#areas).
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


#### `device_info`

List of device type descriptions, used to suggest the `type`, `name` and `icon` of devices that are not configured yet.

###### _Example:_
```yaml
  device_info:
    - product_id: 0x32
      device_type: RGB
      manufacturer: EGLO
      name: Spot 120
      model: EGLOSpot 120/w
      icon: mdi:wall-sconce-flat
```

Unknown devices are suggested as a dimmable device (`DIM`).

##### `product_id` _(hex value)_

Each product type has a unique `product_id` ([see above](#devices) how to find the `product_id` of your devices).

##### `device_type` _(RGB, DIM, WHITE_TEMP, PLUG)_

- `RGB` - RGB device
- `DIM` - dimmable device
- `WHITE_TEMP` - Dimmable and adjustable white color device
- `PLUG` - Plug (or on/off) device

##### `manufacturer` _(string)_

The name of the product manufacturer.

######  _Example value `EGLO`_


##### `name`

The product name.

###### _Example value `Spot 120`_


##### `model`

The product model name.

###### _Example value `EGLOSpot 120/w`_

##### `icon` _(string representing a mdi icon - OPTIONAL)_

Added to the suggested entry for devices of this product. The icon of a configured device is set with `icon` on its entry in [`devices`](#devices-list-optional).

###### _Example value: `mdi:wall-sconce-flat`_


### General YAML / Setup remarks

- Enable the Home Assistant API (`api:`)
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
