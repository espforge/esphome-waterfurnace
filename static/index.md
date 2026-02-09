# ESPHome WaterFurnace Aurora

Monitor and control your WaterFurnace Aurora series geothermal heat pump from [Home Assistant](https://www.home-assistant.io/) using [ESPHome](https://esphome.io/) and RS-485.

## Hardware Requirements

- **[Waveshare ESP32-S3-RS485-CAN](https://www.waveshare.com/esp32-s3-rs485-can.htm)** board
- RS-485 cable connected to the heat pump's **AID port**

### AID Port Wiring

| AID Pin | Signal | Connect to |
|---------|--------|------------|
| 1, 3    | A+     | RS-485 A   |
| 2, 4    | B-     | RS-485 B   |
| 5-8     | 24VAC  | **DO NOT CONNECT** |

> **Warning:** Pins 5-8 carry 24VAC. Do not short or connect these to the RS-485 board.

## Install Firmware

Connect the Waveshare board to your computer via USB-C and click the button below to flash the firmware directly from your browser.

<esp-web-install-button manifest="firmware/waterfurnace.manifest.json"></esp-web-install-button>

After flashing, the device will create a **"WaterFurnace Fallback"** WiFi network. Connect to it and configure your WiFi credentials through the captive portal.

## Source Code

For documentation, configuration options, and source code, visit the [GitHub repository](https://github.com/rwagoner/esphome-waterfurnace).

<script type="module" src="https://unpkg.com/esp-web-tools@10/dist/web/install-button.js?module"></script>
