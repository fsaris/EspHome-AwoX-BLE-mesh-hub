# ESPHome component: AwoX BLE mesh (mqtt) hub


> Devices not yet defined will show up as dimmable lights. Create an issue with the `product id` shown in the device model (mqtt discovery message/HomeAssistant device info) and the features of the device to get it added.
>

A ESPhome component (https://esphome.io/components/external_components.html#git) to create a MQTT hub for your AwoX BLE mesh devices.

For an example yaml see [`awox-ble-mesh-hub.yaml`](awox-ble-mesh-hub.yaml).

You will need your mesh credentials, easiest way to find them when comming from [AwoX MESH control component for Home Assistant](https://github.com/fsaris/home-assistant-awox) is to check the `/config/.storage/core.config_entries` file for `mesh_name` and `mesh_password`.

When setup the component will scan for AwoX BLE mesh devices and publish [discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery) messages for each device on MQTT. When using HomeAssistant the device will show up under the MQTT integration. And you can (re)name the devices there.

### Requirements
- ESP32 module
- ESPHome 2022.12.0 or newer
- MQTT broker

### Recomendations
- HomeAssistant 2022.12.0 or newer
- ESPHome add-on for HomeAssistant


### References
- https://github.com/fsaris/home-assistant-awox
- https://github.com/Leiaz/python-awox-mesh-light
- https://github.com/vpaeder/telinkpp
