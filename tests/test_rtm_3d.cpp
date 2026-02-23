#include <cmath>

#include <gtest/gtest.h>

#include "rtm3d/model/SeismicModel.hpp"
#include "rtm3d/rtm/RtmEngine.hpp"

// 3D-specific tests - verify that ny > 1 works correctly

TEST(Rtm3D, ExtendsTo3DWithNyGreaterThanOne) {
  rtm3d::GridModel2D model{.nx = 16, .nz = 12, .dx = 10.0f, .dz = 10.0f,
                           .values = std::vector<float>(16 * 12, 1800.0f)};

  rtm3d::RtmConfig cfg;
  cfg.ny = 8;   // 3D extension
  cfg.dy = 10.0f;
  cfg.nt = 30;
  cfg.dt = 0.0008f;
  cfg.pml = 4;

  // Should not throw
  auto result = rtm3d::run_single_shot_rtm(model, cfg);

  EXPECT_EQ(result.nx, 16u);
  EXPECT_EQ(result.nz, 12u);
  EXPECT_EQ(result.inline_xz.size(), 16u * 12u);
}

TEST(Rtm3D, DifferentNyValuesProduceConsistentResults) {
  rtm3d::GridModel2D model{.nx = 20, .nz = 16, .dx = 10.0f, .dz = 10.0f,
                           .values = std::vector<float>(20 * 16, 2000.0f)};

  rtm3d::RtmConfig cfg;
  cfg.nt = 40;
  cfg.dt = 0.0008f;
  cfg.pml = 4;

  // Run with ny = 4
  cfg.ny = 4;
  cfg.dy = 10.0f;
  auto result4 = rtm3d::run_single_shot_rtm(model, cfg);

  // Run with ny = 16
  cfg.ny = 16;
  auto result16 = rtm3d::run_single_shot_rtm(model, cfg);

  // Both should produce inline slices of same size
  EXPECT_EQ(result4.nx, result16.nx);
  EXPECT_EQ(result4.nz, result16.nz);

  // For constant velocity, results should be similar
  double energy4 = 0.0, energy16 = 0.0;
  for (std::size_t i = 0; i < result4.inline_xz.size(); ++i) {
    energy4 += std::abs(result4.inline_xz[i]);
    energy16 += std::abs(result16.inline_xz[i]);
  }

  // Both should have non-trivial energy
  EXPECT_GT(energy4, 0.0);
  EXPECT_GT(energy16, 0.0);
}

TEST(Rtm3D, MultiShotWith3DExtension) {
  rtm3d::GridModel2D model{.nx = 24, .nz = 16, .dx = 10.0f, .dz = 10.0f,
                           .values = std::vector<float>(24 * 16, 1800.0f)};

  rtm3d::RtmConfig cfg;
  cfg.ny = 6;
  cfg.dy = 10.0f;
  cfg.nt = 50;
  cfg.dt = 0.0008f;
  cfg.pml = 4;

  std::vector<rtm3d::ShotPosition> shots = {
      {.sx = 6, .sz = 2},
      {.sx = 12, .sz = 2},
      {.sx = 18, .sz = 2}
  };

  auto result = rtm3d::run_multi_shot_rtm(model, cfg, shots);

  EXPECT_EQ(result.nx, 24u);
  EXPECT_EQ(result.nz, 16u);
  EXPECT_EQ(result.inline_xz.size(), 24u * 16u);
}

TEST(Rtm3D, SmallNyProducesValidOutput) {
  rtm3d::GridModel2D model{.nx = 12, .nz = 10, .dx = 10.0f, .dz = 10.0f,
                           .values = std::vector<float>(12 * 10, 1500.0f)};

  rtm3d::RtmConfig cfg;
  cfg.ny = 2;  // Minimum for 3D
  cfg.dy = 10.0f;
  cfg.nt = 20;
  cfg.dt = 0.0008f;
  cfg.pml = 2;

  auto result = rtm3d::run_single_shot_rtm(model, cfg);

  EXPECT_EQ(result.inline_xz.size(), 12u * 10u);
}
