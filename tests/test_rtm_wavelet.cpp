#include <algorithm>
#include <stdexcept>

#include <gtest/gtest.h>

#include "../src/rtm/Wavelet.hpp"

TEST(RtmWavelet, RejectsInvalidArguments) {
  EXPECT_THROW((void)rtm3d::rtm_internal::ricker_wavelet(1, 0.001f, 15.0f), std::runtime_error);
  EXPECT_THROW((void)rtm3d::rtm_internal::ricker_wavelet(100, 0.0f, 15.0f), std::runtime_error);
  EXPECT_THROW((void)rtm3d::rtm_internal::ricker_wavelet(100, 0.001f, 0.0f), std::runtime_error);
}

TEST(RtmWavelet, HasStrongPositivePeak) {
  const auto w = rtm3d::rtm_internal::ricker_wavelet(200, 0.001f, 15.0f);
  const float peak = *std::max_element(w.begin(), w.end());
  EXPECT_GT(peak, 0.9f);
}
