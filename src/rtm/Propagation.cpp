#include "Propagation.hpp"

#include <algorithm>

namespace rtm3d::rtm_internal {

void step_fd3d(const Volume3D& vel, const std::vector<float>& damp, float dt, float dx, float dy,
               float dz, const std::vector<float>& prev, const std::vector<float>& cur,
               std::vector<float>& nxt) {
  std::fill(nxt.begin(), nxt.end(), 0.0f);

  for (std::size_t iz = 1; iz + 1 < vel.nz(); ++iz) {
    for (std::size_t iy = 1; iy + 1 < vel.ny(); ++iy) {
      for (std::size_t ix = 1; ix + 1 < vel.nx(); ++ix) {
        const auto i = vel.index(ix, iy, iz);
        const float d2x = (cur[vel.index(ix + 1, iy, iz)] - 2.0f * cur[i] + cur[vel.index(ix - 1, iy, iz)]) / (dx * dx);
        const float d2y = (cur[vel.index(ix, iy + 1, iz)] - 2.0f * cur[i] + cur[vel.index(ix, iy - 1, iz)]) / (dy * dy);
        const float d2z = (cur[vel.index(ix, iy, iz + 1)] - 2.0f * cur[i] + cur[vel.index(ix, iy, iz - 1)]) / (dz * dz);
        const float lap = d2x + d2y + d2z;
        const float v = vel.raw()[i];
        nxt[i] = (2.0f * cur[i] - prev[i] + (v * v) * (dt * dt) * lap) * damp[i];
      }
    }
  }
}

}  // namespace rtm3d::rtm_internal
