#include <cmath>

#include <gtest/gtest.h>

#include "rtm3d/model/SeismicModel.hpp"

TEST(SeismicModel, ConstantPresetCreatesUniformVelocity) {
  rtm3d::GridSpec grid{.nx = 50, .nz = 30, .ny = 1, .dx = 10.0f, .dz = 10.0f};
  auto model = rtm3d::SeismicModel::from_preset(rtm3d::ModelPreset::Constant, grid, 2000.0f);

  EXPECT_EQ(model.velocity().size(), 50u * 30u);
  EXPECT_FLOAT_EQ(model.min_velocity(), 2000.0f);
  EXPECT_FLOAT_EQ(model.max_velocity(), 2000.0f);
}

TEST(SeismicModel, LayersPresetCreatesGradient) {
  rtm3d::GridSpec grid{.nx = 50, .nz = 30, .ny = 1, .dx = 10.0f, .dz = 10.0f};
  auto model = rtm3d::SeismicModel::from_preset(
      rtm3d::ModelPreset::Layers, grid, 1500.0f, 3500.0f, 3);

  EXPECT_GE(model.min_velocity(), 1500.0f);
  EXPECT_LE(model.max_velocity(), 3500.0f);
  EXPECT_GT(model.max_velocity(), model.min_velocity());
}

TEST(SeismicModel, CirclePresetCreatesAnomaly) {
  rtm3d::GridSpec grid{.nx = 80, .nz = 80, .ny = 1, .dx = 10.0f, .dz = 10.0f};
  auto model = rtm3d::SeismicModel::from_preset(
      rtm3d::ModelPreset::Circle, grid, 1500.0f, 2500.0f);

  // Background is 1500, anomaly is 2500
  EXPECT_FLOAT_EQ(model.min_velocity(), 1500.0f);
  EXPECT_FLOAT_EQ(model.max_velocity(), 2500.0f);
}

TEST(SeismicModel, FromVelocityCopiesData) {
  std::vector<float> vel = {1000.0f, 2000.0f, 3000.0f, 4000.0f};
  rtm3d::GridSpec grid{.nx = 2, .nz = 2, .ny = 1};

  auto model = rtm3d::SeismicModel::from_velocity(vel, grid);

  EXPECT_EQ(model.velocity().size(), 4u);
  EXPECT_FLOAT_EQ(model.velocity()[0], 1000.0f);
  EXPECT_FLOAT_EQ(model.velocity()[3], 4000.0f);
}

TEST(SeismicModel, FromVelocityRejectsSizeMismatch) {
  std::vector<float> vel = {1000.0f, 2000.0f, 3000.0f};  // Wrong size
  rtm3d::GridSpec grid{.nx = 2, .nz = 2, .ny = 1};

  EXPECT_THROW(rtm3d::SeismicModel::from_velocity(vel, grid), std::runtime_error);
}

TEST(SeismicModel, ValidateForRtmChecksCFL) {
  rtm3d::GridSpec grid{.nx = 50, .nz = 30, .ny = 1, .dx = 10.0f, .dz = 10.0f};
  auto model = rtm3d::SeismicModel::from_preset(rtm3d::ModelPreset::Constant, grid, 2000.0f);

  // Valid time step
  rtm3d::TimeAxis valid_time{.step = 0.001f, .num = 100};
  EXPECT_NO_THROW(model.validate_for_rtm(valid_time));

  // Invalid time step (too large)
  rtm3d::TimeAxis invalid_time{.step = 0.01f, .num = 100};
  EXPECT_THROW(model.validate_for_rtm(invalid_time), std::runtime_error);
}

TEST(SeismicModel, GridSpecHasCorrectDefaults) {
  rtm3d::GridSpec grid;

  EXPECT_EQ(grid.nx, 101u);
  EXPECT_EQ(grid.nz, 101u);
  EXPECT_EQ(grid.ny, 1u);
  EXPECT_FLOAT_EQ(grid.dx, 10.0f);
  EXPECT_FLOAT_EQ(grid.dz, 10.0f);
  EXPECT_EQ(grid.nbl, 10u);
}

TEST(SeismicModel, TimeAxisStopCalculation) {
  rtm3d::TimeAxis time{.start = 0.0f, .step = 0.001f, .num = 100};

  EXPECT_FLOAT_EQ(time.stop(), 0.099f);
}
