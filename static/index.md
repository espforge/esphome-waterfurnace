# ESPHome WaterFurnace Aurora

Monitor and control your WaterFurnace Aurora series geothermal heat pump from [Home Assistant](https://www.home-assistant.io/) using [ESPHome](https://esphome.io/) and RS-485.

## Hardware Requirements

- **[Waveshare ESP32-S3-RS485-CAN](https://www.waveshare.com/esp32-s3-rs485-can.htm)** board
- RS-485 cable connected to the heat pump's **AID port**

### AID Port Wiring (RJ-45)

A standard Ethernet cable can be used. Only pins 1-4 (data) need to be connected to your RS-485 adapter.

| RJ-45 Pin | Signal          | Recommended Color | Notes |
|-----------|-----------------|-------------------|-------|
| 1         | RS-485 A+       | Orange-White      | |
| 2         | RS-485 B-       | Orange            | |
| 3         | RS-485 A+       | Green-White       | |
| 4         | RS-485 B-       | Green             | |
| 5         | 24VAC R (hot)   | Blue-White        | **DO NOT CONNECT** |
| 6         | 24VAC C (common)| Blue              | **DO NOT CONNECT** |
| 7         | 24VAC R (hot)   | Brown-White       | **DO NOT CONNECT** |
| 8         | 24VAC C (common)| Brown             | **DO NOT CONNECT** |

If making a custom cable, the colors above keep each signal on its own twisted pair.

> **Warning:** Pins 5-8 carry 24VAC. Do not connect these to data or ground lines. Doing so risks blowing the 3A fuse or damaging the ABC board. Please note that all modifications are performed at your own risk; the author assumes no liability for hardware damage.

### Powering from the AID Port

The 24VAC on pins 5-8 can power the ESP32 with a [24VAC to 12VDC converter](https://www.amazon.com/UHPPOTE-AC16-28V-Convertor-Surveillance-Security/dp/B01MDPAEMZ/), eliminating the need for a separate power supply. The Waveshare board accepts 7-36V DC input.

## Install Firmware

Connect the Waveshare board to your computer via USB-C and click the button below to flash the firmware directly from your browser.

<esp-web-install-button manifest="firmware/waterfurnace.manifest.json"></esp-web-install-button>

## Adding Climate Control

After adopting the device in the ESPHome dashboard, add the following to your device YAML to enable thermostat control:

```yaml
# Single zone (no IZ2):
climate:
  - platform: waterfurnace
    name: "Heat Pump"

# For IZ2 multi-zone, replace the above with:
# climate:
#   - platform: waterfurnace
#     name: "Zone 1"
#     zone: 1
#   - platform: waterfurnace
#     name: "Zone 2"
#     zone: 2
```

## Source Code

For documentation, configuration options, and source code, visit the [GitHub repository](https://github.com/espforge/esphome-waterfurnace).

<script type="module" src="https://unpkg.com/esp-web-tools@10/dist/web/install-button.js?module"></script>
