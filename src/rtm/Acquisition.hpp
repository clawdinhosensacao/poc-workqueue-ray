#pragma once

#include <cstddef>
#include <vector>

#include "rtm3d/core/Volume3D.hpp"

namespace rtm3d::rtm_internal {

struct ShotGeometry {
  std::size_t sx;
  std::size_t sy;
  std::size_t sz;
  std::vector<std::size_t> rx;
};

ShotGeometry make_default_shot_geometry(const Volume3D& vel, std::size_t receiver_stride);

}  // namespace rtm3d::rtm_internal
