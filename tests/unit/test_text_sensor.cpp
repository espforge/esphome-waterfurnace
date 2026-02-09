// Unit tests for waterfurnace_text_sensor component

#include <gtest/gtest.h>
#include "../../components/waterfurnace/protocol.cpp"
#include "hub_stubs.h"
#include "../../components/waterfurnace/text_sensor/waterfurnace_text_sensor.cpp"

using namespace esphome;
using namespace esphome::waterfurnace;

class TextSensorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_millis = 0;
    hub_ = new TestableHub();
    ts_ = new WaterFurnaceTextSensor();
    ts_->set_parent(hub_);
  }

  void TearDown() override {
    delete ts_;
    delete hub_;
  }

  TestableHub *hub_;
  WaterFurnaceTextSensor *ts_;
};

// ====== Fault Sensor ======

TEST_F(TextSensorTest, FaultNoFault) {
  ts_->set_sensor_type("fault");
  ts_->setup();

  hub_->dispatch_register_(REG_LAST_FAULT, 0);
  EXPECT_EQ(ts_->state, "No Fault");
}

TEST_F(TextSensorTest, FaultKnownCode) {
  ts_->set_sensor_type("fault");
  ts_->setup();

  hub_->dispatch_register_(REG_LAST_FAULT, 2);  // High Pressure
  EXPECT_EQ(ts_->state, "E2 High Pressure");
}

TEST_F(TextSensorTest, FaultWithLockout) {
  ts_->set_sensor_type("fault");
  ts_->setup();

  hub_->dispatch_register_(REG_LAST_FAULT, 0x8002);  // bit 15 + code 2
  EXPECT_EQ(ts_->state, "E2 High Pressure (LOCKOUT)");
}

TEST_F(TextSensorTest, FaultUnknownCode) {
  ts_->set_sensor_type("fault");
  ts_->setup();

  hub_->dispatch_register_(REG_LAST_FAULT, 50);
  EXPECT_EQ(ts_->state, "E50 Unknown Fault");
}

// ====== Mode Sensor (system outputs) ======

TEST_F(TextSensorTest, ModeIdle) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, 0x0000);
  EXPECT_EQ(ts_->state, "Idle");
}

TEST_F(TextSensorTest, ModeHeating) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC);  // compressor on, no RV
  EXPECT_EQ(ts_->state, "Heating");
}

TEST_F(TextSensorTest, ModeCooling) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC | OUTPUT_RV);
  EXPECT_EQ(ts_->state, "Cooling");
}

TEST_F(TextSensorTest, ModeEmergencyHeat) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_EH1);
  EXPECT_EQ(ts_->state, "Emergency Heat");
}

TEST_F(TextSensorTest, ModeFanOnly) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_BLOWER);
  EXPECT_EQ(ts_->state, "Fan Only");
}

TEST_F(TextSensorTest, ModeLockout) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_LOCKOUT);
  EXPECT_EQ(ts_->state, "Lockout");
}

TEST_F(TextSensorTest, ModeLockoutPriority) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // Lockout should take priority over compressor
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_LOCKOUT | OUTPUT_CC);
  EXPECT_EQ(ts_->state, "Lockout");
}

TEST_F(TextSensorTest, ModeEheatPriorityOverCompressor) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // EH1 should take priority over CC
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_EH1 | OUTPUT_CC);
  EXPECT_EQ(ts_->state, "Emergency Heat");
}

// ====== Dedup ======

TEST_F(TextSensorTest, DedupSameValue) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, 0);
  EXPECT_EQ(ts_->state, "Idle");

  // Corrupt state to detect if publish is called
  ts_->state = "corrupted";

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, 0);
  EXPECT_EQ(ts_->state, "corrupted");  // Dedup prevented re-publish
}

TEST_F(TextSensorTest, DedupDifferentValue) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, 0);
  EXPECT_EQ(ts_->state, "Idle");

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC);
  EXPECT_EQ(ts_->state, "Heating");
}
