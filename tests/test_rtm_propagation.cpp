#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "rtm3d/core/Volume3D.hpp"
#include "../src/rtm/Propagation.hpp"

TEST(RtmPropagation, StepPreservesZeroWavefield) {
  // Uniform velocity, zero initial conditions → zero output
  rtm3d::Volume3D vel(10, 8, 12, 2000.0f);
  std::vector<float> damp(vel.size(), 1.0f);
  std::vector<float> prev(vel.size(), 0.0f);
  std::vector<float> cur(vel.size(), 0.0f);
  std::vector<float> nxt(vel.size(), 0.0f);

  rtm3d::rtm_internal::step_fd3d(vel, damp, 0.001f, 10.0f, 10.0f, 10.0f, prev, cur, nxt);

  for (float v : nxt) {
    EXPECT_FLOAT_EQ(v, 0.0f);
  }
}

TEST(RtmPropagation, StepWithConstantFieldProducesZeroLaplacian) {
  // Constant (non-zero) wavefield has zero Laplacian → nxt = 2*cur - prev
  rtm3d::Volume3D vel(10, 8, 12, 2000.0f);
  std::vector<float> damp(vel.size(), 1.0f);
  std::vector<float> prev(vel.size(), 1.0f);
  std::vector<float> cur(vel.size(), 1.0f);
  std::vector<float> nxt(vel.size(), 0.0f);

  rtm3d::rtm_internal::step_fd3d(vel, damp, 0.001f, 10.0f, 10.0f, 10.0f, prev, cur, nxt);

  // For interior points: nxt = (2*1 - 1 + v²*dt²*0) * 1 = 1.0
  // Boundary points remain 0 (not updated)
  for (std::size_t iz = 1; iz + 1 < vel.nz(); ++iz) {
    for (std::size_t iy = 1; iy + 1 < vel.ny(); ++iy) {
      for (std::size_t ix = 1; ix + 1 < vel.nx(); ++ix) {
        EXPECT_FLOAT_EQ(nxt[vel.index(ix, iy, iz)], 1.0f)
            << "at (ix=" << ix << ", iy=" << iy << ", iz=" << iz << ")";
      }
    }
  }
}

TEST(RtmPropagation, StepAppliesDampingToWavefield) {
  // With damping < 1, output should be attenuated
  rtm3d::Volume3D vel(10, 8, 12, 2000.0f);
  std::vector<float> damp(vel.size(), 1.0f);
  // Set damping at interior center to 0.5
  const std::size_t cx = vel.nx() / 2;
  const std::size_t cy = vel.ny() / 2;
  const std::size_t cz = vel.nz() / 2;
  damp[vel.index(cx, cy, cz)] = 0.5f;

  std::vector<float> prev(vel.size(), 0.0f);
  std::vector<float> cur(vel.size(), 0.0f);
  std::vector<float> nxt(vel.size(), 0.0f);

  // Place a point source at center of cur
  cur[vel.index(cx, cy, cz)] = 1.0f;

  rtm3d::rtm_internal::step_fd3d(vel, damp, 0.001f, 10.0f, 10.0f, 10.0f, prev, cur, nxt);

  // With damping=0.5, the center point should be attenuated
  // nxt[center] = (2*1 - 0 + v²*dt²*lap) * 0.5
  // The Laplacian at center: each neighbor contributes, but center is 1, neighbors are 0
  // lap = (0 - 2*1 + 0)/dx² + (0 - 2*1 + 0)/dy² + (0 - 2*1 + 0)/dz²
  //     = -6/dx² = -6/100 = -0.06
  // nxt = (2*1 - 0 + 2000²*0.001²*(-0.06)) * 0.5
  //     = (2 + 4*(-0.06)) * 0.5 = (2 - 0.24) * 0.5 = 1.76 * 0.5 = 0.88
  const float expected_center = (2.0f + 2000.0f * 2000.0f * 0.001f * 0.001f * (-0.06f)) * 0.5f;
  EXPECT_NEAR(nxt[vel.index(cx, cy, cz)], expected_center, 1e-4f);
}

TEST(RtmPropagation, BoundaryPointsRemainZero) {
  // Boundary points (ix=0, iy=0, iz=0, ix=nx-1, etc.) should stay 0
  rtm3d::Volume3D vel(10, 8, 12, 2000.0f);
  std::vector<float> damp(vel.size(), 1.0f);
  std::vector<float> prev(vel.size(), 0.0f);
  std::vector<float> cur(vel.size(), 1.0f);  // non-zero everywhere
  std::vector<float> nxt(vel.size(), 0.0f);

  rtm3d::rtm_internal::step_fd3d(vel, damp, 0.001f, 10.0f, 10.0f, 10.0f, prev, cur, nxt);

  // Check all boundary faces
  for (std::size_t iy = 0; iy < vel.ny(); ++iy) {
    for (std::size_t ix = 0; ix < vel.nx(); ++ix) {
      EXPECT_FLOAT_EQ(nxt[vel.index(ix, iy, 0)], 0.0f) << "z=0 boundary";
      EXPECT_FLOAT_EQ(nxt[vel.index(ix, iy, vel.nz() - 1)], 0.0f) << "z=nz-1 boundary";
    }
  }
  for (std::size_t iz = 0; iz < vel.nz(); ++iz) {
    for (std::size_t ix = 0; ix < vel.nx(); ++ix) {
      EXPECT_FLOAT_EQ(nxt[vel.index(ix, 0, iz)], 0.0f) << "y=0 boundary";
      EXPECT_FLOAT_EQ(nxt[vel.index(ix, vel.ny() - 1, iz)], 0.0f) << "y=ny-1 boundary";
    }
  }
  for (std::size_t iz = 0; iz < vel.nz(); ++iz) {
    for (std::size_t iy = 0; iy < vel.ny(); ++iy) {
      EXPECT_FLOAT_EQ(nxt[vel.index(0, iy, iz)], 0.0f) << "x=0 boundary";
      EXPECT_FLOAT_EQ(nxt[vel.index(vel.nx() - 1, iy, iz)], 0.0f) << "x=nx-1 boundary";
    }
  }
}

TEST(RtmPropagation, PointSourcePropagatesOutward) {
  // A point source should create outward-spreading wavefield
  rtm3d::Volume3D vel(20, 10, 16, 1500.0f);
  std::vector<float> damp(vel.size(), 1.0f);
  std::vector<float> prev(vel.size(), 0.0f);
  std::vector<float> cur(vel.size(), 0.0f);
  std::vector<float> nxt(vel.size(), 0.0f);

  // Place point source at center
  const std::size_t cx = vel.nx() / 2;
  const std::size_t cy = vel.ny() / 2;
  const std::size_t cz = vel.nz() / 2;
  cur[vel.index(cx, cy, cz)] = 1.0f;

  rtm3d::rtm_internal::step_fd3d(vel, damp, 0.001f, 10.0f, 10.0f, 10.0f, prev, cur, nxt);

  // After one step, neighbors of the source should be non-zero
  // (they have non-zero Laplacian contribution from the source point)
  bool has_nonzero_neighbor = false;
  const std::size_t neighbors[] = {
      vel.index(cx + 1, cy, cz), vel.index(cx - 1, cy, cz),
      vel.index(cx, cy + 1, cz), vel.index(cx, cy - 1, cz),
      vel.index(cx, cy, cz + 1), vel.index(cx, cy, cz - 1)};
  for (auto idx : neighbors) {
    if (std::abs(nxt[idx]) > 1e-10f) {
      has_nonzero_neighbor = true;
      break;
    }
  }
  EXPECT_TRUE(has_nonzero_neighbor) << "Point source should propagate to neighbors";
}
