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

TEST(RtmBoundary, DampSizeMatchesVolume) {
  const std::size_t nx = 24, ny = 16, nz = 12, pml = 4;
  const auto damp = rtm3d::rtm_internal::make_damp(nx, ny, nz, pml);

  EXPECT_EQ(damp.size(), nx * ny * nz);
}

TEST(RtmBoundary, DampWithMinimalPml) {
  const std::size_t nx = 10, ny = 8, nz = 6, pml = 1;
  const auto damp = rtm3d::rtm_internal::make_damp(nx, ny, nz, pml);

  auto idx = [=](std::size_t x, std::size_t y, std::size_t z) { return (z * ny + y) * nx + x; };

  // With pml=1, only the outermost layer is attenuated
  EXPECT_LT(damp[idx(0, ny/2, nz/2)], 1.0f);
  EXPECT_FLOAT_EQ(damp[idx(1, ny/2, nz/2)], 1.0f);
}

TEST(RtmBoundary, DampWithLargePml) {
  const std::size_t nx = 30, ny = 25, nz = 20, pml = 10;
  const auto damp = rtm3d::rtm_internal::make_damp(nx, ny, nz, pml);

  auto idx = [=](std::size_t x, std::size_t y, std::size_t z) { return (z * ny + y) * nx + x; };

  // Interior should be close to unity (but may not be exactly 1.0 with large PML)
  std::size_t cx = nx / 2, cy = ny / 2, cz = nz / 2;
  EXPECT_GT(damp[idx(cx, cy, cz)], 0.9f);

  // Edge should be significantly attenuated
  EXPECT_LT(damp[idx(0, cy, cz)], 0.8f);
}

TEST(RtmBoundary, DampSymmetricInX) {
  const std::size_t nx = 20, ny = 10, nz = 8, pml = 4;
  const auto damp = rtm3d::rtm_internal::make_damp(nx, ny, nz, pml);

  auto idx = [=](std::size_t x, std::size_t y, std::size_t z) { return (z * ny + y) * nx + x; };

  // Check symmetry: damp[i] should equal damp[nx-1-i] in X direction
  std::size_t cy = ny / 2, cz = nz / 2;
  for (std::size_t i = 0; i < nx / 2; ++i) {
    EXPECT_FLOAT_EQ(damp[idx(i, cy, cz)], damp[idx(nx - 1 - i, cy, cz)])
        << "Damping not symmetric at X=" << i;
  }
}

TEST(RtmBoundary, DampSymmetricInY) {
  const std::size_t nx = 15, ny = 20, nz = 10, pml = 5;
  const auto damp = rtm3d::rtm_internal::make_damp(nx, ny, nz, pml);

  auto idx = [=](std::size_t x, std::size_t y, std::size_t z) { return (z * ny + y) * nx + x; };

  // Check symmetry in Y direction
  std::size_t cx = nx / 2, cz = nz / 2;
  for (std::size_t j = 0; j < ny / 2; ++j) {
    EXPECT_FLOAT_EQ(damp[idx(cx, j, cz)], damp[idx(cx, ny - 1 - j, cz)])
        << "Damping not symmetric at Y=" << j;
  }
}

TEST(RtmBoundary, DampSymmetricInZ) {
  const std::size_t nx = 12, ny = 10, nz = 18, pml = 4;
  const auto damp = rtm3d::rtm_internal::make_damp(nx, ny, nz, pml);

  auto idx = [=](std::size_t x, std::size_t y, std::size_t z) { return (z * ny + y) * nx + x; };

  // Check symmetry in Z direction
  std::size_t cx = nx / 2, cy = ny / 2;
  for (std::size_t k = 0; k < nz / 2; ++k) {
    EXPECT_FLOAT_EQ(damp[idx(cx, cy, k)], damp[idx(cx, cy, nz - 1 - k)])
        << "Damping not symmetric at Z=" << k;
  }
}

TEST(RtmBoundary, DampAllValuesValid) {
  const std::size_t nx = 15, ny = 12, nz = 10, pml = 3;
  const auto damp = rtm3d::rtm_internal::make_damp(nx, ny, nz, pml);

  // All values should be in (0, 1] range
  for (float v : damp) {
    EXPECT_GT(v, 0.0f);
    EXPECT_LE(v, 1.0f);
  }
}
