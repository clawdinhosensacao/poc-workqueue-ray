#include <cmath>

#include <gtest/gtest.h>

#include "rtm3d/io/GridModelLoader.hpp"
#include "rtm3d/rtm/RtmEngine.hpp"

TEST(RtmEngine, RickerWaveletHasStrongPeak) {
  const auto w = rtm3d::ricker_wavelet(200, 0.001f, 15.0f);
  float peak = -1e9f;
  for (float v : w) peak = std::max(peak, v);
  ASSERT_GT(peak, 0.9f);
}

TEST(RtmEngine, RunsSingleShotAndReturnsEnergy) {
  const auto model = rtm3d::load_grid_model_from_json_arrays("data/x.json", "data/z.json", "data/vel.json", {.decim_x = 25, .decim_z = 25, .crop_x = 32, .crop_z = 24});

  rtm3d::RtmConfig cfg;
  cfg.ny = 10;
  cfg.nt = 45;
  cfg.pml = 4;
  cfg.receiver_stride = 4;

  const auto out = rtm3d::run_single_shot_rtm(model, cfg);
  ASSERT_EQ(out.nx, model.nx);
  ASSERT_EQ(out.nz, model.nz);

  double l1 = 0.0;
  for (float v : out.inline_xz) l1 += std::abs(v);
  ASSERT_GT(l1, 0.0);
}

TEST(RtmEngine, RejectsInvalidParameters) {
  rtm3d::GridModel2D bad{.nx = 4, .nz = 4, .dx = 1.0f, .dz = 1.0f, .values = std::vector<float>(16, 1500.0f)};
  rtm3d::RtmConfig cfg;
  EXPECT_THROW((void)rtm3d::run_single_shot_rtm(bad, cfg), std::runtime_error);
}

TEST(RtmEngine, RunsMultiShotWithSingleShot) {
  rtm3d::GridModel2D model{.nx = 20, .nz = 16, .dx = 10.0f, .dz = 10.0f,
                           .values = std::vector<float>(20 * 16, 2000.0f)};

  rtm3d::RtmConfig cfg;
  cfg.ny = 8;
  cfg.nt = 50;
  cfg.dt = 0.001f;
  cfg.pml = 4;
  cfg.receiver_stride = 4;

  std::vector<rtm3d::ShotPosition> shots = {{.sx = 10, .sz = 2}};
  const auto out = rtm3d::run_multi_shot_rtm(model, cfg, shots);

  EXPECT_EQ(out.nx, 20u);
  EXPECT_EQ(out.nz, 16u);
  EXPECT_EQ(out.inline_xz.size(), 20u * 16u);

  // Should have non-zero energy
  double l1 = 0.0;
  for (float v : out.inline_xz) l1 += std::abs(v);
  EXPECT_GT(l1, 0.0);
}

TEST(RtmEngine, MultiShotStackingProducesValidOutput) {
  rtm3d::GridModel2D model{.nx = 24, .nz = 18, .dx = 10.0f, .dz = 10.0f,
                           .values = std::vector<float>(24 * 18, 1800.0f)};

  rtm3d::RtmConfig cfg;
  cfg.ny = 6;
  cfg.nt = 40;
  cfg.dt = 0.001f;
  cfg.pml = 4;
  cfg.receiver_stride = 4;

  std::vector<rtm3d::ShotPosition> shots = {
      {.sx = 6, .sz = 2},
      {.sx = 12, .sz = 2},
      {.sx = 18, .sz = 2}
  };

  const auto out = rtm3d::run_multi_shot_rtm(model, cfg, shots);

  EXPECT_EQ(out.nx, 24u);
  EXPECT_EQ(out.nz, 18u);

  // All values should be finite
  for (float v : out.inline_xz) {
    EXPECT_TRUE(std::isfinite(v));
  }
}

TEST(RtmEngine, DifferentNyProducesSameOutputSize) {
  rtm3d::GridModel2D model{.nx = 16, .nz = 12, .dx = 10.0f, .dz = 10.0f,
                           .values = std::vector<float>(16 * 12, 2000.0f)};

  rtm3d::RtmConfig cfg1;
  cfg1.ny = 4;
  cfg1.nt = 30;
  cfg1.dt = 0.001f;
  cfg1.pml = 4;
  cfg1.receiver_stride = 4;

  rtm3d::RtmConfig cfg2;
  cfg2.ny = 16;
  cfg2.nt = 30;
  cfg2.dt = 0.001f;
  cfg2.pml = 4;
  cfg2.receiver_stride = 4;

  const auto out1 = rtm3d::run_single_shot_rtm(model, cfg1);
  const auto out2 = rtm3d::run_single_shot_rtm(model, cfg2);

  // Output size should be independent of ny
  EXPECT_EQ(out1.nx, out2.nx);
  EXPECT_EQ(out1.nz, out2.nz);
  EXPECT_EQ(out1.inline_xz.size(), out2.inline_xz.size());
}

TEST(RtmEngine, RejectsEmptyShotsVector) {
  rtm3d::GridModel2D model{.nx = 20, .nz = 16, .dx = 10.0f, .dz = 10.0f,
                           .values = std::vector<float>(20 * 16, 2000.0f)};

  rtm3d::RtmConfig cfg;
  cfg.ny = 8;
  cfg.nt = 50;
  cfg.dt = 0.001f;
  cfg.pml = 4;

  std::vector<rtm3d::ShotPosition> empty_shots;
  EXPECT_THROW((void)rtm3d::run_multi_shot_rtm(model, cfg, empty_shots), std::runtime_error);
}
