#include <fstream>

#include <gtest/gtest.h>

#include "rtm3d/io/ArrayModelLoader.hpp"

TEST(ArrayModelLoaderEdge, RejectsMalformed1DJson) {
  const std::string p = "tests/tmp_loader/bad1d.json";
  std::ofstream(p) << "not-json";
  EXPECT_THROW((void)rtm3d::load_array_1d_json(p), std::runtime_error);
}

TEST(ArrayModelLoaderEdge, RejectsEmpty2DJson) {
  const std::string p = "tests/tmp_loader/empty2d.json";
  std::ofstream(p) << "[]";
  EXPECT_THROW((void)rtm3d::load_array_2d_json(p), std::runtime_error);
}
