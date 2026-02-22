#include <gtest/gtest.h>

#include "rtm3d/core/Volume3D.hpp"
#include "../src/rtm/InlineSlice.hpp"

TEST(RtmInlineSlice, ExtractsMiddleYPlaneInXZOrder) {
  rtm3d::Volume3D vel(3, 5, 2, 1500.0f);
  std::vector<float> image(vel.size(), 0.0f);

  const std::size_t ymid = vel.ny() / 2;
  for (std::size_t z = 0; z < vel.nz(); ++z) {
    for (std::size_t x = 0; x < vel.nx(); ++x) {
      image[vel.index(x, ymid, z)] = static_cast<float>(100 * z + x);
    }
  }

  const auto inline_xz = rtm3d::rtm_internal::extract_inline_xz(vel, image);
  ASSERT_EQ(inline_xz.size(), vel.nx() * vel.nz());
  EXPECT_FLOAT_EQ(inline_xz[0], 0.0f);
  EXPECT_FLOAT_EQ(inline_xz[1], 1.0f);
  EXPECT_FLOAT_EQ(inline_xz[2], 2.0f);
  EXPECT_FLOAT_EQ(inline_xz[3], 100.0f);
  EXPECT_FLOAT_EQ(inline_xz[4], 101.0f);
  EXPECT_FLOAT_EQ(inline_xz[5], 102.0f);
}
