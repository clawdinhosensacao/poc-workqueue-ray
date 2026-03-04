#pragma once

#include <cstddef>
#include <vector>

namespace rtm3d::rtm_internal {

std::vector<float> ricker_wavelet(std::size_t nt, float dt, float f0);

}  // namespace rtm3d::rtm_internal
