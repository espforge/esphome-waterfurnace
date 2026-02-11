// Integration test: uses our actual C++ protocol code to talk to the Ruby
// ModBus TCP mock server, verifying the protocol implementation end-to-end.
//
// ModBus TCP wraps the same PDU as RTU but with an MBAP header instead of
// slave_addr + CRC. This test extracts the PDU from our RTU frame builders
// and wraps it in MBAP for TCP transport, then uses our parse functions on
// the response. This tests the real code path - no reimplementation.
//
// Compile: g++ -std=c++17 -I../components/waterfurnace -o test_integration test_integration.cpp ../components/waterfurnace/protocol.cpp
// Run:     docker compose up -d mock && ./test_integration localhost 5020

#include "protocol.h"
#include "registers.h"

#include <arpa/inet.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

using namespace esphome::waterfurnace;

static int tests_passed = 0;
static int tests_failed = 0;
static uint16_t txn_id = 0;

#define TEST(name, condition) do { \
    bool _ok = (condition); \
    printf("  %-55s %s\n", name, _ok ? "PASS" : "FAIL"); \
    if (_ok) tests_passed++; else tests_failed++; \
  } while(0)

// --------------------------------------------------------------------------
// TCP transport: converts our RTU frames to/from ModBus TCP (MBAP)
// --------------------------------------------------------------------------

// Extract PDU from RTU frame: strip slave_addr (byte 0) and CRC (last 2 bytes)
static std::vector<uint8_t> rtu_to_pdu(const std::vector<uint8_t> &rtu_frame) {
  // RTU: [slave][func][data...][crc_lo][crc_hi]
  // PDU: [func][data...]
  return std::vector<uint8_t>(rtu_frame.begin() + 1, rtu_frame.end() - 2);
}

// Send a PDU over ModBus TCP (wraps in MBAP header) and return response PDU
static std::vector<uint8_t> send_modbus_tcp(int sock, const std::vector<uint8_t> &pdu) {
  txn_id++;

  // MBAP header: txn_id(2) + protocol(2) + length(2) + unit_id(1)
  uint16_t length = pdu.size() + 1;  // +1 for unit_id
  uint8_t header[7];
  header[0] = (txn_id >> 8) & 0xFF;
  header[1] = txn_id & 0xFF;
  header[2] = 0; header[3] = 0;  // protocol = 0
  header[4] = (length >> 8) & 0xFF;
  header[5] = length & 0xFF;
  header[6] = SLAVE_ADDRESS;

  // Send header + PDU
  std::vector<uint8_t> msg(header, header + 7);
  msg.insert(msg.end(), pdu.begin(), pdu.end());
  send(sock, msg.data(), msg.size(), 0);

  // Read response header (7 bytes)
  uint8_t resp_header[7];
  size_t got = 0;
  while (got < 7) {
    ssize_t n = recv(sock, resp_header + got, 7 - got, 0);
    if (n <= 0) { fprintf(stderr, "Connection lost\n"); return {}; }
    got += n;
  }

  uint16_t resp_len = (resp_header[4] << 8) | resp_header[5];
  size_t remaining = resp_len - 1;  // subtract unit_id

  // Read response PDU
  std::vector<uint8_t> resp_pdu(remaining);
  got = 0;
  while (got < remaining) {
    ssize_t n = recv(sock, resp_pdu.data() + got, remaining - got, 0);
    if (n <= 0) { fprintf(stderr, "Connection lost\n"); return {}; }
    got += n;
  }

  return resp_pdu;
}

// Build RTU frame using our C++ code, convert to PDU, send via TCP, return response
static std::vector<uint16_t> read_ranges(int sock, const std::vector<std::pair<uint16_t, uint16_t>> &ranges) {
  auto rtu = build_read_ranges_request(ranges);
  auto pdu = rtu_to_pdu(rtu);
  auto resp = send_modbus_tcp(sock, pdu);

  if (resp.empty() || is_error_response(resp[0])) {
    fprintf(stderr, "Error response for func 65\n");
    return {};
  }

  // resp[0] = func, resp[1] = byte_count, resp[2..] = data
  uint8_t byte_count = resp[1];
  return parse_register_values(resp.data() + 2, byte_count);
}

static std::vector<uint16_t> read_registers(int sock, const std::vector<uint16_t> &addresses) {
  auto rtu = build_read_registers_request(addresses);
  auto pdu = rtu_to_pdu(rtu);
  auto resp = send_modbus_tcp(sock, pdu);

  if (resp.empty() || is_error_response(resp[0])) {
    fprintf(stderr, "Error response for func 66\n");
    return {};
  }

  uint8_t byte_count = resp[1];
  return parse_register_values(resp.data() + 2, byte_count);
}

static bool write_registers(int sock, const std::vector<std::pair<uint16_t, uint16_t>> &writes) {
  auto rtu = build_write_registers_request(writes);
  auto pdu = rtu_to_pdu(rtu);
  auto resp = send_modbus_tcp(sock, pdu);

  if (resp.empty() || is_error_response(resp[0])) {
    fprintf(stderr, "Error response for func 67\n");
    return false;
  }
  return resp[0] == FUNC_WRITE_REGISTERS;
}

// Compute expected values for a range read from known fixture data
// This maps register address -> expected value (from sample_registers.yml)
static uint16_t fixture_value(uint16_t addr) {
  // Hard-coded from fixtures/sample_registers.yml to avoid YAML dependency.
  // This is the ONLY duplication - the register fixture data.
  // All protocol logic uses our actual C++ code.
  static const std::pair<uint16_t, uint16_t> data[] = {
    {2, 705}, {33, 0},
    {88, 16706}, {89, 17235}, {90, 20556}, {91, 22099},
    {92, 20308}, {93, 20533}, {94, 12345},
    {105, 12345},
    {400, 1}, {401, 1200}, {404, 1}, {412, 60}, {413, 3},
    {800, 1}, {801, 300}, {802, 100},
    {806, 1}, {807, 200}, {808, 100},
    {812, 3}, {815, 3}, {818, 3},
    {824, 1}, {825, 100}, {827, 1}, {828, 200},
    {483, 0},
    {16, 240},
    {19, 850}, {20, 680}, {25, 0}, {26, 0},
    {30, 9}, {31, 0}, {344, 0}, {362, 0}, {502, 350},
    {740, 705}, {741, 45}, {742, 320},
    {745, 680}, {746, 750}, {747, 710},
    {12005, 0}, {12006, 256},
    {900, 920},
    {1103, 0}, {1104, 1}, {1105, 32}, {1106, 0}, {1107, 85}, {1108, 0},
    {1109, 320}, {1110, 950}, {1111, 450}, {1112, 150}, {1113, 400},
    {1114, 1150}, {1115, 3500}, {1116, 680}, {1117, 50}, {1118, 0}, {1119, 250},
    {1124, 85}, {1125, 120},
    {1134, 30}, {1135, 450}, {1136, 400},
    {1146, 0}, {1147, 3500}, {1148, 0}, {1149, 450},
    {1150, 0}, {1151, 0}, {1152, 0}, {1153, 3950},
    {1154, 0}, {1155, 28000}, {1156, 0}, {1157, 40000},
    {1164, 0}, {1165, 200},
    {3000, 3200}, {3001, 3150},
    {3220, 0}, {3221, 0}, {3222, 0}, {3223, 0},
    {3224, 0}, {3225, 0}, {3226, 0}, {3227, 0},
    {3322, 3500}, {3323, 680}, {3324, 0}, {3325, 1650},
    {3326, 0}, {3327, 0}, {3328, 0}, {3329, 0}, {3330, 0},
    {3522, 950}, {3524, 2800},
  };
  for (const auto &d : data) {
    if (d.first == addr) return d.second;
  }
  return 0;  // unset registers default to 0
}

static std::vector<uint16_t> expected_range_values(
    const std::vector<std::pair<uint16_t, uint16_t>> &ranges) {
  std::vector<uint16_t> values;
  for (const auto &r : ranges) {
    for (uint16_t i = 0; i < r.second; i++) {
      values.push_back(fixture_value(r.first + i));
    }
  }
  return values;
}

static std::vector<uint16_t> expected_individual_values(
    const std::vector<uint16_t> &addresses) {
  std::vector<uint16_t> values;
  for (uint16_t addr : addresses) {
    values.push_back(fixture_value(addr));
  }
  return values;
}

// Decode string from register values (same logic as our waterfurnace.cpp)
static std::string decode_string(const std::vector<uint16_t> &values, size_t offset, size_t num_regs) {
  std::string result;
  for (size_t i = 0; i < num_regs && (offset + i) < values.size(); i++) {
    uint16_t v = values[offset + i];
    char hi = (v >> 8) & 0xFF;
    char lo = v & 0xFF;
    if (hi) result += hi;
    if (lo) result += lo;
  }
  // Trim trailing spaces/nulls
  while (!result.empty() && (result.back() == ' ' || result.back() == '\0'))
    result.pop_back();
  return result;
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------

int main(int argc, char *argv[]) {
  const char *host = argc > 1 ? argv[1] : "localhost";
  int port = argc > 2 ? atoi(argv[2]) : 502;

  printf("Connecting to ModBus TCP mock at %s:%d...\n", host, port);

  // Connect with retries
  int sock = -1;
  for (int attempt = 0; attempt < 10; attempt++) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    struct hostent *he = gethostbyname(host);
    if (!he) {
      printf("  DNS resolution failed, retrying (%d/10)...\n", attempt + 1);
      close(sock);
      sleep(2);
      continue;
    }
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) break;
    printf("  Connection refused, retrying (%d/10)...\n", attempt + 1);
    close(sock);
    sock = -1;
    sleep(2);
  }
  if (sock < 0) {
    printf("  Could not connect to mock server\n");
    return 1;
  }

  // Set socket timeout
  struct timeval tv{5, 0};
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  printf("Connected.\n\n");

  // ============================================================
  printf("Function 65: Read Register Ranges (using build_read_ranges_request)\n");
  printf("============================================================\n");

  // Test 1: System ID registers - uses get_system_id_ranges() from registers.h
  {
    auto ranges = get_system_id_ranges();
    auto values = read_ranges(sock, ranges);
    auto expected = expected_range_values(ranges);
    TEST("System ID ranges: correct count", values.size() == expected.size());
    TEST("System ID ranges: values match", values == expected);

    // Decode program name using same logic as waterfurnace.cpp
    auto program = decode_string(values, 1, 4);  // offset 1 = reg 88, 4 regs
    TEST(("Program name = 'ABCSPLVS' (got '" + program + "')").c_str(),
         program == "ABCSPLVS");
  }

  // Test 2: Thermostat ranges - uses get_thermostat_ranges() from registers.h
  {
    auto ranges = get_thermostat_ranges();
    auto values = read_ranges(sock, ranges);
    auto expected = expected_range_values(ranges);
    TEST("Thermostat ranges: values match", values == expected);

    // Find specific register values from within the returned data
    size_t idx = 0;
    for (const auto &r : ranges) {
      for (uint16_t i = 0; i < r.second; i++, idx++) {
        uint16_t reg = r.first + i;
        if (reg == REG_HEATING_SETPOINT)
          TEST("  Heating SP reg 745 = 680 (68.0F)", values[idx] == 680);
        if (reg == REG_COOLING_SETPOINT)
          TEST("  Cooling SP reg 746 = 750 (75.0F)", values[idx] == 750);
        if (reg == REG_AMBIENT_TEMP)
          TEST("  Ambient reg 747 = 710 (71.0F)", values[idx] == 710);
        if (reg == REG_SYSTEM_OUTPUTS)
          TEST("  Outputs reg 30 = 9 (CC+Blower)", values[idx] == 9);
      }
    }
  }

  // Test 3: Component detection - uses get_component_detect_ranges()
  {
    auto ranges = get_component_detect_ranges();
    auto values = read_ranges(sock, ranges);
    auto expected = expected_range_values(ranges);
    TEST("Component detect ranges: values match", values == expected);

    // Verify component detection logic using our constants
    auto check = [](uint16_t s) {
      return s != COMPONENT_REMOVED && s != COMPONENT_MISSING && s != 0;
    };

    TEST("  Thermostat active", check(values[0]));
    float therm_ver = convert_register(values[1], RegisterType::HUNDREDTHS);
    TEST("  Thermostat AWL (version >= 3.0)", therm_ver >= 3.0f);

    TEST("  AXB active", check(values[3]));
    float axb_ver = convert_register(values[4], RegisterType::HUNDREDTHS);
    TEST("  AXB AWL (version >= 2.0)", axb_ver >= 2.0f);

    TEST("  IZ2 removed", !check(values[6]));
  }

  // Test 4: AXB ranges - uses get_axb_ranges()
  {
    auto ranges = get_axb_ranges();
    auto values = read_ranges(sock, ranges);
    auto expected = expected_range_values(ranges);
    TEST("AXB ranges: values match", values == expected);
  }

  // Test 5: Power ranges - uses get_power_ranges()
  {
    auto ranges = get_power_ranges();
    auto values = read_ranges(sock, ranges);
    auto expected = expected_range_values(ranges);
    TEST("Power ranges: values match", values == expected);

    // Use our to_uint32() for 32-bit power values
    TEST("  Line voltage = 240V", values[0] == 240);
    uint32_t comp_watts = to_uint32(values[1], values[2]);
    TEST("  Compressor power = 3500W", comp_watts == 3500);
    uint32_t total_watts = to_uint32(values[7], values[8]);
    TEST("  Total power = 3950W", total_watts == 3950);
  }

  // Test 6: VS Drive - uses get_vs_drive_ranges()
  {
    auto ranges = get_vs_drive_ranges();
    auto values = read_ranges(sock, ranges);
    auto expected = expected_range_values(ranges);
    TEST("VS Drive ranges: values match", values == expected);
  }

  // ============================================================
  printf("\nFunction 66: Read Individual Registers (using build_read_registers_request)\n");
  printf("============================================================\n");

  // Test: Thermostat config - uses get_thermostat_config_registers()
  {
    auto addrs = get_thermostat_config_registers();
    auto values = read_registers(sock, addrs);
    auto expected = expected_individual_values(addrs);
    TEST("Thermostat config: values match", values == expected);

    // Mode extraction: reg 12006 = 256 = 0x0100, mode = (0x0100 >> 8) & 0x07 = 1 = auto
    uint8_t mode = (values[1] >> 8) & 0x07;
    TEST("  Mode = auto (1)", mode == MODE_AUTO);

    // Fan extraction using same logic as our climate component
    uint8_t fan;
    if (values[0] & 0x80) fan = FAN_CONTINUOUS;
    else if (values[0] & 0x100) fan = FAN_INTERMITTENT;
    else fan = FAN_AUTO;
    TEST("  Fan = auto", fan == FAN_AUTO);
  }

  // Test: Sparse individual registers
  {
    std::vector<uint16_t> addrs = {REG_LINE_VOLTAGE, REG_HEATING_SETPOINT,
                                   REG_ENTERING_WATER, REG_VS_SPEED_ACTUAL};
    auto values = read_registers(sock, addrs);
    auto expected = expected_individual_values(addrs);
    TEST("Sparse individual reads: values match", values == expected);
  }

  // ============================================================
  printf("\nFunction 67: Write Registers (using build_write_registers_request)\n");
  printf("============================================================\n");

  // Test: Write setpoints
  {
    bool ok = write_registers(sock, {{REG_WRITE_HEATING_SP, 700},
                                     {REG_WRITE_COOLING_SP, 730}});
    TEST("Write response received (no error)", ok);

    // Read back using func 66
    auto values = read_registers(sock, {REG_WRITE_HEATING_SP, REG_WRITE_COOLING_SP});
    TEST("  Readback heating SP = 700", values.size() >= 2 && values[0] == 700);
    TEST("  Readback cooling SP = 730", values.size() >= 2 && values[1] == 730);
  }

  // ============================================================
  printf("\nValue Interpretation (using convert_register from registers.h)\n");
  printf("============================================================\n");

  // Temperature conversions using our convert_register()
  {
    auto values = read_registers(sock, {REG_ENTERING_WATER, REG_LEAVING_WATER,
                                        REG_OUTDOOR_TEMP, REG_LEAVING_AIR});

    float ewt = convert_register(values[0], RegisterType::SIGNED_TENTHS);
    float lwt = convert_register(values[1], RegisterType::SIGNED_TENTHS);
    float outdoor = convert_register(values[2], RegisterType::SIGNED_TENTHS);
    float leaving_air = convert_register(values[3], RegisterType::SIGNED_TENTHS);

    char buf[80];
    snprintf(buf, sizeof(buf), "  EWT = 45.0F (got %.1f)", ewt);
    TEST(buf, ewt > 44.9f && ewt < 45.1f);
    snprintf(buf, sizeof(buf), "  LWT = 95.0F (got %.1f)", lwt);
    TEST(buf, lwt > 94.9f && lwt < 95.1f);
    snprintf(buf, sizeof(buf), "  Outdoor = 32.0F (got %.1f)", outdoor);
    TEST(buf, outdoor > 31.9f && outdoor < 32.1f);
    snprintf(buf, sizeof(buf), "  Leaving Air = 92.0F (got %.1f)", leaving_air);
    TEST(buf, leaving_air > 91.9f && leaving_air < 92.1f);
  }

  // Fault code parsing using our constants
  {
    auto values = read_registers(sock, {REG_LAST_FAULT});
    bool locked_out = (values[0] & 0x8000) != 0;
    uint16_t fault_code = values[0] & 0x7FFF;
    TEST("  No fault (code=0, no lockout)", fault_code == 0 && !locked_out);
  }

  // System outputs bitmask using our constants
  {
    auto values = read_registers(sock, {REG_SYSTEM_OUTPUTS});
    uint16_t outputs = values[0];
    char buf[80];
    snprintf(buf, sizeof(buf), "  Compressor ON (outputs=0x%04X)", outputs);
    TEST(buf, (outputs & OUTPUT_CC) != 0);
    TEST("  Blower ON", (outputs & OUTPUT_BLOWER) != 0);
    TEST("  Reversing valve OFF (heating mode)", (outputs & OUTPUT_RV) == 0);
    TEST("  Not in lockout", (outputs & OUTPUT_LOCKOUT) == 0);
  }

  // ============================================================
  close(sock);
  printf("\n============================================================\n");
  printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
  return tests_failed > 0 ? 1 : 0;
}
