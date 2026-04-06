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

TEST(RtmResultBuilder, OutputSizeMatchesVolumeNxNz) {
  rtm3d::Volume3D vel(20, 10, 15, 1500.0f);
  std::vector<float> image(vel.size(), 0.0f);

  const auto out = rtm3d::rtm_internal::build_migration_result(vel, image);

  EXPECT_EQ(out.nx, 20u);
  EXPECT_EQ(out.nz, 15u);
  EXPECT_EQ(out.inline_xz.size(), 20u * 15u);
}

TEST(RtmResultBuilder, ExtractsCorrectYSlice) {
  rtm3d::Volume3D vel(3, 5, 2, 0.0f);
  std::vector<float> image(vel.size(), 0.0f);

  // Fill with unique values to identify correct Y slice
  for (std::size_t y = 0; y < vel.ny(); ++y) {
    for (std::size_t z = 0; z < vel.nz(); ++z) {
      for (std::size_t x = 0; x < vel.nx(); ++x) {
        image[vel.index(x, y, z)] = static_cast<float>(100 * y + 10 * z + x);
      }
    }
  }

  const auto out = rtm3d::rtm_internal::build_migration_result(vel, image);

  // Should extract Y = ny/2 = 2
  // Values should be 200 + 10*z + x
  EXPECT_FLOAT_EQ(out.inline_xz[0], 200.0f);   // y=2, z=0, x=0
  EXPECT_FLOAT_EQ(out.inline_xz[1], 201.0f);   // y=2, z=0, x=1
  EXPECT_FLOAT_EQ(out.inline_xz[2], 202.0f);   // y=2, z=0, x=2
  EXPECT_FLOAT_EQ(out.inline_xz[3], 210.0f);   // y=2, z=1, x=0
}

TEST(RtmResultBuilder, WorksForEvenNy) {
  rtm3d::Volume3D vel(4, 4, 3, 1500.0f);
  std::vector<float> image(vel.size(), 1.0f);

  const auto out = rtm3d::rtm_internal::build_migration_result(vel, image);

  EXPECT_EQ(out.nx, 4u);
  EXPECT_EQ(out.nz, 3u);
  for (float v : out.inline_xz) {
    EXPECT_FLOAT_EQ(v, 1.0f);
  }
}

TEST(RtmResultBuilder, WorksForOddNy) {
  rtm3d::Volume3D vel(4, 5, 3, 1500.0f);
  std::vector<float> image(vel.size(), 2.5f);

  const auto out = rtm3d::rtm_internal::build_migration_result(vel, image);

  EXPECT_EQ(out.nx, 4u);
  EXPECT_EQ(out.nz, 3u);
  for (float v : out.inline_xz) {
    EXPECT_FLOAT_EQ(v, 2.5f);
  }
}

TEST(RtmResultBuilder, MinimalVolumeProducesCorrectOutput) {
  rtm3d::Volume3D vel(1, 1, 1, 42.0f);
  std::vector<float> image(1, 42.0f);

  const auto out = rtm3d::rtm_internal::build_migration_result(vel, image);

  EXPECT_EQ(out.nx, 1u);
  EXPECT_EQ(out.nz, 1u);
  ASSERT_EQ(out.inline_xz.size(), 1u);
  EXPECT_FLOAT_EQ(out.inline_xz[0], 42.0f);
}
