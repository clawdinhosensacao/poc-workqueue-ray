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

}  // namespace rtm3d::rtm_internal
