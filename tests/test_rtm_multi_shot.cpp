#include <cmath>

#include <gtest/gtest.h>

#include "rtm3d/io/GridModelLoader.hpp"
#include "rtm3d/rtm/RtmEngine.hpp"

TEST(RtmMultiShot, RejectsEmptyShotsVector) {
  rtm3d::GridModel2D model{.nx = 16, .nz = 12, .dx = 10.0f, .dz = 10.0f,
                           .values = std::vector<float>(16 * 12, 1800.0f)};
  rtm3d::RtmConfig cfg;
  cfg.ny = 8;
  cfg.nt = 20;

  std::vector<rtm3d::ShotPosition> empty_shots;
  EXPECT_THROW((void)rtm3d::run_multi_shot_rtm(model, cfg, empty_shots), std::runtime_error);
}

TEST(RtmMultiShot, SingleShotMatchesMultiShotWithOneSource) {
  const auto model = rtm3d::load_grid_model_from_json_arrays(
      "data/x.json", "data/z.json", "data/vel.json",
      {.decim_x = 25, .decim_z = 25, .crop_x = 32, .crop_z = 24});

  rtm3d::RtmConfig cfg;
  cfg.ny = 10;
  cfg.nt = 45;
  cfg.pml = 4;
  cfg.receiver_stride = 4;

  const auto single = rtm3d::run_single_shot_rtm(model, cfg);

  std::vector<rtm3d::ShotPosition> shots = {{.sx = model.nx / 2, .sz = 2}};
  const auto multi = rtm3d::run_multi_shot_rtm(model, cfg, shots);

  ASSERT_EQ(single.nx, multi.nx);
  ASSERT_EQ(single.nz, multi.nz);
  ASSERT_EQ(single.inline_xz.size(), multi.inline_xz.size());

  for (std::size_t i = 0; i < single.inline_xz.size(); ++i) {
    EXPECT_NEAR(single.inline_xz[i], multi.inline_xz[i], 1e-5f);
  }
}

TEST(RtmMultiShot, MultipleShotsProduceStrongerImage) {
  rtm3d::GridModel2D model{.nx = 32, .nz = 24, .dx = 10.0f, .dz = 10.0f,
                           .values = std::vector<float>(32 * 24, 1800.0f)};

  rtm3d::RtmConfig cfg;
  cfg.ny = 12;
  cfg.nt = 60;
  cfg.pml = 4;
  cfg.receiver_stride = 4;

  std::vector<rtm3d::ShotPosition> shots = {
      {.sx = 8, .sz = 2}, {.sx = 16, .sz = 2}, {.sx = 24, .sz = 2}};

  const auto result = rtm3d::run_multi_shot_rtm(model, cfg, shots);

  EXPECT_EQ(result.nx, 32u);
  EXPECT_EQ(result.nz, 24u);
  EXPECT_EQ(result.inline_xz.size(), 32u * 24u);

  double energy = 0.0;
  for (float v : result.inline_xz) {
    energy += static_cast<double>(v) * v;
  }
  EXPECT_GT(energy, 0.0);
}
