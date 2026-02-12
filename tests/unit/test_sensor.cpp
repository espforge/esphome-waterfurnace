// Unit tests for waterfurnace_sensor component

#include <gtest/gtest.h>
#include "../../components/waterfurnace/protocol.cpp"
#include "hub_stubs.h"
#include "../../components/waterfurnace/sensor/waterfurnace_sensor.cpp"

using namespace esphome;
using namespace esphome::waterfurnace;

// Expose protected methods for testing
class TestableSensor : public WaterFurnaceSensor {
 public:
  using WaterFurnaceSensor::on_register_value_;
  using WaterFurnaceSensor::on_register_value_hi_;
};

class SensorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_millis = 0;
    waterfurnace::clear_written_registers();
    hub_ = new TestableHub();
    sensor_ = new TestableSensor();
    sensor_->set_parent(hub_);
  }

  void TearDown() override {
    delete sensor_;
    delete hub_;
  }

  TestableHub *hub_;
  TestableSensor *sensor_;
};

// ====== Register Type Conversions via Sensor ======

TEST_F(SensorTest, UnsignedType) {
  sensor_->set_register_address(740);
  sensor_->set_register_type("unsigned");
  sensor_->setup();

  hub_->dispatch_register_(740, 240);
  EXPECT_FLOAT_EQ(sensor_->state, 240.0f);
}

TEST_F(SensorTest, SignedTenthsPositive) {
  sensor_->set_register_address(740);
  sensor_->set_register_type("signed_tenths");
  sensor_->setup();

  hub_->dispatch_register_(740, 700);  // 70.0°F
  EXPECT_FLOAT_EQ(sensor_->state, 70.0f);
}

TEST_F(SensorTest, SignedTenthsNegative) {
  sensor_->set_register_address(740);
  sensor_->set_register_type("signed_tenths");
  sensor_->setup();

  uint16_t neg = static_cast<uint16_t>(static_cast<int16_t>(-105));
  hub_->dispatch_register_(740, neg);
  EXPECT_FLOAT_EQ(sensor_->state, -10.5f);
}

TEST_F(SensorTest, TenthsType) {
  sensor_->set_register_address(745);
  sensor_->set_register_type("tenths");
  sensor_->setup();

  hub_->dispatch_register_(745, 735);  // 73.5
  EXPECT_FLOAT_EQ(sensor_->state, 73.5f);
}

TEST_F(SensorTest, SignedType) {
  sensor_->set_register_address(502);
  sensor_->set_register_type("signed");
  sensor_->setup();

  hub_->dispatch_register_(502, 0xFF9C);  // -100
  EXPECT_FLOAT_EQ(sensor_->state, -100.0f);
}

TEST_F(SensorTest, HundredthsType) {
  sensor_->set_register_address(2);
  sensor_->set_register_type("hundredths");
  sensor_->setup();

  hub_->dispatch_register_(2, 705);  // 7.05
  EXPECT_FLOAT_EQ(sensor_->state, 7.05f);
}

// ====== 32-bit Sensor ======

TEST_F(SensorTest, Uint32Value) {
  sensor_->set_register_address(1152);
  sensor_->set_register_type("uint32");
  sensor_->set_is_32bit(true);
  sensor_->setup();

  // hi word at 1152, lo word at 1153
  hub_->dispatch_register_(1152, 1);    // hi = 1
  hub_->dispatch_register_(1153, 500);  // lo = 500 -> 65536 + 500 = 66036

  EXPECT_FLOAT_EQ(sensor_->state, 66036.0f);
}

TEST_F(SensorTest, Int32NegativeValue) {
  sensor_->set_register_address(1154);
  sensor_->set_register_type("int32");
  sensor_->set_is_32bit(true);
  sensor_->setup();

  // -1000 as int32 = 0xFFFFFC18
  int32_t val = -1000;
  uint32_t uval = static_cast<uint32_t>(val);
  hub_->dispatch_register_(1154, uval >> 16);
  hub_->dispatch_register_(1155, uval & 0xFFFF);

  EXPECT_FLOAT_EQ(sensor_->state, -1000.0f);
}

TEST_F(SensorTest, Uint32WaitsForBothWords) {
  sensor_->set_register_address(1152);
  sensor_->set_register_type("uint32");
  sensor_->set_is_32bit(true);
  sensor_->setup();

  // Only send lo word - should not update
  hub_->dispatch_register_(1153, 500);
  EXPECT_TRUE(std::isnan(sensor_->state));
}

// ====== Change Detection (Dedup) ======

TEST_F(SensorTest, DedupSameValue) {
  sensor_->set_register_address(740);
  sensor_->set_register_type("unsigned");
  sensor_->setup();

  hub_->dispatch_register_(740, 100);
  EXPECT_FLOAT_EQ(sensor_->state, 100.0f);

  // Change to something else to verify dedup works
  sensor_->state = -999.0f;  // Manually corrupt to detect if publish is called

  // Same register value - should NOT publish (dedup)
  hub_->dispatch_register_(740, 100);
  EXPECT_FLOAT_EQ(sensor_->state, -999.0f);  // Not updated because dedup
}

TEST_F(SensorTest, DedupDifferentValue) {
  sensor_->set_register_address(740);
  sensor_->set_register_type("unsigned");
  sensor_->setup();

  hub_->dispatch_register_(740, 100);
  EXPECT_FLOAT_EQ(sensor_->state, 100.0f);

  hub_->dispatch_register_(740, 200);
  EXPECT_FLOAT_EQ(sensor_->state, 200.0f);
}

// ====== Sentinel Value Handling ======

TEST_F(SensorTest, NegativeSentinelSignedTenths) {
  sensor_->set_register_address(1116);
  sensor_->set_register_type("signed_tenths");
  sensor_->setup();

  // -9999 raw as uint16 -> signed_tenths -> -999.9 -> NaN
  uint16_t raw = static_cast<uint16_t>(static_cast<int16_t>(-9999));
  hub_->dispatch_register_(1116, raw);
  EXPECT_TRUE(std::isnan(sensor_->state));
}

TEST_F(SensorTest, PositiveSentinelSignedTenths) {
  sensor_->set_register_address(1117);
  sensor_->set_register_type("signed_tenths");
  sensor_->setup();

  // 9999 raw -> signed_tenths -> 999.9 -> NaN (e.g. waterflow not supported)
  hub_->dispatch_register_(1117, 9999);
  EXPECT_TRUE(std::isnan(sensor_->state));
}

TEST_F(SensorTest, PositiveSentinelTenths) {
  sensor_->set_register_address(1117);
  sensor_->set_register_type("tenths");
  sensor_->setup();

  // 9999 raw -> tenths -> 999.9 -> NaN
  hub_->dispatch_register_(1117, 9999);
  EXPECT_TRUE(std::isnan(sensor_->state));
}

TEST_F(SensorTest, ValidValueNotSentinel) {
  sensor_->set_register_address(1117);
  sensor_->set_register_type("signed_tenths");
  sensor_->setup();

  // 50 raw -> 5.0 gpm (valid, not sentinel)
  hub_->dispatch_register_(1117, 50);
  EXPECT_FLOAT_EQ(sensor_->state, 5.0f);
}
