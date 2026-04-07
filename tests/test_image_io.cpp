#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "rtm3d/io/ImageIO.hpp"

TEST(ImageIO, WritesPGMFile) {
  std::filesystem::create_directories("output");
  std::vector<float> img(100, 0.0f);
  img[50] = 1.0f;
  rtm3d::write_pgm("output/test_inline.pgm", img, 10, 10);
  ASSERT_TRUE(std::filesystem::exists("output/test_inline.pgm"));
  ASSERT_GT(std::filesystem::file_size("output/test_inline.pgm"), 20u);
}

TEST(ImageIO, RejectsShapeMismatch) {
  std::vector<float> img(9, 0.0f);
  EXPECT_THROW((void)rtm3d::write_pgm("output/bad.pgm", img, 4, 4), std::runtime_error);
}

TEST(ImageIO, WritesFloat32RawWithHeader) {
  std::vector<float> img(12, 0.25f);
  rtm3d::write_float32_raw("output/test_inline.bin", img, 4, 3);
  ASSERT_TRUE(std::filesystem::exists("output/test_inline.bin"));
  ASSERT_TRUE(std::filesystem::exists("output/test_inline.bin.json"));
  ASSERT_EQ(std::filesystem::file_size("output/test_inline.bin"), img.size() * sizeof(float));
}

TEST(ImageIO, WritesPGMWithCorrectHeader) {
  std::filesystem::create_directories("output");
  std::vector<float> img(6, 0.5f);
  rtm3d::write_pgm("output/test_header.pgm", img, 3, 2);
  
  // Read and check PGM header format
  std::ifstream file("output/test_header.pgm");
  std::string magic;
  int width, height, maxval;
  file >> magic >> width >> height >> maxval;
  
  EXPECT_EQ(magic, "P5");
  EXPECT_EQ(width, 3);
  EXPECT_EQ(height, 2);
  EXPECT_EQ(maxval, 255);
}

TEST(ImageIO, WritesSinglePixelPGM) {
  std::filesystem::create_directories("output");
  std::vector<float> img(1, 0.5f);
  rtm3d::write_pgm("output/single_pixel.pgm", img, 1, 1);
  ASSERT_TRUE(std::filesystem::exists("output/single_pixel.pgm"));
  EXPECT_GT(std::filesystem::file_size("output/single_pixel.pgm"), 8u);  // Header + 1 byte
}

TEST(ImageIO, HandlesZeroAndOneValues) {
  std::filesystem::create_directories("output");
  std::vector<float> img = {0.0f, 1.0f, 0.5f, 0.25f};
  rtm3d::write_pgm("output/zero_one.pgm", img, 2, 2);
  ASSERT_TRUE(std::filesystem::exists("output/zero_one.pgm"));
}

TEST(ImageIO, RejectsEmptyImage) {
  std::vector<float> img;
  EXPECT_THROW((void)rtm3d::write_pgm("output/empty.pgm", img, 0, 0), std::runtime_error);
}

TEST(ImageIO, RejectsZeroWidth) {
  std::vector<float> img(10, 0.5f);
  EXPECT_THROW((void)rtm3d::write_pgm("output/zero_width.pgm", img, 0, 10), std::runtime_error);
}

TEST(ImageIO, RejectsZeroHeight) {
  std::vector<float> img(10, 0.5f);
  EXPECT_THROW((void)rtm3d::write_pgm("output/zero_height.pgm", img, 10, 0), std::runtime_error);
}

TEST(ImageIO, Float32RawCreatesJSONMetadata) {
  std::filesystem::create_directories("output");
  std::vector<float> img(24, 1.0f);
  rtm3d::write_float32_raw("output/metadata.bin", img, 6, 4);
  
  // Check JSON file exists and is non-empty
  ASSERT_TRUE(std::filesystem::exists("output/metadata.bin.json"));
  EXPECT_GT(std::filesystem::file_size("output/metadata.bin.json"), 0u);
}
