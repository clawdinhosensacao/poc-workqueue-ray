#pragma once

#include <vector>

#include "rtm3d/core/Volume3D.hpp"

namespace rtm3d::rtm_internal {

std::vector<float> extract_inline_xz(const Volume3D& vel, const std::vector<float>& image);

}  // namespace rtm3d::rtm_internal
