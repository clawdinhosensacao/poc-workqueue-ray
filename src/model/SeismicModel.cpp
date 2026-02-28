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

std::size_t velocity_cell_count(const GridSpec& grid) {
  return grid.nx * grid.nz * grid.ny;
}

void resize_velocity_buffer(SeismicModel& model) {
  model.velocity().resize(velocity_cell_count(model.grid()));
}

void read_velocity_file_or_throw(std::ifstream& file, std::vector<float>& vp,
                                 const std::string& path) {
  file.read(reinterpret_cast<char*>(vp.data()), vp.size() * sizeof(float));
  if (!file) {
    throw std::runtime_error("Failed to read velocity file: " + path);
  }
}

float cfl_dimension_factor(const GridSpec& grid) {
  return grid.ny > 1 ? kSqrt3 : kSqrt2;
}

float bounded_max_velocity(float v_max) {
  return std::max(v_max, kMinReferenceVelocity);
}

float compute_max_stable_dt(const GridSpec& grid, float v_max) {
  const float bounded_vmax = bounded_max_velocity(v_max);
  const float d_min = std::min({grid.dx, grid.dz, grid.dy});
  const float cfl_factor = cfl_dimension_factor(grid);
  return d_min / (bounded_vmax * cfl_factor);
}

template <typename ExtremumFn>
float extremum_or_zero(const std::vector<float>& values, ExtremumFn extremum_fn) {
  if (values.empty()) return 0.0f;
  return *extremum_fn(values.begin(), values.end());
}

float max_or_zero(const std::vector<float>& values) {
  return extremum_or_zero(values, std::max_element<decltype(values.begin())>);
}

float min_or_zero(const std::vector<float>& values) {
  return extremum_or_zero(values, std::min_element<decltype(values.begin())>);
}

}  // namespace

SeismicModel SeismicModel::from_preset(ModelPreset preset, const GridSpec& grid,
                                       float vp_top, float vp_bottom,
                                       std::size_t nlayers) {
  SeismicModel model(grid);
  resize_velocity_buffer(model);
  model_internal::fill_preset_velocity(model.vp_, preset, grid, vp_top, vp_bottom,
                                       nlayers);
  return model;
}

SeismicModel SeismicModel::from_velocity(const std::vector<float>& vp,
                                         const GridSpec& grid) {
  if (vp.size() != velocity_cell_count(grid)) {
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
  resize_velocity_buffer(model);
  read_velocity_file_or_throw(file, model.vp_, path);

  return model;
}

float SeismicModel::max_velocity() const { return max_or_zero(vp_); }

float SeismicModel::min_velocity() const { return min_or_zero(vp_); }

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
