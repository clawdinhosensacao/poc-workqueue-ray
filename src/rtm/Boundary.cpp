#include "Boundary.hpp"

#include <algorithm>
#include <cmath>

namespace rtm3d::rtm_internal {

std::vector<float> make_damp(std::size_t nx, std::size_t ny, std::size_t nz, std::size_t pml) {
  std::vector<float> d(nx * ny * nz, 1.0f);
  for (std::size_t iz = 0; iz < nz; ++iz) {
    for (std::size_t iy = 0; iy < ny; ++iy) {
      for (std::size_t ix = 0; ix < nx; ++ix) {
        const auto dist = std::min({ix, nx - 1 - ix, iy, ny - 1 - iy, iz, nz - 1 - iz});
        float coeff = 1.0f;
        if (dist < pml) {
          const float x = static_cast<float>(pml - dist) / static_cast<float>(pml);
          coeff = std::exp(-0.75f * x * x);  // Strong damping for effective boundary absorption
        }
        d[(iz * ny + iy) * nx + ix] = coeff;
      }
    }
  }
  return d;
}

}  // namespace rtm3d::rtm_internal
