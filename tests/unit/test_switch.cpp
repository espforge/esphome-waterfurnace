// Unit tests for waterfurnace_switch component

#include <gtest/gtest.h>
#include "../../components/waterfurnace/protocol.cpp"
#include "hub_stubs.h"
#include "../../components/waterfurnace/switch/waterfurnace_switch.cpp"

using namespace esphome;
using namespace esphome::waterfurnace;

class SwitchTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_millis = 0;
    waterfurnace::clear_written_registers();
    hub_ = new TestableHub();
    sw_ = new WaterFurnaceSwitch();
    sw_->set_parent(hub_);
  }

  void TearDown() override {
    delete sw_;
    delete hub_;
  }

  TestableHub *hub_;
  WaterFurnaceSwitch *sw_;
};

// ====== Read State from Register ======

TEST_F(SwitchTest, RegisterUpdateOn) {
  sw_->set_register_address(REG_DHW_ENABLE);
  sw_->setup();

  hub_->dispatch_register_(REG_DHW_ENABLE, 1);
  EXPECT_TRUE(sw_->state);
}

TEST_F(SwitchTest, RegisterUpdateOff) {
  sw_->set_register_address(REG_DHW_ENABLE);
  sw_->setup();

  hub_->dispatch_register_(REG_DHW_ENABLE, 0);
  EXPECT_FALSE(sw_->state);
}

// ====== Write State ======

TEST_F(SwitchTest, TurnOnWritesRegister) {
  sw_->set_register_address(REG_DHW_ENABLE);
  sw_->set_write_address(REG_DHW_ENABLE);
  sw_->setup();

  sw_->turn_on();

  ASSERT_EQ(waterfurnace::written_registers.size(), 1u);
  EXPECT_EQ(waterfurnace::written_registers[0].first, REG_DHW_ENABLE);
  EXPECT_EQ(waterfurnace::written_registers[0].second, 1u);
  EXPECT_TRUE(sw_->state);  // Optimistic publish
}

TEST_F(SwitchTest, TurnOffWritesRegister) {
  sw_->set_register_address(REG_DHW_ENABLE);
  sw_->set_write_address(REG_DHW_ENABLE);
  sw_->setup();

  sw_->turn_off();

  ASSERT_EQ(waterfurnace::written_registers.size(), 1u);
  EXPECT_EQ(waterfurnace::written_registers[0].first, REG_DHW_ENABLE);
  EXPECT_EQ(waterfurnace::written_registers[0].second, 0u);
  EXPECT_FALSE(sw_->state);
}

// ====== Separate Read/Write Addresses ======

TEST_F(SwitchTest, DifferentWriteAddress) {
  sw_->set_register_address(400);   // Read from 400
  sw_->set_write_address(12400);    // Write to 12400
  sw_->setup();

  sw_->turn_on();

  ASSERT_EQ(waterfurnace::written_registers.size(), 1u);
  EXPECT_EQ(waterfurnace::written_registers[0].first, 12400u);
}
