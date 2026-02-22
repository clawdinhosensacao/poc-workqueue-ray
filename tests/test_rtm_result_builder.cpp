#include <gtest/gtest.h>

#include "rtm3d/core/Volume3D.hpp"
#include "../src/rtm/ResultBuilder.hpp"

TEST(RtmResultBuilder, BuildsInlineWithExpectedShapeAndValues) {
  rtm3d::Volume3D vel(4, 3, 2, 1500.0f);
  std::vector<float> image(vel.size(), 0.0f);

  const std::size_t ymid = vel.ny() / 2;
  for (std::size_t z = 0; z < vel.nz(); ++z) {
    for (std::size_t x = 0; x < vel.nx(); ++x) {
      image[vel.index(x, ymid, z)] = static_cast<float>(10 * z + x);
    }
  }

  const auto out = rtm3d::rtm_internal::build_migration_result(vel, image);

  EXPECT_EQ(out.nx, 4u);
  EXPECT_EQ(out.nz, 2u);
  ASSERT_EQ(out.inline_xz.size(), 8u);
  EXPECT_FLOAT_EQ(out.inline_xz[0], 0.0f);
  EXPECT_FLOAT_EQ(out.inline_xz[3], 3.0f);
  EXPECT_FLOAT_EQ(out.inline_xz[4], 10.0f);
  EXPECT_FLOAT_EQ(out.inline_xz[7], 13.0f);
}
