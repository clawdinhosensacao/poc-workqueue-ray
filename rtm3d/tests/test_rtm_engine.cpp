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
