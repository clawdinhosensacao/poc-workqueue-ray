#pragma once

#include <cstddef>
#include <vector>

#include "rtm3d/model/GridModel2D.hpp"

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
  std::vector<float> inline_xz;
};

std::vector<float> ricker_wavelet(std::size_t nt, float dt, float f0);
MigrationResult run_single_shot_rtm(const GridModel2D& model, const RtmConfig& cfg);

}  // namespace rtm3d
