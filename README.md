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

### AID Port Wiring (RJ-45, TIA-568-B)

| RJ-45 Pin | Wire Color    | Signal     |
|-----------|---------------|------------|
| 1         | White-Orange  | RS-485 A+  |
| 2         | Orange        | RS-485 B-  |
| 3         | White-Green   | RS-485 A+  |
| 4         | Blue          | RS-485 B-  |
| 5-8       | -             | 24VAC (DO NOT CONNECT) |

**WARNING:** Pins 5-8 carry 24VAC. Do not connect these to data or ground lines. Doing so risks blowing the 3A fuse or damaging the ABC board.

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

### Manual ESPHome Configuration

Reference this repository as a remote external component:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/espforge/esphome-waterfurnace
      ref: main
```

See `waterfurnace-esp32-s3.yaml` for a complete example configuration.

### Customizing (IZ2 Multi-Zone, etc.)

After adopting the device in the ESPHome Dashboard, you'll have a local YAML file that imports the base config via `packages:`. Add overrides directly in that file. For example, to configure IZ2 multi-zone, add additional climate entries:

```yaml
climate:
  - platform: waterfurnace
    name: "Zone 1"
    zone: 1
  - platform: waterfurnace
    name: "Zone 2"
    zone: 2
```

ESPHome merges list sections, so your additional entries are appended to the base config's climate list.

## Supported Features

### Climate
- Single zone and IZ2 multi-zone (up to 6 zones, auto-detected)
- Modes: Off, Heat, Cool, Auto (Heat/Cool), Emergency Heat (as Boost preset)
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

## Testing

Unit tests and integration tests are in `tests/`. The integration tests run the actual C++ protocol code against the [waterfurnace_aurora](https://github.com/ccutrer/waterfurnace_aurora) Ruby gem's ModBus server via Docker. See [tests/README.md](tests/README.md) for details.

```sh
# Unit tests (just needs g++)
cd tests && g++ -std=c++17 -I../components/waterfurnace -o test_protocol test_protocol.cpp ../components/waterfurnace/protocol.cpp && ./test_protocol

# Integration tests (needs Docker)
cd tests && docker compose up --build --abort-on-container-exit
```

## Development

### Local Development

For local development, create a `secrets.yaml` in the project root with your WiFi credentials, then use the dev config in `tests/`:

```sh
# Run all tests (unit, integration, config validation, compile)
tests/run_tests.sh

# Or compile and flash directly
cd tests && esphome run waterfurnace-test.yaml
```

The `tests/waterfurnace-test.yaml` overrides the component source to use local files via the `component_source` substitution.

### Release & Deployment

See **[RELEASES.md](RELEASES.md)** for complete documentation on:
- Creating releases
- Testing on a fork
- CI/CD workflows
- Troubleshooting deployments
