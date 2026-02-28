#include "rtm3d/model/SeismicModel.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>

#include "rtm3d/model/ModelPresetsInternal.hpp"

namespace rtm3d {

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
  float v_max = max_velocity();
  v_max = std::max(v_max, 1500.0f);

  float d_min = std::min({grid_.dx, grid_.dz, grid_.dy});
  float cfl_factor = grid_.ny > 1 ? 1.7320508f : 1.41421356f;  // sqrt(3) or sqrt(2)
  float dt_max = d_min / (v_max * cfl_factor);

  if (time.step > dt_max) {
    throw std::runtime_error("CFL condition violated: dt=" +
                             std::to_string(time.step) +
                             " > max_dt=" + std::to_string(dt_max));
  }
}

}  // namespace rtm3d
