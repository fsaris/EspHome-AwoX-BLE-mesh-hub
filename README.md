# ESPHome component: AwoX BLE mesh (mqtt) hub

A ESPhome component (https://esphome.io/components/external_components.html#git) to create a MQTT hub for your AwoX BLE mesh devices.

For an example yaml see [`awox-ble-mesh-hub.yaml`](awox-ble-mesh-hub.yaml).

You will need your mesh credentials, easiest way to find them when comming from [AwoX MESH control component for Home Assistant](https://github.com/fsaris/home-assistant-awox) is to check the `/config/.storage/core.config_entries` file for `mesh_name` and `mesh_password`.

When setup the component will scan for AwoX BLE mesh devices and publish [discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery) messages for each device on MQTT. When using HomeAssistant the device will show up under the MQTT integration. And you can (re)name the devices there.

Each device type/product can be configured by `product_id` so it gets the correct features in HomeAssistant.

Unknown devices will be recognized as a dimmable light.

The `product id` needed for the configuration is shown in model string of the mqtt discovery message/HomeAssistant device info.

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

In case of using a WLAN with hidden SSID, mind to use the multiple netowrk option, to define the hidden variable of the wifi network: [Connecting to Multiple Networks](https://esphome.io/components/wifi.html#connecting-to-multiple-networks)


### Requirements
- ESP32 module
- ESPHome 2023.2.0 or newer
- MQTT broker

### Recomendations
- HomeAssistant 2023.8.0 or newer
- ESPHome add-on for HomeAssistant


### References
- https://github.com/fsaris/home-assistant-awox
- https://github.com/Leiaz/python-awox-mesh-light
- https://github.com/vpaeder/telinkpp
