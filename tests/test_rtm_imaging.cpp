#include <gtest/gtest.h>

#include "../src/rtm/Imaging.hpp"

TEST(RtmImaging, AccumulatesCrossCorrelation) {
  const std::size_t n = 6;
  std::vector<float> src = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
  std::vector<float> rec_field = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
  std::vector<float> image(n, 0.0f);

  rtm3d::rtm_internal::accumulate_cross_correlation_image(src.data(), rec_field, image);

  // Cross-correlation: image[i] += src[i] * rec_field[i]
  EXPECT_FLOAT_EQ(image[0], 0.5f);   // 1 * 0.5
  EXPECT_FLOAT_EQ(image[1], 1.0f);   // 2 * 0.5
  EXPECT_FLOAT_EQ(image[2], 1.5f);   // 3 * 0.5
  EXPECT_FLOAT_EQ(image[3], 2.0f);   // 4 * 0.5
  EXPECT_FLOAT_EQ(image[4], 2.5f);   // 5 * 0.5
  EXPECT_FLOAT_EQ(image[5], 3.0f);   // 6 * 0.5
}

TEST(RtmImaging, AccumulatesToExistingImage) {
  const std::size_t n = 4;
  std::vector<float> src = {1.0f, 1.0f, 1.0f, 1.0f};
  std::vector<float> rec_field = {2.0f, 2.0f, 2.0f, 2.0f};
  std::vector<float> image = {10.0f, 10.0f, 10.0f, 10.0f};  // Pre-existing image

  rtm3d::rtm_internal::accumulate_cross_correlation_image(src.data(), rec_field, image);

  // Should accumulate: 10 + 1*2 = 12
  for (std::size_t i = 0; i < n; ++i) {
    EXPECT_FLOAT_EQ(image[i], 12.0f);
  }
}

TEST(RtmImaging, HandlesNegativeValues) {
  const std::size_t n = 3;
  std::vector<float> src = {-1.0f, 0.0f, 1.0f};
  std::vector<float> rec_field = {1.0f, 1.0f, -1.0f};
  std::vector<float> image(n, 0.0f);

  rtm3d::rtm_internal::accumulate_cross_correlation_image(src.data(), rec_field, image);

  EXPECT_FLOAT_EQ(image[0], -1.0f);   // -1 * 1
  EXPECT_FLOAT_EQ(image[1], 0.0f);    // 0 * 1
  EXPECT_FLOAT_EQ(image[2], -1.0f);   // 1 * -1
}

TEST(RtmImaging, MultipleAccumulationsBuildUp) {
  const std::size_t n = 4;
  std::vector<float> src = {1.0f, 1.0f, 1.0f, 1.0f};
  std::vector<float> rec_field = {1.0f, 1.0f, 1.0f, 1.0f};
  std::vector<float> image(n, 0.0f);

  // Accumulate twice
  rtm3d::rtm_internal::accumulate_cross_correlation_image(src.data(), rec_field, image);
  rtm3d::rtm_internal::accumulate_cross_correlation_image(src.data(), rec_field, image);

  // Should be 2.0 each (accumulated twice)
  for (std::size_t i = 0; i < n; ++i) {
    EXPECT_FLOAT_EQ(image[i], 2.0f);
  }
}
