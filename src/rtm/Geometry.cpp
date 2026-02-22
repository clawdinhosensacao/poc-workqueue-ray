#include "Geometry.hpp"

namespace rtm3d::rtm_internal {

Volume3D make_velocity_volume(const GridModel2D& model, const RtmConfig& cfg) {
  Volume3D vel(model.nx, cfg.ny, model.nz, 1500.0f);
  for (std::size_t iz = 0; iz < model.nz; ++iz) {
    for (std::size_t iy = 0; iy < cfg.ny; ++iy) {
      for (std::size_t ix = 0; ix < model.nx; ++ix) {
        vel(ix, iy, iz) = model.values[iz * model.nx + ix];
      }
    }
  }
  return vel;
}

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
