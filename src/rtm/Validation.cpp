#include "Validation.hpp"

#include <stdexcept>

namespace rtm3d::rtm_internal {

void validate_cfg(const GridModel2D& model, const RtmConfig& cfg) {
  if (model.nx < 8 || model.nz < 8) throw std::runtime_error("model too small");
  if (model.dx <= 0.0f || model.dz <= 0.0f) throw std::runtime_error("invalid model spacing");
  if (cfg.ny < 4 || cfg.nt < 2) throw std::runtime_error("ny/nt too small");
  if (cfg.dy <= 0.0f || cfg.dt <= 0.0f || cfg.f0 <= 0.0f) throw std::runtime_error("invalid RTM scalar parameter");
  if (cfg.receiver_stride == 0) throw std::runtime_error("receiver_stride must be > 0");
  if (cfg.pml == 0) throw std::runtime_error("pml must be > 0");
}

}  // namespace rtm3d::rtm_internal
