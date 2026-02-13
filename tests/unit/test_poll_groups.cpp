// Unit tests for listener-driven poll group building

#include <gtest/gtest.h>
#include "../../components/waterfurnace/protocol.cpp"
#include "../../components/waterfurnace/waterfurnace.cpp"

using namespace esphome::waterfurnace;

// Testable subclass to expose protected members
class TestableHub : public WaterFurnace {
 public:
  using WaterFurnace::build_poll_groups_;
  using WaterFurnace::has_capability_;
  using WaterFurnace::poll_groups_;
  using WaterFurnace::listeners_;
  using WaterFurnace::dispatch_register_;

  void set_awl_thermostat(bool v) { awl_thermostat_ = v; }
  void set_awl_axb(bool v) { awl_axb_ = v; }
  void set_awl_iz2(bool v) { awl_iz2_ = v; }
  void set_has_axb(bool v) { has_axb_ = v; }
  void set_has_vs_drive(bool v) { has_vs_drive_ = v; }
  void set_has_energy_monitoring(bool v) { has_energy_monitoring_ = v; }
  void set_has_refrigeration_monitoring(bool v) { has_refrigeration_monitoring_ = v; }
  void set_setup_complete(bool v) { setup_complete_ = v; }

  // Count total registers across all poll groups
  size_t total_polled_registers() const {
    size_t total = 0;
    for (const auto &g : poll_groups_) {
      for (const auto &r : g.ranges) total += r.second;
      total += g.individual.size();
    }
    return total;
  }

  // Get all polled addresses (expanded from ranges + individual)
  std::vector<uint16_t> all_polled_addresses() const {
    std::vector<uint16_t> addrs;
    for (const auto &g : poll_groups_) {
      for (const auto &r : g.ranges) {
        for (uint16_t i = 0; i < r.second; i++) {
          addrs.push_back(r.first + i);
        }
      }
      for (uint16_t a : g.individual) {
        addrs.push_back(a);
      }
    }
    return addrs;
  }

  // Check if a specific address is covered by any poll group
  bool is_address_polled(uint16_t addr) const {
    for (const auto &g : poll_groups_) {
      for (const auto &r : g.ranges) {
        if (addr >= r.first && addr < r.first + r.second) return true;
      }
      for (uint16_t a : g.individual) {
        if (a == addr) return true;
      }
    }
    return false;
  }
};

// ====== merge_to_ranges ======

TEST(MergeToRanges, EmptyInput) {
  auto result = WaterFurnace::merge_to_ranges({});
  EXPECT_TRUE(result.empty());
}

TEST(MergeToRanges, SingleAddress) {
  auto result = WaterFurnace::merge_to_ranges({100});
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0].first, 100);
  EXPECT_EQ(result[0].second, 1);
}

TEST(MergeToRanges, AdjacentAddresses) {
  auto result = WaterFurnace::merge_to_ranges({10, 11, 12, 13});
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0].first, 10);
  EXPECT_EQ(result[0].second, 4);
}

TEST(MergeToRanges, SmallGapMerged) {
  // Gap of 5 (< default max_gap=8) should merge
  auto result = WaterFurnace::merge_to_ranges({10, 15});
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0].first, 10);
  EXPECT_EQ(result[0].second, 6);  // 10..15 inclusive
}

TEST(MergeToRanges, ExactGapMerged) {
  // Gap of exactly 8 should merge
  auto result = WaterFurnace::merge_to_ranges({10, 18});
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0].first, 10);
  EXPECT_EQ(result[0].second, 9);  // 10..18 inclusive
}

TEST(MergeToRanges, LargeGapSplit) {
  // Gap of 9 (> default max_gap=8) should split
  auto result = WaterFurnace::merge_to_ranges({10, 19});
  ASSERT_EQ(result.size(), 2u);
  EXPECT_EQ(result[0].first, 10);
  EXPECT_EQ(result[0].second, 1);
  EXPECT_EQ(result[1].first, 19);
  EXPECT_EQ(result[1].second, 1);
}

TEST(MergeToRanges, CustomMaxGap) {
  // Gap of 5 with max_gap=4 should split
  auto result = WaterFurnace::merge_to_ranges({10, 15}, 4);
  ASSERT_EQ(result.size(), 2u);
}

TEST(MergeToRanges, MultipleRanges) {
  auto result = WaterFurnace::merge_to_ranges({6, 19, 20, 25, 26, 27, 28, 29, 30, 31, 344, 362});
  // 6 is alone (gap to 19 = 13 > 8)
  // 19-31 merge (all within gap 8)
  // 344 is alone (gap to 31 = 313 > 8)
  // 362 is alone (gap to 344 = 18 > 8)
  ASSERT_EQ(result.size(), 4u);
  EXPECT_EQ(result[0], (std::pair<uint16_t, uint16_t>{6, 1}));
  EXPECT_EQ(result[1], (std::pair<uint16_t, uint16_t>{19, 13}));  // 19..31
  EXPECT_EQ(result[2], (std::pair<uint16_t, uint16_t>{344, 1}));
  EXPECT_EQ(result[3], (std::pair<uint16_t, uint16_t>{362, 1}));
}

TEST(MergeToRanges, TypicalCoreRegisters) {
  // Addresses a sensor entity would register for core status
  std::vector<uint16_t> addrs = {6, 19, 20, 25, 26, 27, 28, 29, 30, 31, 344, 362, 502, 567, 740, 741, 742, 745, 746, 747};
  auto result = WaterFurnace::merge_to_ranges(addrs);

  // Verify ranges cover all input addresses
  for (uint16_t addr : addrs) {
    bool found = false;
    for (const auto &r : result) {
      if (addr >= r.first && addr < r.first + r.second) {
        found = true;
        break;
      }
    }
    EXPECT_TRUE(found) << "Address " << addr << " not covered by any range";
  }
}

// ====== has_capability_ ======

class CapabilityTest : public ::testing::Test {
 protected:
  TestableHub hub_;
};

TEST_F(CapabilityTest, NoneAlwaysTrue) {
  EXPECT_TRUE(hub_.has_capability_(RegisterCapability::NONE));
}

TEST_F(CapabilityTest, AWLThermostatBlockedWithout) {
  hub_.set_awl_thermostat(false);
  EXPECT_FALSE(hub_.has_capability_(RegisterCapability::AWL_THERMOSTAT));
}

TEST_F(CapabilityTest, AWLThermostatAllowedWith) {
  hub_.set_awl_thermostat(true);
  EXPECT_TRUE(hub_.has_capability_(RegisterCapability::AWL_THERMOSTAT));
}

TEST_F(CapabilityTest, AWLAXBBlockedWithout) {
  hub_.set_awl_axb(false);
  EXPECT_FALSE(hub_.has_capability_(RegisterCapability::AWL_AXB));
}

TEST_F(CapabilityTest, AWLAXBAllowedWith) {
  hub_.set_awl_axb(true);
  EXPECT_TRUE(hub_.has_capability_(RegisterCapability::AWL_AXB));
}

TEST_F(CapabilityTest, AWLCommunicatingBlockedWithoutAny) {
  hub_.set_awl_thermostat(false);
  hub_.set_awl_iz2(false);
  EXPECT_FALSE(hub_.has_capability_(RegisterCapability::AWL_COMMUNICATING));
}

TEST_F(CapabilityTest, AWLCommunicatingAllowedWithThermostat) {
  hub_.set_awl_thermostat(true);
  hub_.set_awl_iz2(false);
  EXPECT_TRUE(hub_.has_capability_(RegisterCapability::AWL_COMMUNICATING));
}

TEST_F(CapabilityTest, AWLCommunicatingAllowedWithIZ2) {
  hub_.set_awl_thermostat(false);
  hub_.set_awl_iz2(true);
  EXPECT_TRUE(hub_.has_capability_(RegisterCapability::AWL_COMMUNICATING));
}

TEST_F(CapabilityTest, AXBBlockedWithout) {
  hub_.set_has_axb(false);
  EXPECT_FALSE(hub_.has_capability_(RegisterCapability::AXB));
}

TEST_F(CapabilityTest, AXBAllowedWith) {
  hub_.set_has_axb(true);
  EXPECT_TRUE(hub_.has_capability_(RegisterCapability::AXB));
}

TEST_F(CapabilityTest, VSDriveBlockedWithout) {
  hub_.set_has_vs_drive(false);
  EXPECT_FALSE(hub_.has_capability_(RegisterCapability::VS_DRIVE));
}

TEST_F(CapabilityTest, VSDriveAllowedWith) {
  hub_.set_has_vs_drive(true);
  EXPECT_TRUE(hub_.has_capability_(RegisterCapability::VS_DRIVE));
}

TEST_F(CapabilityTest, EnergyBlockedWithout) {
  hub_.set_has_energy_monitoring(false);
  EXPECT_FALSE(hub_.has_capability_(RegisterCapability::ENERGY));
}

TEST_F(CapabilityTest, EnergyAllowedWith) {
  hub_.set_has_energy_monitoring(true);
  EXPECT_TRUE(hub_.has_capability_(RegisterCapability::ENERGY));
}

TEST_F(CapabilityTest, RefrigerationBlockedWithout) {
  hub_.set_has_refrigeration_monitoring(false);
  EXPECT_FALSE(hub_.has_capability_(RegisterCapability::REFRIGERATION));
}

TEST_F(CapabilityTest, RefrigerationAllowedWith) {
  hub_.set_has_refrigeration_monitoring(true);
  EXPECT_TRUE(hub_.has_capability_(RegisterCapability::REFRIGERATION));
}

TEST_F(CapabilityTest, IZ2BlockedWithout) {
  hub_.set_awl_iz2(false);
  EXPECT_FALSE(hub_.has_capability_(RegisterCapability::IZ2));
}

TEST_F(CapabilityTest, IZ2AllowedWith) {
  hub_.set_awl_iz2(true);
  EXPECT_TRUE(hub_.has_capability_(RegisterCapability::IZ2));
}

// ====== build_poll_groups_ ======

class BuildPollGroupsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_millis = 0;
  }

  TestableHub hub_;

  // Helper: register a no-op listener for an address with optional capability
  void listen(uint16_t addr, RegisterCapability cap = RegisterCapability::NONE) {
    hub_.register_listener(addr, [](uint16_t) {}, cap);
  }
};

TEST_F(BuildPollGroupsTest, NoListenersProducesEmptyGroups) {
  hub_.set_awl_axb(true);
  hub_.build_poll_groups_();
  EXPECT_TRUE(hub_.poll_groups_.empty());
}

TEST_F(BuildPollGroupsTest, SingleListenerOneGroup) {
  hub_.set_awl_axb(true);
  listen(30);  // System outputs
  hub_.build_poll_groups_();

  ASSERT_EQ(hub_.poll_groups_.size(), 1u);
  EXPECT_TRUE(hub_.is_address_polled(30));
}

TEST_F(BuildPollGroupsTest, NearbyAddressesMergeIntoRange) {
  hub_.set_awl_axb(true);
  listen(19);  // FP1
  listen(20);  // FP2
  listen(25);  // Last fault
  listen(30);  // Outputs
  listen(31);  // Inputs
  hub_.build_poll_groups_();

  // These should merge into ranges (gap between 20 and 25 is 5, between 25 and 30 is 5)
  // All within gap=8, so they should merge into one range: {19, 13} (19..31)
  ASSERT_GE(hub_.poll_groups_.size(), 1u);
  EXPECT_TRUE(hub_.is_address_polled(19));
  EXPECT_TRUE(hub_.is_address_polled(20));
  EXPECT_TRUE(hub_.is_address_polled(25));
  EXPECT_TRUE(hub_.is_address_polled(30));
  EXPECT_TRUE(hub_.is_address_polled(31));
}

TEST_F(BuildPollGroupsTest, Func66SegmentForHighRegisters) {
  hub_.set_awl_axb(true);
  // Addresses in 12100-12500 range use func 66 (individual reads)
  listen(12100);
  listen(12200);
  hub_.build_poll_groups_();

  ASSERT_EQ(hub_.poll_groups_.size(), 1u);
  EXPECT_FALSE(hub_.poll_groups_[0].individual.empty());
  EXPECT_TRUE(hub_.poll_groups_[0].ranges.empty());
}

TEST_F(BuildPollGroupsTest, ConfigRegistersInSegmentA) {
  // 12005/12006 are below REGISTER_BREAKPOINT_1 (12100), so they go to segment A (func 65)
  hub_.set_awl_axb(true);
  hub_.set_awl_thermostat(true);
  listen(12005);
  listen(12006);
  hub_.build_poll_groups_();

  ASSERT_EQ(hub_.poll_groups_.size(), 1u);
  EXPECT_FALSE(hub_.poll_groups_[0].ranges.empty());
  EXPECT_TRUE(hub_.poll_groups_[0].individual.empty());
  EXPECT_TRUE(hub_.is_address_polled(12005));
  EXPECT_TRUE(hub_.is_address_polled(12006));
}

TEST_F(BuildPollGroupsTest, IZ2AddressesInSeparateSegment) {
  hub_.set_awl_axb(true);
  listen(30);     // Core register
  listen(31007);  // IZ2 zone 1 ambient
  listen(31008);  // IZ2 zone 1 config1
  hub_.build_poll_groups_();

  // Should produce at least 2 groups: one for addr < 12100, one for addr >= 31000
  ASSERT_GE(hub_.poll_groups_.size(), 2u);
  EXPECT_TRUE(hub_.is_address_polled(30));
  EXPECT_TRUE(hub_.is_address_polled(31007));
  EXPECT_TRUE(hub_.is_address_polled(31008));
}

TEST_F(BuildPollGroupsTest, GroupSplittingAt25Registers) {
  hub_.set_awl_axb(true);
  // Register 30 listeners across widely-spaced addresses that won't merge
  // Each will be its own range of 1, so after 25 we should get a new group
  for (uint16_t i = 0; i < 30; i++) {
    listen(i * 20);  // 0, 20, 40, ... 580 — gaps of 20 > max_gap(8)
  }
  hub_.build_poll_groups_();

  // With 30 individual ranges (1 register each), should split into 2 groups
  EXPECT_GE(hub_.poll_groups_.size(), 2u);
}

TEST_F(BuildPollGroupsTest, AWLFilterBlocksRegisters) {
  hub_.set_awl_axb(true);
  hub_.set_awl_thermostat(false);
  listen(30);                                             // Always pollable (NONE)
  listen(502, RegisterCapability::AWL_THERMOSTAT);        // Requires AWL thermostat
  listen(745, RegisterCapability::AWL_THERMOSTAT);        // Requires AWL thermostat
  hub_.build_poll_groups_();

  EXPECT_TRUE(hub_.is_address_polled(30));
  EXPECT_FALSE(hub_.is_address_polled(502));
  EXPECT_FALSE(hub_.is_address_polled(745));
}

TEST_F(BuildPollGroupsTest, Address740RewriteTo567WhenNoAWLAXB) {
  hub_.set_awl_axb(false);
  listen(740);  // Entering air — should be rewritten to 567
  hub_.build_poll_groups_();

  EXPECT_TRUE(hub_.is_address_polled(567));
  EXPECT_FALSE(hub_.is_address_polled(740));
}

TEST_F(BuildPollGroupsTest, Address740NotRewrittenWithAWLAXB) {
  hub_.set_awl_axb(true);
  listen(740);
  hub_.build_poll_groups_();

  EXPECT_TRUE(hub_.is_address_polled(740));
}

TEST_F(BuildPollGroupsTest, ForwardingListenerRegisteredWhenNoAWLAXB) {
  hub_.set_awl_axb(false);
  listen(740);  // Sensor listens on 740
  hub_.build_poll_groups_();

  // The forwarding listener on 567 should dispatch to 740
  // Simulate a value arriving on 567
  bool got_740 = false;
  hub_.register_listener(740, [&got_740](uint16_t v) {
    if (v == 42) got_740 = true;
  });
  hub_.dispatch_register_(567, 42);

  EXPECT_TRUE(got_740);
}

TEST_F(BuildPollGroupsTest, DuplicateListenerAddressesDeduped) {
  hub_.set_awl_axb(true);
  listen(30);
  listen(30);  // Same address, different listener
  listen(30);
  hub_.build_poll_groups_();

  // Should only produce one range entry for address 30
  ASSERT_EQ(hub_.poll_groups_.size(), 1u);
  size_t total = hub_.total_polled_registers();
  EXPECT_EQ(total, 1u);
}

TEST_F(BuildPollGroupsTest, MixedSegments) {
  hub_.set_awl_axb(true);

  // Segment A (func 65)
  listen(30);
  listen(31);

  // Segment B (func 66) — addresses in 12100-12500 range
  listen(12100);
  listen(12200);

  // Segment C (func 65, IZ2)
  listen(31007);
  listen(31008);

  hub_.build_poll_groups_();

  // Should have 3 groups: segment A, segment B, segment C
  ASSERT_EQ(hub_.poll_groups_.size(), 3u);

  // First group: func 65 ranges
  EXPECT_FALSE(hub_.poll_groups_[0].ranges.empty());
  EXPECT_TRUE(hub_.poll_groups_[0].individual.empty());

  // Second group: func 66 individual
  EXPECT_TRUE(hub_.poll_groups_[1].ranges.empty());
  EXPECT_FALSE(hub_.poll_groups_[1].individual.empty());

  // Third group: func 65 ranges (IZ2)
  EXPECT_FALSE(hub_.poll_groups_[2].ranges.empty());
  EXPECT_TRUE(hub_.poll_groups_[2].individual.empty());
}

TEST_F(BuildPollGroupsTest, TypicalFullSystem) {
  // Simulate a typical system with AWL thermostat + AXB
  hub_.set_awl_thermostat(true);
  hub_.set_awl_axb(true);

  // Core status (NONE capability)
  listen(6);    // Compressor delay
  listen(19);   // FP1
  listen(20);   // FP2
  listen(25);   // Last fault
  listen(26);   // Last lockout
  listen(27);   // Outputs at lockout
  listen(28);   // Inputs at lockout
  listen(30);   // System outputs
  listen(31);   // System inputs
  listen(344);  // ECM speed
  listen(362, RegisterCapability::VS_DRIVE);  // Active dehumidify

  // AWL thermostat
  listen(502, RegisterCapability::AWL_THERMOSTAT);   // Ambient temp
  listen(745, RegisterCapability::AWL_THERMOSTAT);   // Heating SP
  listen(746, RegisterCapability::AWL_THERMOSTAT);   // Cooling SP
  listen(747, RegisterCapability::AWL_THERMOSTAT);   // Ambient

  // AWL AXB / AWL communicating
  listen(740);                                        // Entering air (NONE)
  listen(741, RegisterCapability::AWL_COMMUNICATING); // Humidity
  listen(742, RegisterCapability::AWL_COMMUNICATING); // Outdoor temp
  listen(900, RegisterCapability::AWL_AXB);           // Leaving air

  // Thermostat config
  listen(12005, RegisterCapability::AWL_THERMOSTAT);
  listen(12006, RegisterCapability::AWL_THERMOSTAT);

  hub_.build_poll_groups_();

  // All addresses should be polled (all capabilities satisfied)
  EXPECT_TRUE(hub_.is_address_polled(6));
  EXPECT_TRUE(hub_.is_address_polled(30));
  EXPECT_TRUE(hub_.is_address_polled(502));
  EXPECT_TRUE(hub_.is_address_polled(740));
  EXPECT_TRUE(hub_.is_address_polled(900));
  EXPECT_TRUE(hub_.is_address_polled(12005));

  // Should have multiple groups
  EXPECT_GE(hub_.poll_groups_.size(), 2u);
}

TEST_F(BuildPollGroupsTest, ThirtyTwoBitSensorsBothAddressesPolled) {
  hub_.set_awl_axb(true);
  hub_.set_has_energy_monitoring(true);
  // 32-bit sensors register on addr and addr+1
  listen(1146, RegisterCapability::ENERGY);  // Compressor watts hi
  listen(1147, RegisterCapability::ENERGY);  // Compressor watts lo
  hub_.build_poll_groups_();

  EXPECT_TRUE(hub_.is_address_polled(1146));
  EXPECT_TRUE(hub_.is_address_polled(1147));
}

// ====== Capability-based filtering in build_poll_groups_ ======

TEST_F(BuildPollGroupsTest, VSDriveListenerFilteredWithoutVS) {
  hub_.set_awl_axb(true);
  hub_.set_has_vs_drive(false);
  listen(30);                                     // Always pollable (NONE)
  listen(3001, RegisterCapability::VS_DRIVE);     // VS compressor speed
  listen(3327, RegisterCapability::VS_DRIVE);     // VS drive temp
  hub_.build_poll_groups_();

  EXPECT_TRUE(hub_.is_address_polled(30));
  EXPECT_FALSE(hub_.is_address_polled(3001));
  EXPECT_FALSE(hub_.is_address_polled(3327));
}

TEST_F(BuildPollGroupsTest, VSDriveListenerAllowedWithVS) {
  hub_.set_awl_axb(true);
  hub_.set_has_vs_drive(true);
  listen(30);
  listen(3001, RegisterCapability::VS_DRIVE);
  hub_.build_poll_groups_();

  EXPECT_TRUE(hub_.is_address_polled(30));
  EXPECT_TRUE(hub_.is_address_polled(3001));
}

TEST_F(BuildPollGroupsTest, AXBListenerFilteredWithoutAXB) {
  hub_.set_has_axb(false);
  listen(30);
  listen(1111, RegisterCapability::AXB);   // Entering water temp
  listen(1110, RegisterCapability::AXB);   // Leaving water temp
  hub_.build_poll_groups_();

  EXPECT_TRUE(hub_.is_address_polled(30));
  EXPECT_FALSE(hub_.is_address_polled(1111));
  EXPECT_FALSE(hub_.is_address_polled(1110));
}

TEST_F(BuildPollGroupsTest, AXBListenerAllowedWithAXB) {
  hub_.set_has_axb(true);
  listen(30);
  listen(1111, RegisterCapability::AXB);
  hub_.build_poll_groups_();

  EXPECT_TRUE(hub_.is_address_polled(30));
  EXPECT_TRUE(hub_.is_address_polled(1111));
}

TEST_F(BuildPollGroupsTest, EnergyListenerFilteredWithoutEnergy) {
  hub_.set_has_axb(true);
  hub_.set_has_energy_monitoring(false);
  listen(30);
  listen(1146, RegisterCapability::ENERGY);
  listen(1147, RegisterCapability::ENERGY);
  hub_.build_poll_groups_();

  EXPECT_TRUE(hub_.is_address_polled(30));
  EXPECT_FALSE(hub_.is_address_polled(1146));
}

TEST_F(BuildPollGroupsTest, RefrigerationFilteredWithoutRefrigeration) {
  hub_.set_has_axb(true);
  hub_.set_has_refrigeration_monitoring(false);
  listen(30);
  listen(1124, RegisterCapability::REFRIGERATION);
  listen(1125, RegisterCapability::REFRIGERATION);
  hub_.build_poll_groups_();

  EXPECT_TRUE(hub_.is_address_polled(30));
  EXPECT_FALSE(hub_.is_address_polled(1124));
}

TEST_F(BuildPollGroupsTest, RefrigerationAllowedWithRefrigeration) {
  hub_.set_has_axb(true);
  hub_.set_has_refrigeration_monitoring(true);
  listen(30);
  listen(1124, RegisterCapability::REFRIGERATION);
  hub_.build_poll_groups_();

  EXPECT_TRUE(hub_.is_address_polled(30));
  EXPECT_TRUE(hub_.is_address_polled(1124));
}

TEST_F(BuildPollGroupsTest, IZ2ListenerFilteredWithoutIZ2) {
  hub_.set_awl_iz2(false);
  listen(30);
  listen(31003, RegisterCapability::IZ2);
  listen(31005, RegisterCapability::IZ2);
  hub_.build_poll_groups_();

  EXPECT_TRUE(hub_.is_address_polled(30));
  EXPECT_FALSE(hub_.is_address_polled(31003));
}

TEST_F(BuildPollGroupsTest, IZ2ListenerAllowedWithIZ2) {
  hub_.set_awl_iz2(true);
  listen(30);
  listen(31003, RegisterCapability::IZ2);
  hub_.build_poll_groups_();

  EXPECT_TRUE(hub_.is_address_polled(30));
  EXPECT_TRUE(hub_.is_address_polled(31003));
}

TEST_F(BuildPollGroupsTest, NoneCapabilityAlwaysPasses) {
  // Even with no capabilities set, NONE listeners should always pass
  hub_.set_awl_axb(true);  // Prevent 740→567 rewrite
  listen(6);
  listen(19);
  listen(30);
  listen(344);
  listen(740);
  listen(1117);
  hub_.build_poll_groups_();

  EXPECT_TRUE(hub_.is_address_polled(6));
  EXPECT_TRUE(hub_.is_address_polled(19));
  EXPECT_TRUE(hub_.is_address_polled(30));
  EXPECT_TRUE(hub_.is_address_polled(344));
  EXPECT_TRUE(hub_.is_address_polled(740));
  EXPECT_TRUE(hub_.is_address_polled(1117));
}
