#include <gtest/gtest.h>

#include "rtm3d/core/Volume3D.hpp"
#include "../src/rtm/Acquisition.hpp"

TEST(RtmAcquisition, BuildsCenteredDefaultShotGeometry) {
  rtm3d::Volume3D vel(20, 10, 16, 1500.0f);

  const auto shot = rtm3d::rtm_internal::make_default_shot_geometry(vel, 4);

  EXPECT_EQ(shot.sx, 10u);
  EXPECT_EQ(shot.sy, 5u);
  EXPECT_EQ(shot.sz, 2u);
  ASSERT_GE(shot.rx.size(), 2u);
  EXPECT_EQ(shot.rx.front(), 1u);
  EXPECT_LE(shot.rx.back(), vel.nx() - 2);
}

TEST(RtmAcquisition, BuildsCustomShotGeometry) {
  rtm3d::Volume3D vel(30, 12, 20, 2000.0f);

  const auto shot = rtm3d::rtm_internal::make_shot_geometry(vel, 15, 3, 5);

  EXPECT_EQ(shot.sx, 15u);
  EXPECT_EQ(shot.sy, 6u);  // ny/2
  EXPECT_EQ(shot.sz, 3u);
  ASSERT_GE(shot.rx.size(), 2u);
  EXPECT_EQ(shot.rx.front(), 1u);
  EXPECT_LE(shot.rx.back(), vel.nx() - 2);
}

TEST(RtmAcquisition, CustomShotGeometryRespectsReceiverStride) {
  rtm3d::Volume3D vel(40, 8, 24, 1800.0f);

  const auto shot1 = rtm3d::rtm_internal::make_shot_geometry(vel, 10, 2, 8);
  const auto shot2 = rtm3d::rtm_internal::make_shot_geometry(vel, 25, 4, 4);

  // Larger stride means fewer receivers
  EXPECT_GT(shot2.rx.size(), shot1.rx.size());

  // Both should have valid receiver positions
  for (const auto& shot : {shot1, shot2}) {
    for (std::size_t rx : shot.rx) {
      EXPECT_GE(rx, 1u);
      EXPECT_LT(rx, vel.nx() - 1);
    }
  }
}
