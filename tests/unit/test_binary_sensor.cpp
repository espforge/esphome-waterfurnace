// Unit tests for waterfurnace_binary_sensor component

#include <gtest/gtest.h>
#include "../../components/waterfurnace/protocol.cpp"
#include "hub_stubs.h"
#include "../../components/waterfurnace/binary_sensor/waterfurnace_binary_sensor.cpp"

using namespace esphome;
using namespace esphome::waterfurnace;

class BinarySensorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_millis = 0;
    hub_ = new TestableHub();
    bs_ = new WaterFurnaceBinarySensor();
    bs_->set_parent(hub_);
  }

  void TearDown() override {
    delete bs_;
    delete hub_;
  }

  TestableHub *hub_;
  WaterFurnaceBinarySensor *bs_;
};

// ====== Bitmask Matching ======

TEST_F(BinarySensorTest, CompressorOn) {
  bs_->set_register_address(REG_SYSTEM_OUTPUTS);
  bs_->set_bitmask(OUTPUT_CC);
  bs_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC | OUTPUT_BLOWER);
  EXPECT_TRUE(bs_->state);
}

TEST_F(BinarySensorTest, CompressorOff) {
  bs_->set_register_address(REG_SYSTEM_OUTPUTS);
  bs_->set_bitmask(OUTPUT_CC);
  bs_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_BLOWER);  // CC not set
  EXPECT_FALSE(bs_->state);
}

TEST_F(BinarySensorTest, ReversingValve) {
  bs_->set_register_address(REG_SYSTEM_OUTPUTS);
  bs_->set_bitmask(OUTPUT_RV);
  bs_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC | OUTPUT_RV);
  EXPECT_TRUE(bs_->state);

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC);
  EXPECT_FALSE(bs_->state);
}

TEST_F(BinarySensorTest, LockoutBit) {
  bs_->set_register_address(REG_SYSTEM_OUTPUTS);
  bs_->set_bitmask(OUTPUT_LOCKOUT);
  bs_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_LOCKOUT);
  EXPECT_TRUE(bs_->state);

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, 0);
  EXPECT_FALSE(bs_->state);
}

TEST_F(BinarySensorTest, AlarmBit) {
  bs_->set_register_address(REG_SYSTEM_OUTPUTS);
  bs_->set_bitmask(OUTPUT_ALARM);
  bs_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_ALARM);
  EXPECT_TRUE(bs_->state);
}

TEST_F(BinarySensorTest, AllZero) {
  bs_->set_register_address(REG_SYSTEM_OUTPUTS);
  bs_->set_bitmask(OUTPUT_CC);
  bs_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, 0);
  EXPECT_FALSE(bs_->state);
}

TEST_F(BinarySensorTest, AuxHeatStage1) {
  bs_->set_register_address(REG_SYSTEM_OUTPUTS);
  bs_->set_bitmask(OUTPUT_EH1);
  bs_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_EH1 | OUTPUT_EH2);
  EXPECT_TRUE(bs_->state);
}

TEST_F(BinarySensorTest, BlowerOnly) {
  bs_->set_register_address(REG_SYSTEM_OUTPUTS);
  bs_->set_bitmask(OUTPUT_BLOWER);
  bs_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_BLOWER);
  EXPECT_TRUE(bs_->state);
}
