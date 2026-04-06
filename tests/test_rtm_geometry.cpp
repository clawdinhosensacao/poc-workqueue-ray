#include <gtest/gtest.h>

#include "rtm3d/model/GridModel2D.hpp"
#include "rtm3d/rtm/RtmEngine.hpp"
#include "../src/rtm/Geometry.hpp"

TEST(RtmGeometry, Builds3DVelocityVolumeFrom2DModel) {
  rtm3d::GridModel2D model;
  model.nx = 3;
  model.nz = 2;
  model.dx = 10.0f;
  model.dz = 10.0f;
  model.values = {1500.0f, 1600.0f, 1700.0f,
                  1800.0f, 1900.0f, 2000.0f};

  rtm3d::RtmConfig cfg;
  cfg.ny = 4;

  const auto vel = rtm3d::rtm_internal::make_velocity_volume(model, cfg);

  EXPECT_EQ(vel.nx(), 3u);
  EXPECT_EQ(vel.ny(), 4u);
  EXPECT_EQ(vel.nz(), 2u);

  for (std::size_t y = 0; y < vel.ny(); ++y) {
    EXPECT_FLOAT_EQ(vel(0, y, 0), 1500.0f);
    EXPECT_FLOAT_EQ(vel(2, y, 0), 1700.0f);
    EXPECT_FLOAT_EQ(vel(1, y, 1), 1900.0f);
  }
}

TEST(RtmGeometry, PreservesAllVelocityValuesInYExtension) {
  rtm3d::GridModel2D model;
  model.nx = 4;
  model.nz = 3;
  model.dx = 10.0f;
  model.dz = 10.0f;
  // Unique values to verify correct placement
  model.values = {
    100.0f, 101.0f, 102.0f, 103.0f,  // z=0
    200.0f, 201.0f, 202.0f, 203.0f,  // z=1
    300.0f, 301.0f, 302.0f, 303.0f   // z=2
  };

  rtm3d::RtmConfig cfg;
  cfg.ny = 5;

  const auto vel = rtm3d::rtm_internal::make_velocity_volume(model, cfg);

  // Verify all Y slices have identical velocity values
  for (std::size_t y = 0; y < cfg.ny; ++y) {
    for (std::size_t z = 0; z < model.nz; ++z) {
      for (std::size_t x = 0; x < model.nx; ++x) {
        const float expected = model.values[z * model.nx + x];
        EXPECT_FLOAT_EQ(vel(x, y, z), expected)
            << "Mismatch at (x=" << x << ", y=" << y << ", z=" << z << ")";
      }
    }
  }
}

TEST(RtmGeometry, WorksForSingleYSlice) {
  rtm3d::GridModel2D model;
  model.nx = 5;
  model.nz = 4;
  model.dx = 10.0f;
  model.dz = 10.0f;
  model.values = std::vector<float>(20, 2000.0f);

  rtm3d::RtmConfig cfg;
  cfg.ny = 1;  // Single Y slice

  const auto vel = rtm3d::rtm_internal::make_velocity_volume(model, cfg);

  EXPECT_EQ(vel.nx(), 5u);
  EXPECT_EQ(vel.ny(), 1u);
  EXPECT_EQ(vel.nz(), 4u);
  EXPECT_EQ(vel.size(), 20u);
}

TEST(RtmGeometry, LargerGridPreservesCorrectLayout) {
  rtm3d::GridModel2D model;
  model.nx = 10;
  model.nz = 8;
  model.dx = 5.0f;
  model.dz = 5.0f;
  model.values.resize(80);
  for (std::size_t i = 0; i < 80; ++i) {
    model.values[i] = static_cast<float>(i);
  }

  rtm3d::RtmConfig cfg;
  cfg.ny = 3;

  const auto vel = rtm3d::rtm_internal::make_velocity_volume(model, cfg);

  EXPECT_EQ(vel.size(), 10u * 3u * 8u);

  // Check that Y-slice at y=1 has same values as y=0 and y=2
  for (std::size_t z = 0; z < model.nz; ++z) {
    for (std::size_t x = 0; x < model.nx; ++x) {
      EXPECT_FLOAT_EQ(vel(x, 0, z), vel(x, 1, z));
      EXPECT_FLOAT_EQ(vel(x, 1, z), vel(x, 2, z));
    }
  }
}
