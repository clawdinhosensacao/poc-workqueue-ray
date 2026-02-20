#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "rtm3d/MarmousiLoader.hpp"
#include "rtm3d/Volume3D.hpp"

namespace rtm3d {

struct RtmConfig {
  std::size_t ny = 32;
  float dy = 20.0f;
  float dt = 0.0015f;
  std::size_t nt = 300;
  float f0 = 12.0f;
  std::size_t pml = 10;
  std::size_t receiver_stride = 8;
};

struct MigrationResult {
  std::size_t nx{};
  std::size_t nz{};
  std::vector<float> inline_xz;  // y-mid slice [nz][nx]
};

MigrationResult run_single_shot_rtm(const Marmousi2D& model, const RtmConfig& cfg);
std::vector<float> ricker_wavelet(std::size_t nt, float dt, float f0);

}  // namespace rtm3d
