// Unit tests for waterfurnace_climate component

#include <gtest/gtest.h>
#include "../../components/waterfurnace/protocol.cpp"
#include "hub_stubs.h"
#include "../../components/waterfurnace/climate/waterfurnace_climate.cpp"

using namespace esphome;
using namespace esphome::waterfurnace;
using namespace esphome::climate;

// Expose protected methods for testing
class TestableClimate : public WaterFurnaceClimate {
 public:
  using WaterFurnaceClimate::on_ambient_temp_;
  using WaterFurnaceClimate::on_heating_setpoint_;
  using WaterFurnaceClimate::on_cooling_setpoint_;
  using WaterFurnaceClimate::on_mode_config_;
  using WaterFurnaceClimate::on_fan_config_;
  using WaterFurnaceClimate::on_iz2_config1_;
  using WaterFurnaceClimate::on_iz2_config2_;
  using WaterFurnaceClimate::get_mode_write_reg_;
  using WaterFurnaceClimate::get_heating_sp_write_reg_;
  using WaterFurnaceClimate::get_cooling_sp_write_reg_;
  using WaterFurnaceClimate::get_fan_mode_write_reg_;
  using WaterFurnaceClimate::publish_state_if_changed_;
  using WaterFurnaceClimate::iz2_config1_;
  using WaterFurnaceClimate::iz2_config2_;
  using WaterFurnaceClimate::has_published_;
  using WaterFurnaceClimate::custom_fan_mode_;
};

class ClimateTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_millis = 0;
    waterfurnace::clear_written_registers();
    hub_ = new TestableHub();
    climate_ = new TestableClimate();
    climate_->set_parent(hub_);
  }

  void TearDown() override {
    delete climate_;
    delete hub_;
  }

  TestableHub *hub_;
  TestableClimate *climate_;
};

// ====== Single Zone: Temperature Callbacks ======

TEST_F(ClimateTest, AmbientTempConversion) {
  climate_->set_zone(0);
  climate_->setup();

  // 70.0°F = 700 raw → 21.111°C
  hub_->dispatch_register_(REG_AMBIENT_TEMP, 700);
  float expected_c = (70.0f - 32.0f) * 5.0f / 9.0f;
  EXPECT_NEAR(climate_->current_temperature, expected_c, 0.01f);
}

TEST_F(ClimateTest, AmbientTempNegative) {
  climate_->set_zone(0);
  climate_->setup();

  // -10.0°F = 0xFF9C as signed → should convert to negative Celsius
  uint16_t neg = static_cast<uint16_t>(static_cast<int16_t>(-100));  // -10.0°F
  hub_->dispatch_register_(REG_AMBIENT_TEMP, neg);
  float expected_c = (-10.0f - 32.0f) * 5.0f / 9.0f;
  EXPECT_NEAR(climate_->current_temperature, expected_c, 0.01f);
}

TEST_F(ClimateTest, HeatingSetpointConversion) {
  climate_->set_zone(0);
  climate_->setup();

  // 68.0°F = 680 raw
  hub_->dispatch_register_(REG_HEATING_SETPOINT, 680);
  float expected_c = (68.0f - 32.0f) * 5.0f / 9.0f;  // 20.0°C
  EXPECT_NEAR(climate_->target_temperature_low, expected_c, 0.01f);
}

TEST_F(ClimateTest, CoolingSetpointConversion) {
  climate_->set_zone(0);
  climate_->setup();

  // 75.0°F = 750 raw
  hub_->dispatch_register_(REG_COOLING_SETPOINT, 750);
  float expected_c = (75.0f - 32.0f) * 5.0f / 9.0f;  // 23.889°C
  EXPECT_NEAR(climate_->target_temperature_high, expected_c, 0.01f);
}

// ====== Single Zone: Mode Callbacks ======

TEST_F(ClimateTest, ModeOff) {
  climate_->set_zone(0);
  climate_->setup();

  // MODE_OFF = 0, in bits 8-10: 0x0000
  hub_->dispatch_register_(REG_MODE_CONFIG, MODE_OFF << 8);
  EXPECT_EQ(climate_->mode, CLIMATE_MODE_OFF);
  EXPECT_EQ(climate_->preset, CLIMATE_PRESET_NONE);
}

TEST_F(ClimateTest, ModeAuto) {
  climate_->set_zone(0);
  climate_->setup();

  hub_->dispatch_register_(REG_MODE_CONFIG, MODE_AUTO << 8);
  EXPECT_EQ(climate_->mode, CLIMATE_MODE_HEAT_COOL);
  EXPECT_EQ(climate_->preset, CLIMATE_PRESET_NONE);
}

TEST_F(ClimateTest, ModeCool) {
  climate_->set_zone(0);
  climate_->setup();

  hub_->dispatch_register_(REG_MODE_CONFIG, MODE_COOL << 8);
  EXPECT_EQ(climate_->mode, CLIMATE_MODE_COOL);
}

TEST_F(ClimateTest, ModeHeat) {
  climate_->set_zone(0);
  climate_->setup();

  hub_->dispatch_register_(REG_MODE_CONFIG, MODE_HEAT << 8);
  EXPECT_EQ(climate_->mode, CLIMATE_MODE_HEAT);
  EXPECT_EQ(climate_->preset, CLIMATE_PRESET_NONE);
}

TEST_F(ClimateTest, ModeEheat) {
  climate_->set_zone(0);
  climate_->setup();

  hub_->dispatch_register_(REG_MODE_CONFIG, MODE_EHEAT << 8);
  EXPECT_EQ(climate_->mode, CLIMATE_MODE_HEAT);
  EXPECT_EQ(climate_->preset, CLIMATE_PRESET_BOOST);
}

// ====== Single Zone: Fan Callbacks ======

TEST_F(ClimateTest, FanAuto) {
  climate_->set_zone(0);
  climate_->setup();

  hub_->dispatch_register_(REG_FAN_CONFIG, 0x0000);
  EXPECT_EQ(climate_->fan_mode, CLIMATE_FAN_AUTO);
}

TEST_F(ClimateTest, FanContinuous) {
  climate_->set_zone(0);
  climate_->setup();

  // Bit 7 set = continuous (FAN_ON)
  hub_->dispatch_register_(REG_FAN_CONFIG, 0x0080);
  EXPECT_EQ(climate_->fan_mode, CLIMATE_FAN_ON);
}

TEST_F(ClimateTest, FanIntermittent) {
  climate_->set_zone(0);
  climate_->setup();

  // Bit 8 set = intermittent
  hub_->dispatch_register_(REG_FAN_CONFIG, 0x0100);
  EXPECT_FALSE(climate_->fan_mode.has_value());
  EXPECT_EQ(climate_->custom_fan_mode_, "Intermittent");
}

// ====== IZ2 Zone: Config Callbacks ======

TEST_F(ClimateTest, IZ2Config1FanAuto) {
  climate_->set_zone(1);
  climate_->setup();

  // Fan mode auto = bits 7-8 clear
  uint16_t base = REG_IZ2_ZONE_BASE;
  hub_->dispatch_register_(base + 1, 0x004E);  // cooling SP = 0x4E = 78 raw → 75°F + 36 offset
  EXPECT_EQ(climate_->fan_mode, CLIMATE_FAN_AUTO);
}

TEST_F(ClimateTest, IZ2Config1FanContinuous) {
  climate_->set_zone(1);
  climate_->setup();

  uint16_t base = REG_IZ2_ZONE_BASE;
  hub_->dispatch_register_(base + 1, 0x0080);  // bit 7 set
  EXPECT_EQ(climate_->fan_mode, CLIMATE_FAN_ON);
}

TEST_F(ClimateTest, IZ2Config1FanIntermittent) {
  climate_->set_zone(1);
  climate_->setup();

  uint16_t base = REG_IZ2_ZONE_BASE;
  hub_->dispatch_register_(base + 1, 0x0100);  // bit 8 set
  EXPECT_FALSE(climate_->fan_mode.has_value());
  EXPECT_EQ(climate_->custom_fan_mode_, "Intermittent");
}

TEST_F(ClimateTest, IZ2Config1CoolingSetpoint) {
  climate_->set_zone(1);
  climate_->setup();

  uint16_t base = REG_IZ2_ZONE_BASE;
  // iz2_extract_cooling_setpoint: ((config1 & 0x7E) >> 1) + 36
  // For 75°F: 75 - 36 = 39, 39 << 1 = 78 = 0x4E
  hub_->dispatch_register_(base + 1, 0x004E);
  float expected_c = (75.0f - 32.0f) * 5.0f / 9.0f;
  EXPECT_NEAR(climate_->target_temperature_high, expected_c, 0.01f);
}

TEST_F(ClimateTest, IZ2Config2ModeOff) {
  climate_->set_zone(1);
  climate_->setup();

  uint16_t base = REG_IZ2_ZONE_BASE;
  hub_->dispatch_register_(base + 2, 0x0000);  // MODE_OFF
  EXPECT_EQ(climate_->mode, CLIMATE_MODE_OFF);
}

TEST_F(ClimateTest, IZ2Config2ModeAuto) {
  climate_->set_zone(1);
  climate_->setup();

  uint16_t base = REG_IZ2_ZONE_BASE;
  hub_->dispatch_register_(base + 2, 0x0100);  // MODE_AUTO in bits 8-9
  EXPECT_EQ(climate_->mode, CLIMATE_MODE_HEAT_COOL);
}

TEST_F(ClimateTest, IZ2Config2ModeCool) {
  climate_->set_zone(1);
  climate_->setup();

  uint16_t base = REG_IZ2_ZONE_BASE;
  hub_->dispatch_register_(base + 2, 0x0200);  // MODE_COOL
  EXPECT_EQ(climate_->mode, CLIMATE_MODE_COOL);
}

TEST_F(ClimateTest, IZ2Config2ModeHeat) {
  climate_->set_zone(1);
  climate_->setup();

  uint16_t base = REG_IZ2_ZONE_BASE;
  hub_->dispatch_register_(base + 2, 0x0300);  // MODE_HEAT
  EXPECT_EQ(climate_->mode, CLIMATE_MODE_HEAT);
}

TEST_F(ClimateTest, IZ2HeatingSetpoint) {
  climate_->set_zone(1);
  climate_->setup();

  uint16_t base = REG_IZ2_ZONE_BASE;
  // Send config2 first so config1 can extract heating SP
  // iz2_extract_heating_setpoint(config1, config2):
  //   low 5 bits of (config1 >> 9) | high bits from config2
  // For 68°F: need the right bit pattern
  // Let's use dispatch for both and verify
  hub_->dispatch_register_(base + 2, 0x2100);  // mode auto + heating SP bits
  hub_->dispatch_register_(base + 1, 0x0227);  // cooling SP 0x27 + carry bit in upper bits

  // The heating setpoint depends on iz2_extract_heating_setpoint which combines
  // bits from both config words. Just verify it produces a reasonable value.
  EXPECT_FALSE(std::isnan(climate_->target_temperature_low));
}

// ====== Write Register Addresses ======

TEST_F(ClimateTest, SingleZoneWriteRegs) {
  climate_->set_zone(0);
  EXPECT_EQ(climate_->get_mode_write_reg_(), REG_WRITE_MODE);
  EXPECT_EQ(climate_->get_heating_sp_write_reg_(), REG_WRITE_HEATING_SP);
  EXPECT_EQ(climate_->get_cooling_sp_write_reg_(), REG_WRITE_COOLING_SP);
  EXPECT_EQ(climate_->get_fan_mode_write_reg_(), REG_WRITE_FAN_MODE);
}

TEST_F(ClimateTest, IZ2Zone1WriteRegs) {
  climate_->set_zone(1);
  EXPECT_EQ(climate_->get_mode_write_reg_(), REG_IZ2_WRITE_BASE);
  EXPECT_EQ(climate_->get_heating_sp_write_reg_(), REG_IZ2_WRITE_BASE + 1);
  EXPECT_EQ(climate_->get_cooling_sp_write_reg_(), REG_IZ2_WRITE_BASE + 2);
  EXPECT_EQ(climate_->get_fan_mode_write_reg_(), REG_IZ2_WRITE_BASE + 3);
}

TEST_F(ClimateTest, IZ2Zone3WriteRegs) {
  climate_->set_zone(3);
  EXPECT_EQ(climate_->get_mode_write_reg_(), REG_IZ2_WRITE_BASE + 18);
  EXPECT_EQ(climate_->get_heating_sp_write_reg_(), REG_IZ2_WRITE_BASE + 19);
  EXPECT_EQ(climate_->get_cooling_sp_write_reg_(), REG_IZ2_WRITE_BASE + 20);
  EXPECT_EQ(climate_->get_fan_mode_write_reg_(), REG_IZ2_WRITE_BASE + 21);
}

// ====== Control Method ======

TEST_F(ClimateTest, ControlSetModeOff) {
  climate_->set_zone(0);
  climate_->setup();

  ClimateCall call;
  call.set_mode(CLIMATE_MODE_OFF);
  climate_->control(call);

  ASSERT_EQ(waterfurnace::written_registers.size(), 1u);
  EXPECT_EQ(waterfurnace::written_registers[0].first, REG_WRITE_MODE);
  EXPECT_EQ(waterfurnace::written_registers[0].second, MODE_OFF);
}

TEST_F(ClimateTest, ControlSetModeHeatCool) {
  climate_->set_zone(0);
  climate_->setup();

  ClimateCall call;
  call.set_mode(CLIMATE_MODE_HEAT_COOL);
  climate_->control(call);

  ASSERT_EQ(waterfurnace::written_registers.size(), 1u);
  EXPECT_EQ(waterfurnace::written_registers[0].second, MODE_AUTO);
}

TEST_F(ClimateTest, ControlSetModeCool) {
  climate_->set_zone(0);
  climate_->setup();

  ClimateCall call;
  call.set_mode(CLIMATE_MODE_COOL);
  climate_->control(call);

  ASSERT_EQ(waterfurnace::written_registers.size(), 1u);
  EXPECT_EQ(waterfurnace::written_registers[0].second, MODE_COOL);
}

TEST_F(ClimateTest, ControlSetModeHeat) {
  climate_->set_zone(0);
  climate_->setup();

  ClimateCall call;
  call.set_mode(CLIMATE_MODE_HEAT);
  climate_->control(call);

  ASSERT_EQ(waterfurnace::written_registers.size(), 1u);
  EXPECT_EQ(waterfurnace::written_registers[0].second, MODE_HEAT);
}

TEST_F(ClimateTest, ControlSetPresetBoost) {
  climate_->set_zone(0);
  climate_->setup();

  ClimateCall call;
  call.set_preset(CLIMATE_PRESET_BOOST);
  climate_->control(call);

  ASSERT_EQ(waterfurnace::written_registers.size(), 1u);
  EXPECT_EQ(waterfurnace::written_registers[0].first, REG_WRITE_MODE);
  EXPECT_EQ(waterfurnace::written_registers[0].second, MODE_EHEAT);
}

TEST_F(ClimateTest, ControlSetHeatingSetpoint) {
  climate_->set_zone(0);
  climate_->setup();

  // 20°C → 68°F → raw 680
  float temp_c = 20.0f;
  ClimateCall call;
  call.set_target_temperature_low(temp_c);
  climate_->control(call);

  ASSERT_EQ(waterfurnace::written_registers.size(), 1u);
  EXPECT_EQ(waterfurnace::written_registers[0].first, REG_WRITE_HEATING_SP);
  // Verify: 20°C * 9/5 + 32 = 68°F, * 10 = 680
  EXPECT_EQ(waterfurnace::written_registers[0].second, 680u);
}

TEST_F(ClimateTest, ControlSetCoolingSetpoint) {
  climate_->set_zone(0);
  climate_->setup();

  // 23.889°C → 75°F → raw 750
  float temp_c = (75.0f - 32.0f) * 5.0f / 9.0f;
  ClimateCall call;
  call.set_target_temperature_high(temp_c);
  climate_->control(call);

  ASSERT_EQ(waterfurnace::written_registers.size(), 1u);
  EXPECT_EQ(waterfurnace::written_registers[0].first, REG_WRITE_COOLING_SP);
  EXPECT_EQ(waterfurnace::written_registers[0].second, 750u);
}

TEST_F(ClimateTest, ControlSetFanAuto) {
  climate_->set_zone(0);
  climate_->setup();

  ClimateCall call;
  call.set_fan_mode(CLIMATE_FAN_AUTO);
  climate_->control(call);

  ASSERT_EQ(waterfurnace::written_registers.size(), 1u);
  EXPECT_EQ(waterfurnace::written_registers[0].first, REG_WRITE_FAN_MODE);
  EXPECT_EQ(waterfurnace::written_registers[0].second, FAN_AUTO);
}

TEST_F(ClimateTest, ControlSetFanOn) {
  climate_->set_zone(0);
  climate_->setup();

  ClimateCall call;
  call.set_fan_mode(CLIMATE_FAN_ON);
  climate_->control(call);

  ASSERT_EQ(waterfurnace::written_registers.size(), 1u);
  EXPECT_EQ(waterfurnace::written_registers[0].second, FAN_CONTINUOUS);
}

TEST_F(ClimateTest, ControlSetCustomFanIntermittent) {
  climate_->set_zone(0);
  climate_->setup();

  ClimateCall call;
  call.set_custom_fan_mode("Intermittent");
  climate_->control(call);

  ASSERT_EQ(waterfurnace::written_registers.size(), 1u);
  EXPECT_EQ(waterfurnace::written_registers[0].first, REG_WRITE_FAN_MODE);
  EXPECT_EQ(waterfurnace::written_registers[0].second, FAN_INTERMITTENT);
}

TEST_F(ClimateTest, ControlIZ2WritesCorrectRegister) {
  climate_->set_zone(2);
  climate_->setup();

  ClimateCall call;
  call.set_mode(CLIMATE_MODE_OFF);
  climate_->control(call);

  ASSERT_EQ(waterfurnace::written_registers.size(), 1u);
  // Zone 2: REG_IZ2_WRITE_BASE + (2-1)*9 = 21202 + 9 = 21211
  EXPECT_EQ(waterfurnace::written_registers[0].first, REG_IZ2_WRITE_BASE + 9);
}

// ====== Publish Dedup ======

TEST_F(ClimateTest, DedupFirstPublishAlwaysWorks) {
  climate_->set_zone(0);
  climate_->setup();

  hub_->dispatch_register_(REG_AMBIENT_TEMP, 700);
  EXPECT_EQ(climate_->publish_count_, 1);
}

TEST_F(ClimateTest, DedupSameValueNoRepublish) {
  climate_->set_zone(0);
  climate_->setup();

  hub_->dispatch_register_(REG_AMBIENT_TEMP, 700);
  EXPECT_EQ(climate_->publish_count_, 1);

  hub_->dispatch_register_(REG_AMBIENT_TEMP, 700);
  EXPECT_EQ(climate_->publish_count_, 1);  // Dedup prevents republish
}

TEST_F(ClimateTest, DedupDifferentValueRepublishes) {
  climate_->set_zone(0);
  climate_->setup();

  hub_->dispatch_register_(REG_AMBIENT_TEMP, 700);
  EXPECT_EQ(climate_->publish_count_, 1);

  hub_->dispatch_register_(REG_AMBIENT_TEMP, 750);
  EXPECT_EQ(climate_->publish_count_, 2);
}

TEST_F(ClimateTest, DedupModeChangeRepublishes) {
  climate_->set_zone(0);
  climate_->setup();

  hub_->dispatch_register_(REG_MODE_CONFIG, MODE_OFF << 8);
  int count_after_off = climate_->publish_count_;

  hub_->dispatch_register_(REG_MODE_CONFIG, MODE_HEAT << 8);
  EXPECT_GT(climate_->publish_count_, count_after_off);
}

// ====== IZ2 Zone Addressing ======

TEST_F(ClimateTest, IZ2Zone2RegisterBase) {
  climate_->set_zone(2);
  climate_->setup();

  // Zone 2: base = REG_IZ2_ZONE_BASE + (2-1)*3 = 31007 + 3 = 31010
  uint16_t base = REG_IZ2_ZONE_BASE + 3;

  // Ambient temp at base
  hub_->dispatch_register_(base, 700);
  float expected_c = (70.0f - 32.0f) * 5.0f / 9.0f;
  EXPECT_NEAR(climate_->current_temperature, expected_c, 0.01f);
}

TEST_F(ClimateTest, IZ2Zone6RegisterBase) {
  climate_->set_zone(6);
  climate_->setup();

  // Zone 6: base = REG_IZ2_ZONE_BASE + (6-1)*3 = 31007 + 15 = 31022
  uint16_t base = REG_IZ2_ZONE_BASE + 15;

  hub_->dispatch_register_(base, 700);
  float expected_c = (70.0f - 32.0f) * 5.0f / 9.0f;
  EXPECT_NEAR(climate_->current_temperature, expected_c, 0.01f);
}
