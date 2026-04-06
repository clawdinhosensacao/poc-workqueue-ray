#include <gtest/gtest.h>

#include "rtm3d/core/Volume3D.hpp"
#include "../src/rtm/Receivers.hpp"

TEST(RtmReceivers, BuildsBoundedReceiverPositions) {
  rtm3d::Volume3D vel(17, 8, 10, 1500.0f);

  const auto rx = rtm3d::rtm_internal::make_receiver_positions(vel, 4);

  ASSERT_GE(rx.size(), 2u);
  EXPECT_EQ(rx.front(), 1u);
  EXPECT_LE(rx.back(), vel.nx() - 2);
  for (std::size_t i = 1; i < rx.size(); ++i) {
    EXPECT_GE(rx[i], rx[i - 1]);
  }
}

TEST(RtmReceivers, RecordAndInjectRoundTripAtSampleIndex) {
  rtm3d::Volume3D vel(12, 6, 8, 1500.0f);
  const std::size_t sy = vel.ny() / 2;
  const std::size_t sz = 2;
  const auto rx = rtm3d::rtm_internal::make_receiver_positions(vel, 3);

  std::vector<float> src_field(vel.size(), 0.0f);
  for (std::size_t ir = 0; ir < rx.size(); ++ir) {
    src_field[vel.index(rx[ir], sy, sz)] = static_cast<float>(ir + 1);
  }

  std::vector<float> rec_data(2 * rx.size(), 0.0f);
  rtm3d::rtm_internal::record_receivers(vel, sy, sz, rx, src_field, rec_data, 1);

  std::vector<float> rec_field(vel.size(), 0.0f);
  rtm3d::rtm_internal::inject_receivers(vel, sy, sz, rx, rec_data, 1, rec_field);

  for (std::size_t ir = 0; ir < rx.size(); ++ir) {
    EXPECT_FLOAT_EQ(rec_field[vel.index(rx[ir], sy, sz)], static_cast<float>(ir + 1));
  }
}

TEST(RtmReceivers, StrideAffectsReceiverCount) {
  rtm3d::Volume3D vel(50, 10, 20, 1500.0f);

  const auto rx1 = rtm3d::rtm_internal::make_receiver_positions(vel, 5);
  const auto rx2 = rtm3d::rtm_internal::make_receiver_positions(vel, 10);

  // Larger stride should produce fewer receivers
  EXPECT_GT(rx1.size(), rx2.size());
}

TEST(RtmReceivers, PositionsAreMonotonic) {
  rtm3d::Volume3D vel(100, 20, 30, 2000.0f);

  const auto rx = rtm3d::rtm_internal::make_receiver_positions(vel, 7);

  // Positions should be monotonically increasing
  for (std::size_t i = 1; i < rx.size(); ++i) {
    EXPECT_GT(rx[i], rx[i - 1]) << "Receiver positions not monotonic at index " << i;
  }
}

TEST(RtmReceivers, InjectAccumulatesValues) {
  rtm3d::Volume3D vel(10, 4, 6, 1500.0f);
  const std::size_t sy = vel.ny() / 2;
  const std::size_t sz = 2;
  const auto rx = rtm3d::rtm_internal::make_receiver_positions(vel, 2);

  std::vector<float> rec_data(2 * rx.size(), 0.0f);
  for (std::size_t ir = 0; ir < rx.size(); ++ir) {
    rec_data[1 * rx.size() + ir] = static_cast<float>(ir + 1);
  }

  std::vector<float> rec_field(vel.size(), 0.0f);

  // Inject twice - should accumulate
  rtm3d::rtm_internal::inject_receivers(vel, sy, sz, rx, rec_data, 1, rec_field);
  rtm3d::rtm_internal::inject_receivers(vel, sy, sz, rx, rec_data, 1, rec_field);

  // Values should be doubled
  for (std::size_t ir = 0; ir < rx.size(); ++ir) {
    EXPECT_FLOAT_EQ(rec_field[vel.index(rx[ir], sy, sz)], 2.0f * static_cast<float>(ir + 1));
  }
}

TEST(RtmReceivers, DifferentTimeIndexAccessesCorrectSlot) {
  rtm3d::Volume3D vel(8, 4, 4, 1500.0f);
  const std::size_t sy = vel.ny() / 2;
  const std::size_t sz = 1;
  const auto rx = rtm3d::rtm_internal::make_receiver_positions(vel, 2);

  std::vector<float> src_field(vel.size(), 0.0f);
  for (std::size_t ir = 0; ir < rx.size(); ++ir) {
    src_field[vel.index(rx[ir], sy, sz)] = static_cast<float>(100 + ir);
  }

  // Record at time index 5
  std::vector<float> rec_data(10 * rx.size(), 0.0f);
  rtm3d::rtm_internal::record_receivers(vel, sy, sz, rx, src_field, rec_data, 5);

  // Verify data is at correct slot (it=5)
  for (std::size_t ir = 0; ir < rx.size(); ++ir) {
    EXPECT_FLOAT_EQ(rec_data[5 * rx.size() + ir], static_cast<float>(100 + ir));
  }
}
