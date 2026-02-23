#include "InlineSlice.hpp"

namespace rtm3d::rtm_internal {

std::vector<float> extract_inline_xz(const Volume3D& vol, const std::vector<float>& image) {
  std::vector<float> slice(vol.nx() * vol.nz(), 0.0f);
  const std::size_t ymid = vol.ny() / 2;
  for (std::size_t iz = 0; iz < vol.nz(); ++iz) {
    for (std::size_t ix = 0; ix < vol.nx(); ++ix) {
      slice[iz * vol.nx() + ix] = image[vol.index(ix, ymid, iz)];
    }
  }
  return slice;
}

std::vector<float> extract_crossline_yz(const Volume3D& vol, const std::vector<float>& image) {
  std::vector<float> slice(vol.ny() * vol.nz(), 0.0f);
  const std::size_t xmid = vol.nx() / 2;
  for (std::size_t iz = 0; iz < vol.nz(); ++iz) {
    for (std::size_t iy = 0; iy < vol.ny(); ++iy) {
      slice[iz * vol.ny() + iy] = image[vol.index(xmid, iy, iz)];
    }
  }
  return slice;
}

std::vector<float> extract_depth_xy(const Volume3D& vol, const std::vector<float>& image,
                                     std::size_t iz) {
  std::vector<float> slice(vol.nx() * vol.ny(), 0.0f);
  iz = std::min(iz, vol.nz() - 1);
  for (std::size_t iy = 0; iy < vol.ny(); ++iy) {
    for (std::size_t ix = 0; ix < vol.nx(); ++ix) {
      slice[iy * vol.nx() + ix] = image[vol.index(ix, iy, iz)];
    }
  }
  return slice;
}

}  // namespace rtm3d::rtm_internal
