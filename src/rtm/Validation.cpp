#include "Validation.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace rtm3d::rtm_internal {

void validate_cfg(const GridModel2D& model, const RtmConfig& cfg) {
  if (model.nx < 8 || model.nz < 8) throw std::runtime_error("model too small");
  if (model.dx <= 0.0f || model.dz <= 0.0f) throw std::runtime_error("invalid model spacing");
  if (cfg.ny < 1 || cfg.nt < 2) throw std::runtime_error("ny/nt too small");
  if (cfg.dy <= 0.0f || cfg.dt <= 0.0f || cfg.f0 <= 0.0f) throw std::runtime_error("invalid RTM scalar parameter");
  if (cfg.receiver_stride == 0) throw std::runtime_error("receiver_stride must be > 0");
  if (cfg.pml == 0) throw std::runtime_error("pml must be > 0");

  // CFL stability check: dt <= min(dx,dy,dz) / (v_max * sqrt(dim))
  // Uses actual max velocity from model for accurate CFL bound
  float v_max = 0.0f;
  for (float v : model.values) {
    v_max = std::max(v_max, v);
  }
  v_max = std::max(v_max, 1500.0f);  // Ensure minimum for water velocity
  
  float d_min = std::min({model.dx, model.dz, cfg.dy});
  constexpr float cfl_factor = 1.7320508f;  // sqrt(3) for 3D
  float dt_max = d_min / (v_max * cfl_factor);
  
  if (cfg.dt > dt_max) {
    throw std::runtime_error("CFL condition violated: dt too large for grid spacing");
  }
}

}  // namespace rtm3d::rtm_internal
