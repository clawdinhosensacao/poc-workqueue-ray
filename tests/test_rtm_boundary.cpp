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

TEST(RtmBoundary, DampIsMonotonicFromEdgeToInterior) {
  const std::size_t nx = 30, ny = 20, nz = 25, pml = 6;
  const auto damp = rtm3d::rtm_internal::make_damp(nx, ny, nz, pml);

  auto idx = [=](std::size_t x, std::size_t y, std::size_t z) { return (z * ny + y) * nx + x; };

  // Check X-direction monotonicity: damping should increase from edge toward interior
  for (std::size_t i = 0; i < pml - 1; ++i) {
    EXPECT_LT(damp[idx(i, ny/2, nz/2)], damp[idx(i + 1, ny/2, nz/2)])
        << "Damping not monotonic at X=" << i;
  }

  // Check Y-direction monotonicity
  for (std::size_t j = 0; j < pml - 1; ++j) {
    EXPECT_LT(damp[idx(nx/2, j, nz/2)], damp[idx(nx/2, j + 1, nz/2)])
        << "Damping not monotonic at Y=" << j;
  }

  // Check Z-direction monotonicity
  for (std::size_t k = 0; k < pml - 1; ++k) {
    EXPECT_LT(damp[idx(nx/2, ny/2, k)], damp[idx(nx/2, ny/2, k + 1)])
        << "Damping not monotonic at Z=" << k;
  }
}

TEST(RtmBoundary, DampAtEdgeIsBelowThreshold) {
  const std::size_t nx = 20, ny = 15, nz = 10, pml = 4;
  const auto damp = rtm3d::rtm_internal::make_damp(nx, ny, nz, pml);

  auto idx = [=](std::size_t x, std::size_t y, std::size_t z) { return (z * ny + y) * nx + x; };

  // At the very edge, damping should be significantly < 1
  EXPECT_LT(damp[idx(0, ny/2, nz/2)], 0.5f);
  EXPECT_LT(damp[idx(nx-1, ny/2, nz/2)], 0.5f);
  EXPECT_LT(damp[idx(nx/2, 0, nz/2)], 0.5f);
  EXPECT_LT(damp[idx(nx/2, ny-1, nz/2)], 0.5f);
  EXPECT_LT(damp[idx(nx/2, ny/2, 0)], 0.5f);
  EXPECT_LT(damp[idx(nx/2, ny/2, nz-1)], 0.5f);
}

TEST(RtmBoundary, DampAtPmlBoundaryIsUnity) {
  const std::size_t nx = 20, ny = 15, nz = 10, pml = 4;
  const auto damp = rtm3d::rtm_internal::make_damp(nx, ny, nz, pml);

  auto idx = [=](std::size_t x, std::size_t y, std::size_t z) { return (z * ny + y) * nx + x; };

  // At exactly pml cells from edge, damping should be 1.0 (interior)
  EXPECT_FLOAT_EQ(damp[idx(pml, ny/2, nz/2)], 1.0f);
  EXPECT_FLOAT_EQ(damp[idx(nx - pml - 1, ny/2, nz/2)], 1.0f);
}
