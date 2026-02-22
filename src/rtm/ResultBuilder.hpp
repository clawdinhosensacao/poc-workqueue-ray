#pragma once

#include <vector>

#include "rtm3d/core/Volume3D.hpp"
#include "rtm3d/rtm/RtmEngine.hpp"

namespace rtm3d::rtm_internal {

MigrationResult build_migration_result(const Volume3D& vel, const std::vector<float>& image);

}  // namespace rtm3d::rtm_internal
