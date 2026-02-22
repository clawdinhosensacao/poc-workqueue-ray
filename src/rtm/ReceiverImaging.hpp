#pragma once

#include <cstddef>
#include <vector>

#include "rtm3d/core/Volume3D.hpp"
#include "rtm3d/model/GridModel2D.hpp"
#include "rtm3d/rtm/RtmEngine.hpp"

namespace rtm3d::rtm_internal {

void receiver_backpropagation_and_imaging(const GridModel2D& model, const RtmConfig& cfg,
                                          const Volume3D& vel, const std::vector<float>& damp,
                                          std::size_t sy, std::size_t sz,
                                          const std::vector<std::size_t>& rx,
                                          const std::vector<float>& src_snaps,
                                          const std::vector<float>& rec_data,
                                          std::vector<float>& image);

}  // namespace rtm3d::rtm_internal
