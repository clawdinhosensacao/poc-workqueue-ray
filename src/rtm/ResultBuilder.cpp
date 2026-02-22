#include "ResultBuilder.hpp"

#include "Geometry.hpp"

namespace rtm3d::rtm_internal {

MigrationResult build_migration_result(const Volume3D& vel, const std::vector<float>& image) {
  MigrationResult out;
  out.nx = vel.nx();
  out.nz = vel.nz();
  out.inline_xz = extract_inline_xz(vel, image);
  return out;
}

}  // namespace rtm3d::rtm_internal
