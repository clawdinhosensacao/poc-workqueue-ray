#include "rtm3d/model/SeismicModel.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>

#include "rtm3d/model/ModelPresetsInternal.hpp"

namespace rtm3d {
namespace {

constexpr float kMinReferenceVelocity = 1500.0f;
constexpr float kSqrt2 = 1.41421356f;
constexpr float kSqrt3 = 1.7320508f;

float compute_max_stable_dt(const GridSpec& grid, float v_max) {
  const float bounded_vmax = std::max(v_max, kMinReferenceVelocity);
  const float d_min = std::min({grid.dx, grid.dz, grid.dy});
  const float cfl_factor = grid.ny > 1 ? kSqrt3 : kSqrt2;
  return d_min / (bounded_vmax * cfl_factor);
}

}  // namespace

SeismicModel SeismicModel::from_preset(ModelPreset preset, const GridSpec& grid,
                                       float vp_top, float vp_bottom,
                                       std::size_t nlayers) {
  SeismicModel model(grid);
  model.vp_.resize(grid.nx * grid.nz * grid.ny);
  model_internal::fill_preset_velocity(model.vp_, preset, grid, vp_top, vp_bottom,
                                       nlayers);
  return model;
}

SeismicModel SeismicModel::from_velocity(const std::vector<float>& vp,
                                         const GridSpec& grid) {
  if (vp.size() != grid.nx * grid.nz * grid.ny) {
    throw std::runtime_error("Velocity array size mismatch with grid dimensions");
  }
  SeismicModel model(grid);
  model.vp_ = vp;
  return model;
}

SeismicModel SeismicModel::from_file(const std::string& path, const GridSpec& grid) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Cannot open velocity file: " + path);
  }

  SeismicModel model(grid);
  model.vp_.resize(grid.nx * grid.nz * grid.ny);
  file.read(reinterpret_cast<char*>(model.vp_.data()),
            model.vp_.size() * sizeof(float));

  if (!file) {
    throw std::runtime_error("Failed to read velocity file: " + path);
  }

  return model;
}

float SeismicModel::max_velocity() const {
  if (vp_.empty()) return 0.0f;
  return *std::max_element(vp_.begin(), vp_.end());
}

float SeismicModel::min_velocity() const {
  if (vp_.empty()) return 0.0f;
  return *std::min_element(vp_.begin(), vp_.end());
}

void SeismicModel::validate_for_rtm(const TimeAxis& time) const {
  if (vp_.empty()) {
    throw std::runtime_error("Empty velocity model");
  }

  // CFL check: dt <= min(dx,dy,dz) / (v_max * sqrt(dim))
  const float dt_max = compute_max_stable_dt(grid_, max_velocity());

  if (time.step > dt_max) {
    throw std::runtime_error("CFL condition violated: dt=" +
                             std::to_string(time.step) +
                             " > max_dt=" + std::to_string(dt_max));
  }
}

}  // namespace rtm3d
