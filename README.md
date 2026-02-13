# ESPHome WaterFurnace Aurora Component

ESPHome external component for WaterFurnace Aurora heat pumps. Communicates via the AID port using ModBus RTU with custom function codes over RS-485.

## Background

This project's ModBus protocol implementation is derived from the [waterfurnace_aurora](https://github.com/ccutrer/waterfurnace_aurora) Ruby gem by Cody Cutrer. That project provides a comprehensive Ruby library and MQTT bridge for WaterFurnace systems, requiring a Raspberry Pi with a USB RS-485 adapter and an MQTT broker:

```
WaterFurnace -> USB RS-485 Adapter -> Raspberry Pi -> MQTT Broker -> Home Assistant
```

This ESPHome component provides a simpler integration path using a single ESP32-S3 device that connects directly to Home Assistant with no intermediate services:

```
WaterFurnace -> Waveshare ESP32-S3 -> Home Assistant
```

The waterfurnace_aurora gem's register documentation and protocol details were invaluable in building this component. See [NOTICE](NOTICE) for attribution.

## Hardware

**Target board:** [Waveshare ESP32-S3-RS485-CAN](https://www.waveshare.com/esp32-s3-rs485-can.htm)
- ESP32-S3, 16MB flash
- RS-485 with hardware auto-direction control, TVS/ESD protection, 120 ohm termination (jumper)
- 7-36V DC input + USB-C

**RS-485 GPIO pins:**
| Signal | GPIO |
|--------|------|
| TX     | 17   |
| RX     | 18   |
| DE     | 21   |

The Waveshare board handles DE (direction enable) automatically via the ESP32-S3's UART RS-485 half-duplex mode. No `flow_control_pin` configuration is needed unless you're using a different board.

### AID Port Wiring (RJ-45)

A standard Ethernet cable can be used to connect the AID port to your RS-485 adapter. Only pins 1-4 (data) need to be connected; pins 5-8 carry 24VAC and should not be connected to your device.

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

If making a custom cable, the color code above keeps each signal on its own twisted pair: A+ on the white-stripe wires (pins 1, 3) and B- on the solid wires (pins 2, 4), with 24VAC R and C each on their own pairs.

> **WARNING:** Pins 5-8 carry 24VAC. Do not connect these to data or ground lines. Doing so risks blowing the 3A fuse or damaging the ABC board. Please note that all modifications are performed at your own risk; the author assumes no liability for hardware damage.

### Powering from the AID Port

The 24VAC on pins 5-8 can be used to power the ESP32 with a [24VAC to 12VDC converter](https://www.amazon.com/UHPPOTE-AC16-28V-Convertor-Surveillance-Security/dp/B01MDPAEMZ/), eliminating the need for a separate power supply. The Waveshare board accepts 7-36V DC input.

### RS-485 Adapter Note

MAX485-based adapters are **not supported** due to poor auto-direction timing. The Waveshare ESP32-S3-RS485-CAN uses proper isolated RS-485 with hardware direction control.

## Requirements

- ESP32-S3 with RS-485 transceiver (Waveshare ESP32-S3-RS485-CAN recommended)
- ESPHome with ESP-IDF framework (for manual installs)

## Installation

### One-Click Web Install (Recommended)

Visit **[espforge.github.io/esphome-waterfurnace](https://espforge.github.io/esphome-waterfurnace/)** and click the Install button to flash firmware directly from your browser via USB. No ESPHome installation required.

After flashing, you'll be prompted to enter your WiFi credentials. The device uses Improv Serial for provisioning — no separate AP or captive portal needed.

### ESPHome Dashboard Import

If you use the ESPHome Dashboard, adopt the device after it connects to your network. It will automatically import the configuration from this repository.

After adopting, add a climate section to your device YAML to enable thermostat control (see [Adding Climate Control](#adding-climate-control) below).

### Manual ESPHome Configuration

Reference this repository as a remote external component:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/espforge/esphome-waterfurnace
      ref: main
```

See `waterfurnace-esp32-s3.yaml` and `waterfurnace-esp32-s3.factory.yaml` for complete example configurations. You will also need to add a climate section (see [Adding Climate Control](#adding-climate-control) below).

### Adding Climate Control

Add a climate section to your device YAML to enable thermostat control.

**Single zone (no IZ2):**
```yaml
climate:
  - platform: waterfurnace
    name: "Heat Pump"
```

**IZ2 multi-zone:**
```yaml
climate:
  - platform: waterfurnace
    name: "Zone 1"
    zone: 1
  - platform: waterfurnace
    name: "Zone 2"
    zone: 2
```

Up to 6 zones are supported. The component auto-detects whether IZ2 is installed.

## Supported Features

### Climate
- Single zone and IZ2 multi-zone (up to 6 zones, auto-detected)
- Modes: Off, Heat, Cool, Auto (Heat/Cool), Emergency Heat (E-Heat custom preset, zone 1 only)
- Fan modes: Auto, Continuous, Intermittent
- Two-point setpoints (heating + cooling)

### Sensors
Temperature, pressure, power, current, humidity, compressor speed, waterflow, heat of extraction/rejection, and more.

### Binary Sensors
Compressor, blower, aux heat stages, reversing valve, lockout, alarm status from the system outputs register.

### Switches
DHW (Domestic Hot Water) enable/disable.

### Text Sensors
Model number, serial number, current fault code with description, system operating mode.

## Protocol

Uses ModBus RTU with WaterFurnace custom function codes:
- **Function 65:** Read multiple discontiguous register ranges
- **Function 66:** Read multiple discontiguous individual registers
- **Function 67:** Write multiple discontiguous registers

Communication: 19200 baud, 8 data bits, even parity, 1 stop bit. Slave address 1.

## Component Detection

On startup, the hub reads component status registers to detect installed equipment:
- Thermostat / AWL thermostat
- AXB (Advanced Extension Board) for performance and energy data
- IZ2 (IntelliZone 2) for multi-zone support
- VS Drive (Variable Speed compressor)

Polling groups are automatically configured based on detected components.

## Development & Testing

See **[DEVELOPMENT.md](DEVELOPMENT.md)** for local development, testing, and release documentation.
