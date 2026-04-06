#include <algorithm>
#include <cmath>
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

TEST(RtmWavelet, OutputSizeMatchesNt) {
  const std::size_t nt = 150;
  const auto w = rtm3d::rtm_internal::ricker_wavelet(nt, 0.001f, 20.0f);
  EXPECT_EQ(w.size(), nt);
}

TEST(RtmWavelet, HasNegativeSideLobes) {
  const auto w = rtm3d::rtm_internal::ricker_wavelet(200, 0.001f, 15.0f);

  // Ricker wavelet should have negative side lobes
  const float min_val = *std::min_element(w.begin(), w.end());
  EXPECT_LT(min_val, -0.1f);
}

TEST(RtmWavelet, PeakOccursNearCenter) {
  const auto w = rtm3d::rtm_internal::ricker_wavelet(200, 0.001f, 15.0f);

  // Find peak location
  const auto peak_it = std::max_element(w.begin(), w.end());
  const std::size_t peak_idx = static_cast<std::size_t>(peak_it - w.begin());

  // Peak should be roughly at t0 = 1/f0 = ~67 samples for f0=15 Hz, dt=0.001s
  // Allow some tolerance
  EXPECT_GT(peak_idx, 40u);
  EXPECT_LT(peak_idx, 100u);
}

TEST(RtmWavelet, DifferentFrequenciesProduceDifferentWavelets) {
  const auto w1 = rtm3d::rtm_internal::ricker_wavelet(200, 0.001f, 10.0f);
  const auto w2 = rtm3d::rtm_internal::ricker_wavelet(200, 0.001f, 25.0f);

  // Different peak frequencies should produce different wavelets
  // Higher frequency should have narrower main lobe
  const auto peak1 = std::max_element(w1.begin(), w1.end());
  const auto peak2 = std::max_element(w2.begin(), w2.end());

  // Find approximate width at half max for each
  const float half_max1 = *peak1 * 0.5f;
  const float half_max2 = *peak2 * 0.5f;

  std::size_t width1 = 0, width2 = 0;
  for (float v : w1) { if (v > half_max1) ++width1; }
  for (float v : w2) { if (v > half_max2) ++width2; }

  // Higher frequency should have narrower width
  EXPECT_LT(width2, width1);
}
