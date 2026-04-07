#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "rtm3d/io/ArrayModelLoader.hpp"
#include "rtm3d/io/GridModelLoader.hpp"

TEST(ArrayModelLoader, Parse1DAnd2DJson) {
  const std::string d = "tests/tmp_loader";
  std::filesystem::create_directories(d);
  std::ofstream(d + "/x.json") << "[0, 25, 50, 75]";
  std::ofstream(d + "/z.json") << "[0, 10, 20]";
  std::ofstream(d + "/v.json") << "[[1500,1510,1520,1530],[1600,1610,1620,1630],[1700,1710,1720,1730]]";

  const auto x = rtm3d::load_array_1d_json(d + "/x.json");
  const auto v = rtm3d::load_array_2d_json(d + "/v.json");
  ASSERT_EQ(x.size(), 4u);
  ASSERT_EQ(v.size(), 3u);
  ASSERT_EQ(v[1][2], 1620);
}

TEST(GridModelLoader, DecimationAndCropWorks) {
  const auto model = rtm3d::load_grid_model_from_json_arrays("data/x.json", "data/z.json", "data/vel.json", {.decim_x = 20, .decim_z = 20, .crop_x = 30, .crop_z = 20});
  ASSERT_EQ(model.nx, 30u);
  ASSERT_GT(model.nz, 0u);
  ASSERT_LE(model.nz, 20u);
  ASSERT_GT(model.dx, 0.0f);
  ASSERT_GT(model.dz, 0.0f);
  ASSERT_EQ(model.values.size(), model.nx * model.nz);
}

TEST(GridModelLoader, RejectsZeroDecimation) {
  EXPECT_THROW((void)rtm3d::load_grid_model_from_json_arrays("data/x.json", "data/z.json", "data/vel.json", {.decim_x = 0, .decim_z = 1}), std::runtime_error);
}

TEST(ArrayModelLoader, RejectsMissingFile) {
  EXPECT_THROW((void)rtm3d::load_array_1d_json("nonexistent/file.json"), std::runtime_error);
}

TEST(ArrayModelLoader, RejectsInvalidJson) {
  const std::string d = "tests/tmp_loader";
  std::filesystem::create_directories(d);
  std::ofstream(d + "/invalid.json") << "not a json array";

  EXPECT_THROW((void)rtm3d::load_array_1d_json(d + "/invalid.json"), std::runtime_error);
}

TEST(ArrayModelLoader, RejectsEmptyArray) {
  const std::string d = "tests/tmp_loader";
  std::filesystem::create_directories(d);
  std::ofstream(d + "/empty.json") << "[]";

  // Empty array throws
  EXPECT_THROW((void)rtm3d::load_array_1d_json(d + "/empty.json"), std::runtime_error);
}

TEST(ArrayModelLoader, HandlesSingleElement) {
  const std::string d = "tests/tmp_loader";
  std::filesystem::create_directories(d);
  std::ofstream(d + "/single.json") << "[42.5]";

  const auto arr = rtm3d::load_array_1d_json(d + "/single.json");
  ASSERT_EQ(arr.size(), 1u);
  EXPECT_FLOAT_EQ(arr[0], 42.5f);
}

TEST(ArrayModelLoader, HandlesNegativeValues) {
  const std::string d = "tests/tmp_loader";
  std::filesystem::create_directories(d);
  std::ofstream(d + "/negative.json") << "[-10.5, 0.0, 25.3, -5.0]";

  const auto arr = rtm3d::load_array_1d_json(d + "/negative.json");
  ASSERT_EQ(arr.size(), 4u);
  EXPECT_FLOAT_EQ(arr[0], -10.5f);
  EXPECT_FLOAT_EQ(arr[3], -5.0f);
}

TEST(ArrayModelLoader, 2DHandlesSingleRow) {
  const std::string d = "tests/tmp_loader";
  std::filesystem::create_directories(d);
  std::ofstream(d + "/single_row.json") << "[[1, 2, 3, 4, 5]]";

  const auto arr = rtm3d::load_array_2d_json(d + "/single_row.json");
  ASSERT_EQ(arr.size(), 1u);
  ASSERT_EQ(arr[0].size(), 5u);
}

TEST(ArrayModelLoader, 2DHandlesSingleColumn) {
  const std::string d = "tests/tmp_loader";
  std::filesystem::create_directories(d);
  std::ofstream(d + "/single_col.json") << "[[1], [2], [3], [4]]";

  const auto arr = rtm3d::load_array_2d_json(d + "/single_col.json");
  ASSERT_EQ(arr.size(), 4u);
  for (const auto& row : arr) {
    ASSERT_EQ(row.size(), 1u);
  }
}
