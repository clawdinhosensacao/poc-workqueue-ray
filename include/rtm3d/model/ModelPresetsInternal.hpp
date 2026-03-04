#pragma once

#include <cstddef>
#include <vector>

#include "rtm3d/model/SeismicModel.hpp"

namespace rtm3d::model_internal {

void fill_preset_velocity(std::vector<float>& vp, ModelPreset preset,
                          const GridSpec& grid, float vp_top,
                          float vp_bottom, std::size_t nlayers);

}  // namespace rtm3d::model_internal
