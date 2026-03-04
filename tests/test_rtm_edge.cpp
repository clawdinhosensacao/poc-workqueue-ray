#include <gtest/gtest.h>

#include "rtm3d/rtm/RtmEngine.hpp"

TEST(RtmEdge, RejectsInvalidWaveletArgs) {
  EXPECT_THROW((void)rtm3d::ricker_wavelet(1, 0.001f, 10.0f), std::runtime_error);
  EXPECT_THROW((void)rtm3d::ricker_wavelet(32, -0.001f, 10.0f), std::runtime_error);
  EXPECT_THROW((void)rtm3d::ricker_wavelet(32, 0.001f, 0.0f), std::runtime_error);
}
