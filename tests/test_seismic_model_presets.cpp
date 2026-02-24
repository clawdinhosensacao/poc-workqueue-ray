#include <cmath>

#include <gtest/gtest.h>

#include "rtm3d/model/SeismicModel.hpp"

TEST(SeismicModelPreset, ConstantReturnsUniformVelocity) {
  rtm3d::GridSpec grid{.nx = 20, .nz = 15, .ny = 1, .dx = 10.0f, .dz = 10.0f};
  auto model = rtm3d::SeismicModel::from_preset(rtm3d::ModelPreset::Constant, grid, 2000.0f);

  EXPECT_EQ(model.velocity().size(), 20u * 15u);
  EXPECT_FLOAT_EQ(model.min_velocity(), 2000.0f);
  EXPECT_FLOAT_EQ(model.max_velocity(), 2000.0f);
}

TEST(SeismicModelPreset, LayersCreatesGradient) {
  rtm3d::GridSpec grid{.nx = 30, .nz = 30, .ny = 1, .dx = 10.0f, .dz = 10.0f};
  auto model = rtm3d::SeismicModel::from_preset(
      rtm3d::ModelPreset::Layers, grid, 1500.0f, 3500.0f, 3);

  EXPECT_LT(model.min_velocity(), model.max_velocity());
  EXPECT_GE(model.min_velocity(), 1500.0f);
  EXPECT_LE(model.max_velocity(), 3500.0f);
}

TEST(SeismicModelPreset, CircleHasAnomaly) {
  rtm3d::GridSpec grid{.nx = 50, .nz = 50, .ny = 1, .dx = 10.0f, .dz = 10.0f};
  auto model = rtm3d::SeismicModel::from_preset(
      rtm3d::ModelPreset::Circle, grid, 1500.0f, 2500.0f);

  // Background is 1500, anomaly is 2500
  EXPECT_FLOAT_EQ(model.min_velocity(), 1500.0f);
  EXPECT_FLOAT_EQ(model.max_velocity(), 2500.0f);
}

TEST(SeismicModelPreset, CircleLensHasSmoothTransition) {
  rtm3d::GridSpec grid{.nx = 40, .nz = 40, .ny = 1, .dx = 10.0f, .dz = 10.0f};
  auto model = rtm3d::SeismicModel::from_preset(
      rtm3d::ModelPreset::CircleLens, grid, 1800.0f, 2800.0f);

  // Gaussian lens creates smooth transition
  // Should have velocities between min and max
  EXPECT_GT(model.min_velocity(), 1500.0f);
  EXPECT_LT(model.max_velocity(), 3500.0f);
}

TEST(SeismicModelPreset, SaltDomeHasHighVelocityCore) {
  rtm3d::GridSpec grid{.nx = 80, .nz = 60, .ny = 1, .dx = 10.0f, .dz = 10.0f};
  auto model = rtm3d::SeismicModel::from_preset(
      rtm3d::ModelPreset::SaltDome, grid, 2000.0f, 3500.0f);

  // Salt dome should have high velocity (~4500 m/s) for the salt body
  // and background gradient from vp_top to vp_bottom
  EXPECT_GT(model.max_velocity(), 4000.0f);  // Salt velocity
  EXPECT_LT(model.min_velocity(), 2500.0f);  // Shallow sediments

  // Center column should have salt at depth
  const auto& vel = model.velocity();
  std::size_t center_x = grid.nx / 2;
  std::size_t deep_z = static_cast<std::size_t>(grid.nz * 0.5);

  std::size_t idx = deep_z * grid.nx + center_x;
  EXPECT_GT(vel[idx], 4000.0f);  // Salt body at center
}

TEST(SeismicModelPreset, FaultHasLayerOffset) {
  rtm3d::GridSpec grid{.nx = 80, .nz = 60, .ny = 1, .dx = 10.0f, .dz = 10.0f};
  auto model = rtm3d::SeismicModel::from_preset(
      rtm3d::ModelPreset::Fault, grid, 1500.0f, 3500.0f, 5);

  // Fault model should have velocity gradient
  EXPECT_GT(model.max_velocity(), model.min_velocity());

  // Check for layer offset across fault (discontinuity)
  const auto& vel = model.velocity();
  std::size_t mid_z = grid.nz / 2;
  std::size_t left_x = static_cast<std::size_t>(grid.nx * 0.3);
  std::size_t right_x = static_cast<std::size_t>(grid.nx * 0.7);

  // Velocities at same depth but different sides should differ due to fault offset
  float vel_left = vel[mid_z * grid.nx + left_x];
  float vel_right = vel[mid_z * grid.nx + right_x];
  EXPECT_NE(vel_left, vel_right);  // Fault creates offset
}

TEST(SeismicModelFromVelocity, AcceptsValidData) {
  rtm3d::GridSpec grid{.nx = 10, .nz = 8, .ny = 1};
  std::vector<float> vel(10 * 8, 2000.0f);

  auto model = rtm3d::SeismicModel::from_velocity(vel, grid);
  EXPECT_EQ(model.velocity().size(), 80u);
}

TEST(SeismicModelFromVelocity, RejectsSizeMismatch) {
  rtm3d::GridSpec grid{.nx = 10, .nz = 8, .ny = 1};
  std::vector<float> vel(50, 2000.0f);  // Wrong size

  EXPECT_THROW(rtm3d::SeismicModel::from_velocity(vel, grid), std::runtime_error);
}

TEST(SeismicModelValidation, PassesForStableConfig) {
  rtm3d::GridSpec grid{.nx = 20, .nz = 15, .ny = 1, .dx = 10.0f, .dz = 10.0f};
  auto model = rtm3d::SeismicModel::from_preset(rtm3d::ModelPreset::Constant, grid, 2000.0f);

  rtm3d::TimeAxis time{.start = 0.0f, .step = 0.0005f, .num = 100};

  EXPECT_NO_THROW(model.validate_for_rtm(time));
}

TEST(SeismicModelValidation, FailsForLargeDt) {
  rtm3d::GridSpec grid{.nx = 20, .nz = 15, .ny = 1, .dx = 10.0f, .dz = 10.0f};
  auto model = rtm3d::SeismicModel::from_preset(rtm3d::ModelPreset::Constant, grid, 4000.0f);

  rtm3d::TimeAxis time{.start = 0.0f, .step = 0.005f, .num = 100};  // Too large

  EXPECT_THROW(model.validate_for_rtm(time), std::runtime_error);
}
