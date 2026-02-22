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
