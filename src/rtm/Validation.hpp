#pragma once

#include "rtm3d/model/GridModel2D.hpp"
#include "rtm3d/rtm/RtmEngine.hpp"

namespace rtm3d::rtm_internal {

void validate_cfg(const GridModel2D& model, const RtmConfig& cfg);

}  // namespace rtm3d::rtm_internal
