#include "InlineSlice.hpp"

namespace rtm3d::rtm_internal {

std::vector<float> extract_inline_xz(const Volume3D& vel, const std::vector<float>& image) {
  std::vector<float> inline_xz(vel.nx() * vel.nz(), 0.0f);
  const std::size_t ymid = vel.ny() / 2;
  for (std::size_t iz = 0; iz < vel.nz(); ++iz) {
    for (std::size_t ix = 0; ix < vel.nx(); ++ix) {
      inline_xz[iz * vel.nx() + ix] = image[vel.index(ix, ymid, iz)];
    }
  }
  return inline_xz;
}

}  // namespace rtm3d::rtm_internal
