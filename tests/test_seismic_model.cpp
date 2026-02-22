#include <cmath>
#include <stdexcept>

#include <gtest/gtest.h>

#include "rtm3d/model/SeismicModel.hpp"

TEST(SeismicModel, ConstantPresetCreatesUniformVelocity) {
  rtm3d::GridSpec grid{.nx = 50, .nz = 40, .ny = 1, .dx = 10.0f, .dz = 10.0f};
  auto model = rtm3d::SeismicModel::from_preset(rtm3d::ModelPreset::Constant, grid, 2000.0f);

  EXPECT_EQ(model.velocity().size(), 50u * 40u);
  EXPECT_FLOAT_EQ(model.min_velocity(), 2000.0f);
  EXPECT_FLOAT_EQ(model.max_velocity(), 2000.0f);
}

TEST(SeismicModel, LayersPresetCreatesVelocityGradient) {
  rtm3d::GridSpec grid{.nx = 50, .nz = 60, .ny = 1, .dx = 10.0f, .dz = 10.0f};
  auto model = rtm3d::SeismicModel::from_preset(
      rtm3d::ModelPreset::Layers, grid, 1500.0f, 3500.0f, 3);

  EXPECT_GE(model.max_velocity(), 1500.0f);
  EXPECT_LE(model.max_velocity(), 3500.0f);
  EXPECT_GE(model.min_velocity(), 1500.0f);
}

TEST(SeismicModel, CirclePresetCreatesCircularAnomaly) {
  rtm3d::GridSpec grid{.nx = 60, .nz = 60, .ny = 1, .dx = 10.0f, .dz = 10.0f};
  auto model = rtm3d::SeismicModel::from_preset(
      rtm3d::ModelPreset::Circle, grid, 1500.0f, 2500.0f);

  // Center should have higher velocity
  std::size_t center_idx = 30 * 60 + 30;  // (z=30, x=30)
  EXPECT_NEAR(model.velocity()[center_idx], 2500.0f, 1.0f);

  // Corner should have background velocity
  std::size_t corner_idx = 0;  // (z=0, x=0)
  EXPECT_NEAR(model.velocity()[corner_idx], 1500.0f, 1.0f);
}

TEST(SeismicModel, CircleLensPresetCreatesGaussianAnomaly) {
  rtm3d::GridSpec grid{.nx = 80, .nz = 60, .ny = 1, .dx = 10.0f, .dz = 10.0f};
  auto model = rtm3d::SeismicModel::from_preset(
      rtm3d::ModelPreset::CircleLens, grid, 1800.0f, 2200.0f);

  EXPECT_GT(model.max_velocity(), 1800.0f);
  EXPECT_GE(model.min_velocity(), 1800.0f);
}

TEST(SeismicModel, ValidatesCFLCondition) {
  rtm3d::GridSpec grid{.nx = 50, .nz = 50, .ny = 1, .dx = 10.0f, .dz = 10.0f};
  auto model = rtm3d::SeismicModel::from_preset(rtm3d::ModelPreset::Constant, grid, 2000.0f);

  // Valid time step
  rtm3d::TimeAxis valid_time{.step = 0.001f, .num = 100};
  EXPECT_NO_THROW(model.validate_for_rtm(valid_time));

  // Invalid time step (too large)
  rtm3d::TimeAxis invalid_time{.step = 0.005f, .num = 100};
  EXPECT_THROW(model.validate_for_rtm(invalid_time), std::runtime_error);
}

TEST(SeismicModel, FromVelocityArray) {
  rtm3d::GridSpec grid{.nx = 10, .nz = 10, .ny = 1, .dx = 10.0f, .dz = 10.0f};
  std::vector<float> vp(100, 2500.0f);

  auto model = rtm3d::SeismicModel::from_velocity(vp, grid);
  EXPECT_EQ(model.velocity().size(), 100u);
  EXPECT_FLOAT_EQ(model.max_velocity(), 2500.0f);
}

TEST(SeismicModel, RejectsMismatchedVelocitySize) {
  rtm3d::GridSpec grid{.nx = 10, .nz = 10, .ny = 1};
  std::vector<float> vp(50, 2000.0f);  // Wrong size

  EXPECT_THROW(rtm3d::SeismicModel::from_velocity(vp, grid), std::runtime_error);
}

TEST(TimeAxis, ComputesStopCorrectly) {
  rtm3d::TimeAxis time{.start = 0.0f, .step = 0.001f, .num = 100};
  EXPECT_FLOAT_EQ(time.stop(), 0.099f);
}
