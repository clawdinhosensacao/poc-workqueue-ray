#pragma once

#include <cstddef>
#include <vector>

#include "rtm3d/core/Volume3D.hpp"
#include "rtm3d/model/GridModel2D.hpp"
#include "rtm3d/rtm/RtmEngine.hpp"

namespace rtm3d::rtm_internal {

Volume3D make_velocity_volume(const GridModel2D& model, const RtmConfig& cfg);
std::vector<float> extract_inline_xz(const Volume3D& vel, const std::vector<float>& image);

}  // namespace rtm3d::rtm_internal
