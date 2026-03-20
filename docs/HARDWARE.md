# Hardware Setup & Wiring

## Recommended: Waveshare ESP32-S3-RS485-CAN

The **[Waveshare ESP32-S3-RS485-CAN](https://www.waveshare.com/esp32-s3-rs485-can.htm)** board has a built-in RS-485 transceiver, so no external MAX485 module is needed. Just connect the board to the heat pump's AID port with an RJ-45 cable.

| Pin    | Function            |
| :----- | :------------------ |
| GPIO17 | UART TX             |
| GPIO18 | UART RX             |
| GPIO21 | RS-485 Flow Control |

The board accepts 7-36V DC input, so it can be powered from the AID port's 24VAC via a [24VAC to 12VDC converter](https://www.amazon.com/UHPPOTE-AC16-28V-Convertor-Surveillance-Security/dp/B01MDPAEMZ/).

## Alternative: ESP32/ESP8266 + MAX485

For other boards, you need:

1. **ESP32 or ESP8266 Development Board** (e.g., Wemos D1 Mini, NodeMCU, ESP32-DevKit)
2. **TTL to RS485 Adapter** with manual DE/RE flow control pins (see below)
3. **RJ45 Connector/Cable** to connect to the AID Tool port on the heat pump

### Recommended RS485 Module

Use a MAX485 module with **exposed DE/RE pins** for manual flow control. A known working module is the [Alinan MAX485 RS485 Transceiver Module](https://www.amazon.com/dp/B00NIOLNAG).

> **Important**: Avoid RS485 modules with "automatic flow control" that only have VCC/TXD/RXD/GND pins. These lack the DE/RE pins needed for reliable Modbus RTU timing and will cause communication errors.

### Wiring (ESP32 + MAX485)

```
  RJ45 Plug                              MAX485          ESP
  (to heat pump)     Ethernet Cable      Module          Board
  +-----------+                          +---------+     +-----------+
  |Pin 1  A+  |-- White/Orange --+       |         |     |           |
  |Pin 2  B-  |-- Orange --------+       |         |     |           |
  |Pin 3  A+  |-- White/Green ---+------>| A+      |     |           |
  |Pin 4  B-  |-- Blue ----------+------>| B-      |     |           |
  |           |                          |         |     |           |
  |Pin 5   R  |.. White/Blue  \          |         |     |           |
  |Pin 6   C  |.. Green        \         |         |     |           |
  |Pin 7   R  |.. White/Brown  / CUT &   |         |     |           |
  |Pin 8   C  |.. Brown       /  TAPE!   |         |     |           |
  +-----------+                          |         |     |           |
               DO NOT USE pins 5-8!      | DI (TX) |---->| GPIO_TX   |
               24VAC - DANGER!           | RO (RX) |---->| GPIO_RX   |
                                         | DE + RE |---->| GPIO_FLOW |
                                         | VCC     |---->| 3.3V      |
                                         | GND     |---->| GND       |
                                         +---------+     +-----------+
```

## AID Tool Port Pinout

The AID Tool port on the front of your heat pump uses an RJ45 jack. The 8 pins carry **two completely different buses** — RS-485 data and 24VAC thermostat power. You **must** understand which is which before connecting anything.

### RJ45 Plug Pin Numbering

When building your cable, you need to know how pin numbers map to wire positions. Hold the RJ45 plug with the **clip on the bottom** (facing away from you) and the gold contacts facing away from you. **Pin 1 is on the left**.

The first picture on this link gives a good visual (https://www.thetechmentor.com/posts/easy-rj45-wiring-with-rj45-pinout-pic/) has a good visual

<img width="600" height="300" alt="image" src="https://github.com/user-attachments/assets/6f0b67b6-e0eb-49c7-b8e0-48ca1689c62c" />

### Pin Assignment

```
  Pin   Signal   Wire Color (T568B)   Function
  ---   ------   ------------------   -------------------------
   1      A+     White/Orange         RS-485 data  <- CONNECT
   2      B-     Orange               RS-485 data  <- CONNECT
   3      A+     White/Green          RS-485 data  <- CONNECT
   4      B-     Blue                 RS-485 data  <- CONNECT
   5      R      White/Blue           24VAC power  !! DO NOT USE
   6      C      Green                24VAC power  !! DO NOT USE
   7      R      White/Brown          24VAC power  !! DO NOT USE
   8      C      Brown                24VAC power  !! DO NOT USE
```

Pins 1 & 3 are both A+ (tied together), and pins 2 & 4 are both B- (tied together). This is redundant by design — you only need one pair, but connecting both is fine.

> ### DANGER: 24VAC Power on Pins 5, 6, 7, 8
>
> Pins 5-8 carry **24VAC thermostat bus power** (C and R lines). These are **NOT** data pins.
>
> **DO NOT connect these pins to anything** — not to your RS-485 adapter, not to your ESP, not to ground, not to each other. If you short a 24VAC pin against a data pin, a ground wire, or anything else:
>
> - **Best case**: You blow the 3A automotive fuse inside the heat pump (replaceable, but you'll need to find it)
> - **Worst case**: You destroy the ABC (Aurora Base Control) board — a very expensive repair on a $40K+ heat pump
>
> If you're cutting an ethernet cable, strip and connect only the white/orange, orange, white/green, and blue wires. **Cut the remaining four wires short and tape or heat-shrink them** so they can't accidentally touch anything.

## Wiring Summary

| Heat Pump (RJ45) | Wire Color (T568B) | RS485 Adapter | ESP32/8266 |
| :--- | :--- | :--- | :--- |
| Pin 1, 3 (RS485 A+) | White/Orange, White/Green | A / + | - |
| Pin 2, 4 (RS485 B-) | Orange, Blue | B / - | - |
| Pin 5-8 (24VAC) | *(cut and insulate!)* | **DO NOT CONNECT** | **DO NOT CONNECT** |
| - | - | DI (TX) | GPIO_TX |
| - | - | RO (RX) | GPIO_RX |
| - | - | DE + RE (tied) | GPIO_FLOW |
| - | - | VCC | 3.3V / 5V |
| - | - | GND | GND |

## Flow Control

The DE/RE pins on most RS485 modules need to be tied together and connected to a GPIO for flow control. The component automatically switches this pin HIGH during transmit and LOW during receive.

RS-485 is half-duplex, meaning only one device can talk at a time. The flow control pin tells the MAX485 transceiver when to switch between transmit and receive mode. The component handles this automatically with proper timing (500 microsecond turnaround delay after transmit).

## Communication Settings

| Parameter | Value |
| :--- | :--- |
| **Baud Rate** | 19200 |
| **Data Bits** | 8 |
| **Parity** | Even |
| **Stop Bits** | 1 |
| **Default Address** | 1 |

## ESP8266 Notes

The ESP8266 (including D1 Mini / NodeMCU) is supported but requires special configuration:

1. **Logger baud rate**: The ESP8266 has only one fully functional UART. You **must** set `logger: baud_rate: 0` to disable serial logging so the UART is available for RS-485 communication:

   ```yaml
   logger:
     baud_rate: 0
   ```

2. **Pin selection**: The default GPIO pins are for the Waveshare board. For ESP8266, use appropriate pins (e.g., GPIO1/GPIO3 for hardware UART, or software serial pins).

3. **Memory**: The ESP8266 has limited RAM. If you experience stability issues, consider disabling sensors you don't need by commenting them out in your YAML.

## Community Hardware Projects

- [**Project Box for WaterFurnace Aurora**](https://github.com/benpeart/esphome_waterfurnace_aurora_projectbox) by [Ben Peart](https://github.com/benpeart) — Parts list, assembly instructions, and a 3D-printable project box to house the ESP8266 + MAX485 module.
