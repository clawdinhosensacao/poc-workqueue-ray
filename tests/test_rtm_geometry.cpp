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
