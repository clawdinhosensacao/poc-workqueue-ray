#pragma once

#include <cstddef>
#include <vector>

#include "rtm3d/core/Volume3D.hpp"
#include "rtm3d/model/GridModel2D.hpp"
#include "rtm3d/rtm/RtmEngine.hpp"

namespace rtm3d::rtm_internal {

void forward_source_propagation(const GridModel2D& model, const RtmConfig& cfg, const Volume3D& vel,
                                const std::vector<float>& damp, const std::vector<float>& wavelet,
                                std::size_t sx, std::size_t sy, std::size_t sz,
                                const std::vector<std::size_t>& rx, std::vector<float>& src_snaps,
                                std::vector<float>& rec_data);

}  // namespace rtm3d::rtm_internal
