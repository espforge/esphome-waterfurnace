# Tests

Two test suites verify the protocol implementation against the [waterfurnace_aurora](https://github.com/ccutrer/waterfurnace_aurora) Ruby gem.

## Unit Tests

`test_protocol.cpp` — 38 native C++ tests covering:

- CRC16 calculation (ModBus polynomial 0xA001)
- Frame building for functions 65, 66, 67, and 6
- Frame CRC validation
- Response parsing
- Register type conversions (signed, tenths, hundredths, boolean)
- 32-bit register assembly
- IZ2 zone bit extraction (mode, fan, setpoints, damper)
- Fault code lookup
- Polling register group definitions

### Run

```sh
cd tests
g++ -std=c++17 -I../components/waterfurnace -o test_protocol test_protocol.cpp ../components/waterfurnace/protocol.cpp
./test_protocol
```

## Integration Tests

`test_integration.cpp` — 36 tests that send ModBus requests to a Ruby mock server and verify responses using our actual C++ protocol code. No reimplementation — the test uses `build_read_ranges_request()`, `parse_register_values()`, `convert_register()`, `get_thermostat_ranges()`, and all other functions from `protocol.h` and `registers.h` directly.

The mock server runs the `waterfurnace_aurora` Ruby gem's `ModBus::TCPServer` with custom function code support (65/66/67), loaded with a known register fixture.

### Architecture

```
test_integration.cpp          Ruby mock (waterfurnace_aurora gem)
┌──────────────────┐          ┌─────────────────────┐
│ Our C++ code:    │          │ ModBus::TCPServer   │
│  protocol.h      │──TCP────-│  Aurora::ModBus::   │
│  registers.h     │          │    Server (func     │
│                  │          │    65/66/67)        │
│ RTU PDU →        │          │                     │
│   MBAP wrapper → │          │ ← sample_registers  │
│     TCP socket   │          │      .yml           │
└──────────────────┘          └─────────────────────┘
```

The C++ test builds RTU frames with our code, strips the slave address and CRC to get the PDU, wraps it in a ModBus TCP (MBAP) header, and sends over TCP. Responses are parsed with our `parse_register_values()`. The only data not from our C++ code is the expected fixture values.

### What's Tested

- **Function 65** (read ranges): system ID, thermostat, component detection, AXB, power, VS drive — using `get_system_id_ranges()`, `get_thermostat_ranges()`, etc.
- **Function 66** (read individual): thermostat config, sparse registers — using `get_thermostat_config_registers()`
- **Function 67** (write + readback): setpoint writes using `build_write_registers_request()`
- **Value interpretation**: temperature conversion via `convert_register(SIGNED_TENTHS)`, 32-bit power via `to_uint32()`, fault parsing, output bitmask constants

### Run

Requires Docker.

```sh
cd tests
docker compose up --build --abort-on-container-exit
```

To run just the mock server (e.g. for manual testing):

```sh
docker compose up -d mock
# Mock listens on localhost:5020 (mapped to container port 502)
```

## Fixture Data

`fixtures/sample_registers.yml` — simulates a 5-series VS unit with AXB, AWL thermostat, no IZ2. Used by both the Ruby mock server and as expected values in the integration test.

## Known Issues

The `waterfurnace_aurora` gem (as of v1.5.8) has a bug in `Aurora::ModBus::Server#process_func` for function 67 — it references `param` (singular) instead of `params` (plural) and doesn't iterate over multiple writes. The mock server patches this with a `Func67Fix` module prepended onto `ModBus::TCPServer`.
