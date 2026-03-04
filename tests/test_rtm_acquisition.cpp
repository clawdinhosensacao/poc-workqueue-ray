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
