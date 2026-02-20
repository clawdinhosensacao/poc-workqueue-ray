#include <fstream>

#include <gtest/gtest.h>

#include "rtm3d/io/ArrayModelLoader.hpp"
#include "rtm3d/io/GridModelLoader.hpp"

TEST(ArrayModelLoader, Parse1DAnd2DJson) {
  const std::string d = "tests/tmp_loader";
  std::system(("mkdir -p " + d).c_str());
  std::ofstream(d + "/x.json") << "[0, 25, 50, 75]";
  std::ofstream(d + "/z.json") << "[0, 10, 20]";
  std::ofstream(d + "/v.json") << "[[1500,1510,1520,1530],[1600,1610,1620,1630],[1700,1710,1720,1730]]";

  const auto x = rtm3d::load_array_1d_json(d + "/x.json");
  const auto v = rtm3d::load_array_2d_json(d + "/v.json");
  ASSERT_EQ(x.size(), 4);
  ASSERT_EQ(v.size(), 3);
  ASSERT_EQ(v[1][2], 1620);
}

TEST(GridModelLoader, DecimationAndCropWorks) {
  const auto model = rtm3d::load_grid_model_from_json_arrays("data/x.json", "data/z.json", "data/vel.json", {.decim_x = 20, .decim_z = 20, .crop_x = 30, .crop_z = 20});
  ASSERT_EQ(model.nx, 30);
  ASSERT_GT(model.nz, 0u);
  ASSERT_LE(model.nz, 20u);
  ASSERT_GT(model.dx, 0.0f);
  ASSERT_GT(model.dz, 0.0f);
  ASSERT_EQ(model.values.size(), model.nx * model.nz);
}

TEST(GridModelLoader, RejectsZeroDecimation) {
  EXPECT_THROW((void)rtm3d::load_grid_model_from_json_arrays("data/x.json", "data/z.json", "data/vel.json", {.decim_x = 0, .decim_z = 1}), std::runtime_error);
}
