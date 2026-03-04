#pragma once

#include <cstddef>
#include <vector>

namespace rtm3d::rtm_internal {

std::vector<float> make_damp(std::size_t nx, std::size_t ny, std::size_t nz, std::size_t pml);

}  // namespace rtm3d::rtm_internal
