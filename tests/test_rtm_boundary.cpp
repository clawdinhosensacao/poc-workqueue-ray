#include <gtest/gtest.h>

#include "rtm3d/core/Volume3D.hpp"
#include "../src/rtm/Boundary.hpp"

TEST(RtmBoundary, DampIsUnityInsideAndAttenuatedNearEdges) {
  const std::size_t nx = 21, ny = 11, nz = 13, pml = 4;
  const auto damp = rtm3d::rtm_internal::make_damp(nx, ny, nz, pml);

  auto idx = [=](std::size_t x, std::size_t y, std::size_t z) { return (z * ny + y) * nx + x; };

  const std::size_t cx = nx / 2;
  const std::size_t cy = ny / 2;
  const std::size_t cz = nz / 2;

  EXPECT_FLOAT_EQ(damp[idx(cx, cy, cz)], 1.0f);
  EXPECT_LT(damp[idx(0, cy, cz)], 1.0f);
  EXPECT_LT(damp[idx(cx, 0, cz)], 1.0f);
  EXPECT_LT(damp[idx(cx, cy, 0)], 1.0f);
}
