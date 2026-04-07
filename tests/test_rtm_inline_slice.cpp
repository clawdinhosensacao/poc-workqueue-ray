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

TEST(RtmInlineSlice, ExtractsCrosslineYZAtCenterX) {
  rtm3d::Volume3D vol(3, 5, 2, 1500.0f);
  std::vector<float> image(vol.size(), 0.0f);

  const std::size_t xmid = vol.nx() / 2;
  for (std::size_t z = 0; z < vol.nz(); ++z) {
    for (std::size_t y = 0; y < vol.ny(); ++y) {
      image[vol.index(xmid, y, z)] = static_cast<float>(10 * z + y);
    }
  }

  const auto crossline = rtm3d::rtm_internal::extract_crossline_yz(vol, image);
  ASSERT_EQ(crossline.size(), vol.ny() * vol.nz());

  // Check first row (z=0)
  for (std::size_t y = 0; y < vol.ny(); ++y) {
    EXPECT_FLOAT_EQ(crossline[y], static_cast<float>(y));
  }
}

TEST(RtmInlineSlice, ExtractsDepthXYAtGivenZ) {
  rtm3d::Volume3D vol(3, 5, 2, 1500.0f);
  std::vector<float> image(vol.size(), 0.0f);

  const std::size_t target_z = 1;
  for (std::size_t y = 0; y < vol.ny(); ++y) {
    for (std::size_t x = 0; x < vol.nx(); ++x) {
      image[vol.index(x, y, target_z)] = static_cast<float>(100 * y + x);
    }
  }

  const auto depth_slice = rtm3d::rtm_internal::extract_depth_xy(vol, image, target_z);
  ASSERT_EQ(depth_slice.size(), vol.nx() * vol.ny());

  // Check first row (y=0)
  EXPECT_FLOAT_EQ(depth_slice[0], 0.0f);
  EXPECT_FLOAT_EQ(depth_slice[1], 1.0f);
  EXPECT_FLOAT_EQ(depth_slice[2], 2.0f);

  // Check second row (y=1)
  EXPECT_FLOAT_EQ(depth_slice[3], 100.0f);
  EXPECT_FLOAT_EQ(depth_slice[4], 101.0f);
  EXPECT_FLOAT_EQ(depth_slice[5], 102.0f);
}

TEST(RtmInlineSlice, ClampsDepthIndexToValidRange) {
  rtm3d::Volume3D vol(3, 5, 2, 1500.0f);
  std::vector<float> image(vol.size(), 1.0f);

  // Request out-of-range depth
  const auto slice = rtm3d::rtm_internal::extract_depth_xy(vol, image, 100);
  ASSERT_EQ(slice.size(), vol.nx() * vol.ny());

  // Should return slice at z = nz-1
  for (float v : slice) {
    EXPECT_FLOAT_EQ(v, 1.0f);
  }
}

TEST(RtmInlineSlice, HandlesSingleZPlane) {
  rtm3d::Volume3D vol(4, 3, 1, 1500.0f);  // nz = 1
  std::vector<float> image(vol.size(), 0.0f);

  // Fill the only z-plane
  for (std::size_t y = 0; y < vol.ny(); ++y) {
    for (std::size_t x = 0; x < vol.nx(); ++x) {
      image[vol.index(x, y, 0)] = static_cast<float>(y * 10 + x);
    }
  }

  const auto inline_xz = rtm3d::rtm_internal::extract_inline_xz(vol, image);
  ASSERT_EQ(inline_xz.size(), vol.nx() * vol.nz());

  // Should extract the middle y-slice from the single z-plane
  std::size_t ymid = vol.ny() / 2;
  for (std::size_t x = 0; x < vol.nx(); ++x) {
    EXPECT_FLOAT_EQ(inline_xz[x], static_cast<float>(ymid * 10 + x));
  }
}

TEST(RtmInlineSlice, HandlesLargeNy) {
  rtm3d::Volume3D vol(4, 100, 3, 1500.0f);  // ny = 100
  std::vector<float> image(vol.size(), 0.0f);

  // Fill with y-dependent values
  for (std::size_t z = 0; z < vol.nz(); ++z) {
    for (std::size_t y = 0; y < vol.ny(); ++y) {
      for (std::size_t x = 0; x < vol.nx(); ++x) {
        image[vol.index(x, y, z)] = static_cast<float>(y);
      }
    }
  }

  const auto inline_xz = rtm3d::rtm_internal::extract_inline_xz(vol, image);
  ASSERT_EQ(inline_xz.size(), vol.nx() * vol.nz());

  // All values should be from middle y-slice (y = 50)
  for (float v : inline_xz) {
    EXPECT_FLOAT_EQ(v, 50.0f);
  }
}

TEST(RtmInlineSlice, CrosslineHandlesSingleX) {
  rtm3d::Volume3D vol(1, 5, 3, 1500.0f);  // nx = 1
  std::vector<float> image(vol.size(), 0.0f);

  for (std::size_t z = 0; z < vol.nz(); ++z) {
    for (std::size_t y = 0; y < vol.ny(); ++y) {
      image[vol.index(0, y, z)] = static_cast<float>(z * 10 + y);
    }
  }

  const auto crossline = rtm3d::rtm_internal::extract_crossline_yz(vol, image);
  ASSERT_EQ(crossline.size(), vol.ny() * vol.nz());

  // All values should match since x=0 is the only column
  for (std::size_t z = 0; z < vol.nz(); ++z) {
    for (std::size_t y = 0; y < vol.ny(); ++y) {
      std::size_t idx = z * vol.ny() + y;
      EXPECT_FLOAT_EQ(crossline[idx], static_cast<float>(z * 10 + y));
    }
  }
}

TEST(RtmInlineSlice, DepthSliceAtZeroZ) {
  rtm3d::Volume3D vol(3, 4, 5, 1500.0f);
  std::vector<float> image(vol.size(), 0.0f);

  // Fill z=0 plane with specific values
  for (std::size_t y = 0; y < vol.ny(); ++y) {
    for (std::size_t x = 0; x < vol.nx(); ++x) {
      image[vol.index(x, y, 0)] = static_cast<float>(y * 100 + x);
    }
  }

  const auto depth_slice = rtm3d::rtm_internal::extract_depth_xy(vol, image, 0);
  ASSERT_EQ(depth_slice.size(), vol.nx() * vol.ny());

  // Verify values from z=0
  for (std::size_t y = 0; y < vol.ny(); ++y) {
    for (std::size_t x = 0; x < vol.nx(); ++x) {
      std::size_t idx = y * vol.nx() + x;
      EXPECT_FLOAT_EQ(depth_slice[idx], static_cast<float>(y * 100 + x));
    }
  }
}
