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
  EXPECT_EQ(ts_->state, "Standby");
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

  // CC + EH1 (no RV) = "Heating with Aux" (compressor takes priority branch)
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_EH1 | OUTPUT_CC);
  EXPECT_EQ(ts_->state, "Heating with Aux");
}

// ====== System Mode Combinations ======

TEST_F(TextSensorTest, ModeStandby) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // No outputs, no compressor delay → Standby
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, 0x0000);
  EXPECT_EQ(ts_->state, "Standby");
}

TEST_F(TextSensorTest, ModeWaitingCompressorDelay) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // No outputs active, but compressor anti-short cycle delay > 0
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, 0x0000);
  hub_->dispatch_register_(REG_COMPRESSOR_DELAY, 120);  // 120 seconds remaining
  EXPECT_EQ(ts_->state, "Waiting");
}

TEST_F(TextSensorTest, ModeWaitingClearsWhenDelayReachesZero) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // Start with delay active
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, 0x0000);
  hub_->dispatch_register_(REG_COMPRESSOR_DELAY, 120);
  EXPECT_EQ(ts_->state, "Waiting");

  // Delay reaches zero → Standby
  hub_->dispatch_register_(REG_COMPRESSOR_DELAY, 0);
  EXPECT_EQ(ts_->state, "Standby");
}

TEST_F(TextSensorTest, ModeDehumidify) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // Active dehumidify flag set, compressor running
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC | OUTPUT_BLOWER);
  EXPECT_EQ(ts_->state, "Heating");  // Before dehumidify flag

  hub_->dispatch_register_(REG_ACTIVE_DEHUMIDIFY, 1);
  EXPECT_EQ(ts_->state, "Dehumidify");
}

TEST_F(TextSensorTest, ModeDehumidifyClearsWhenFlagClears) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC | OUTPUT_BLOWER);
  hub_->dispatch_register_(REG_ACTIVE_DEHUMIDIFY, 1);
  EXPECT_EQ(ts_->state, "Dehumidify");

  // Dehumidify clears → back to Heating
  hub_->dispatch_register_(REG_ACTIVE_DEHUMIDIFY, 0);
  EXPECT_EQ(ts_->state, "Heating");
}

TEST_F(TextSensorTest, ModeDehumidifyPriorityOverCompressor) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // Dehumidify takes priority over compressor+RV (would be "Cooling")
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC | OUTPUT_RV | OUTPUT_BLOWER);
  hub_->dispatch_register_(REG_ACTIVE_DEHUMIDIFY, 1);
  EXPECT_EQ(ts_->state, "Dehumidify");
}

TEST_F(TextSensorTest, ModeLockoutPriorityOverDehumidify) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // Lockout has highest priority — even over dehumidify
  hub_->dispatch_register_(REG_ACTIVE_DEHUMIDIFY, 1);
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_LOCKOUT | OUTPUT_CC);
  EXPECT_EQ(ts_->state, "Lockout");
}

TEST_F(TextSensorTest, ModeHeatingStage2) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // CC2 alone (no RV) = Heating
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC2);
  EXPECT_EQ(ts_->state, "Heating");
}

TEST_F(TextSensorTest, ModeCoolingStage2) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // CC2 + RV = Cooling
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC2 | OUTPUT_RV);
  EXPECT_EQ(ts_->state, "Cooling");
}

TEST_F(TextSensorTest, ModeCoolingBothStages) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // CC + CC2 + RV + blower = Cooling
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC | OUTPUT_CC2 | OUTPUT_RV | OUTPUT_BLOWER);
  EXPECT_EQ(ts_->state, "Cooling");
}

TEST_F(TextSensorTest, ModeHeatingWithAuxEH1) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // CC + EH1 (no RV) = Heating with Aux
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC | OUTPUT_EH1);
  EXPECT_EQ(ts_->state, "Heating with Aux");
}

TEST_F(TextSensorTest, ModeHeatingWithAuxEH2) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // CC + EH2 (no RV) = Heating with Aux
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC | OUTPUT_EH2);
  EXPECT_EQ(ts_->state, "Heating with Aux");
}

TEST_F(TextSensorTest, ModeHeatingWithAuxBothStages) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // CC + EH1 + EH2 + blower = Heating with Aux
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC | OUTPUT_EH1 | OUTPUT_EH2 | OUTPUT_BLOWER);
  EXPECT_EQ(ts_->state, "Heating with Aux");
}

TEST_F(TextSensorTest, ModeEmergencyHeatEH2Only) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // EH2 without compressor = Emergency Heat
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_EH2);
  EXPECT_EQ(ts_->state, "Emergency Heat");
}

TEST_F(TextSensorTest, ModeEmergencyHeatBothStages) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // EH1 + EH2 without compressor = Emergency Heat
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_EH1 | OUTPUT_EH2 | OUTPUT_BLOWER);
  EXPECT_EQ(ts_->state, "Emergency Heat");
}

TEST_F(TextSensorTest, ModeFanOnlyWithBlower) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // Blower only = Fan Only
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_BLOWER);
  EXPECT_EQ(ts_->state, "Fan Only");
}

TEST_F(TextSensorTest, ModeFanOnlyIgnoredWhenCompressorRunning) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // CC + blower (no RV) = Heating, not Fan Only
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC | OUTPUT_BLOWER);
  EXPECT_EQ(ts_->state, "Heating");
}

TEST_F(TextSensorTest, ModeLockoutWithEverythingOn) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // Lockout always wins, even if CC + EH + blower all active
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS,
      OUTPUT_LOCKOUT | OUTPUT_CC | OUTPUT_EH1 | OUTPUT_BLOWER | OUTPUT_RV);
  EXPECT_EQ(ts_->state, "Lockout");
}

TEST_F(TextSensorTest, ModeAlarmBitDoesNotAffectMode) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // Alarm bit without lockout — compressor still shows as heating
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_ALARM | OUTPUT_CC);
  EXPECT_EQ(ts_->state, "Heating");
}

TEST_F(TextSensorTest, ModeAccessoryBitDoesNotAffectMode) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // Accessory bit doesn't influence mode determination
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_ACCESSORY | OUTPUT_CC | OUTPUT_RV);
  EXPECT_EQ(ts_->state, "Cooling");
}

TEST_F(TextSensorTest, ModeTransitionHeatingToCooling) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC | OUTPUT_BLOWER);
  EXPECT_EQ(ts_->state, "Heating");

  // Reversing valve engages → Cooling
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC | OUTPUT_RV | OUTPUT_BLOWER);
  EXPECT_EQ(ts_->state, "Cooling");
}

TEST_F(TextSensorTest, ModeTransitionCoolingToStandby) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC | OUTPUT_RV | OUTPUT_BLOWER);
  EXPECT_EQ(ts_->state, "Cooling");

  // Everything off
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, 0x0000);
  EXPECT_EQ(ts_->state, "Standby");
}

TEST_F(TextSensorTest, ModeWaitingNotShownWhenCompressorRunning) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // Compressor delay value doesn't matter when compressor is actually running
  hub_->dispatch_register_(REG_COMPRESSOR_DELAY, 60);
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC | OUTPUT_BLOWER);
  EXPECT_EQ(ts_->state, "Heating");
}

TEST_F(TextSensorTest, ModeReversingValveAloneIsStandby) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  // RV alone (no compressor, no blower) = Standby
  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_RV);
  EXPECT_EQ(ts_->state, "Standby");
}

// ====== Outputs At Lockout ======

TEST_F(TextSensorTest, OutputsAtLockoutNone) {
  ts_->set_sensor_type("outputs_at_lockout");
  ts_->setup();

  hub_->dispatch_register_(REG_OUTPUTS_AT_LOCKOUT, 0);
  EXPECT_EQ(ts_->state, "None");
}

TEST_F(TextSensorTest, OutputsAtLockoutSingleBit) {
  ts_->set_sensor_type("outputs_at_lockout");
  ts_->setup();

  hub_->dispatch_register_(REG_OUTPUTS_AT_LOCKOUT, OUTPUT_CC);
  EXPECT_EQ(ts_->state, "CC");
}

TEST_F(TextSensorTest, OutputsAtLockoutMultipleBits) {
  ts_->set_sensor_type("outputs_at_lockout");
  ts_->setup();

  hub_->dispatch_register_(REG_OUTPUTS_AT_LOCKOUT, OUTPUT_CC | OUTPUT_RV | OUTPUT_BLOWER | OUTPUT_LOCKOUT);
  EXPECT_EQ(ts_->state, "CC, RV, Blower, Lockout");
}

// ====== Inputs At Lockout ======

TEST_F(TextSensorTest, InputsAtLockoutNone) {
  ts_->set_sensor_type("inputs_at_lockout");
  ts_->setup();

  hub_->dispatch_register_(REG_INPUTS_AT_LOCKOUT, 0);
  EXPECT_EQ(ts_->state, "None");
}

TEST_F(TextSensorTest, InputsAtLockoutLPS) {
  ts_->set_sensor_type("inputs_at_lockout");
  ts_->setup();

  // LPS is 0x80 — previously misidentified as Load Shed
  hub_->dispatch_register_(REG_INPUTS_AT_LOCKOUT, 0x80);
  EXPECT_EQ(ts_->state, "LPS");
}

TEST_F(TextSensorTest, InputsAtLockoutHPS) {
  ts_->set_sensor_type("inputs_at_lockout");
  ts_->setup();

  hub_->dispatch_register_(REG_INPUTS_AT_LOCKOUT, 0x100);
  EXPECT_EQ(ts_->state, "HPS");
}

TEST_F(TextSensorTest, InputsAtLockoutLoadShed) {
  ts_->set_sensor_type("inputs_at_lockout");
  ts_->setup();

  // Load Shed is 0x200
  hub_->dispatch_register_(REG_INPUTS_AT_LOCKOUT, 0x200);
  EXPECT_EQ(ts_->state, "Load Shed");
}

TEST_F(TextSensorTest, InputsAtLockoutMultipleBits) {
  ts_->set_sensor_type("inputs_at_lockout");
  ts_->setup();

  hub_->dispatch_register_(REG_INPUTS_AT_LOCKOUT, INPUT_Y1 | INPUT_W | INPUT_LPS);
  EXPECT_EQ(ts_->state, "Y1, W, LPS");
}

// ====== Dedup ======

TEST_F(TextSensorTest, DedupSameValue) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, 0);
  EXPECT_EQ(ts_->state, "Standby");

  // Corrupt state to detect if publish is called
  ts_->state = "corrupted";

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, 0);
  EXPECT_EQ(ts_->state, "corrupted");  // Dedup prevented re-publish
}

TEST_F(TextSensorTest, DedupDifferentValue) {
  ts_->set_sensor_type("mode");
  ts_->setup();

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, 0);
  EXPECT_EQ(ts_->state, "Standby");

  hub_->dispatch_register_(REG_SYSTEM_OUTPUTS, OUTPUT_CC);
  EXPECT_EQ(ts_->state, "Heating");
}
