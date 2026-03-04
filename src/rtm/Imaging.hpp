#pragma once

#include <vector>

namespace rtm3d::rtm_internal {

void accumulate_cross_correlation_image(const float* src, const std::vector<float>& rec_field,
                                        std::vector<float>& image);

}  // namespace rtm3d::rtm_internal
