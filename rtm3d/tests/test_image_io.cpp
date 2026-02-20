#include <filesystem>

#include <gtest/gtest.h>

#include "rtm3d/io/ImageIO.hpp"

TEST(ImageIO, WritesPGMFile) {
  std::filesystem::create_directories("output");
  std::vector<float> img(100, 0.0f);
  img[50] = 1.0f;
  rtm3d::write_pgm("output/test_inline.pgm", img, 10, 10);
  ASSERT_TRUE(std::filesystem::exists("output/test_inline.pgm"));
  ASSERT_GT(std::filesystem::file_size("output/test_inline.pgm"), 20);
}

TEST(ImageIO, RejectsShapeMismatch) {
  std::vector<float> img(9, 0.0f);
  EXPECT_THROW((void)rtm3d::write_pgm("output/bad.pgm", img, 4, 4), std::runtime_error);
}
