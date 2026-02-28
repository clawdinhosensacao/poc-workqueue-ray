#include "rtm3d/model/ModelPresetsInternal.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace rtm3d::model_internal {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kFaultDipDegrees = 60.0f;
constexpr float kFaultXRatio = 0.45f;
constexpr float kFaultThrowRatio = 0.08f;
constexpr float kCircleRadiusDivisor = 4.0f;
constexpr float kCircleLensCenterXRatio = 0.48f;
constexpr float kCircleLensCenterZRatio = 0.52f;
constexpr float kCircleLensSigmaRatio = 0.14f;
constexpr float kSaltTopDepthRatio = 0.12f;
constexpr float kSaltBaseDepthRatio = 0.75f;
constexpr float kSaltBaseRadiusRatio = 0.18f;

float degrees_to_radians(float degrees) { return degrees * kPi / 180.0f; }

float ratio(std::size_t value, std::size_t max_value) {
  if (max_value == 0) return 0.0f;
  return static_cast<float>(value) / static_cast<float>(max_value);
}

float grid_center_x(const GridSpec& grid) {
  return static_cast<float>(grid.nx) / 2.0f;
}

float grid_center_z(const GridSpec& grid) {
  return static_cast<float>(grid.nz) / 2.0f;
}

float scaled_nx(const GridSpec& grid, float ratio_value) {
  return static_cast<float>(grid.nx) * ratio_value;
}

float scaled_nz(const GridSpec& grid, float ratio_value) {
  return static_cast<float>(grid.nz) * ratio_value;
}

float scaled_min_nx_nz(const GridSpec& grid, float ratio_value) {
  return static_cast<float>(std::min(grid.nx, grid.nz)) * ratio_value;
}

float default_circle_radius(const GridSpec& grid) {
  return scaled_min_nx_nz(grid, 1.0f / kCircleRadiusDivisor);
}

float clampf(float value, float lo, float hi) {
  return std::max(lo, std::min(hi, value));
}

std::size_t layer_index_from_depth(float depth, std::size_t nz,
                                   std::size_t nlayers) {
  const float clamped_depth = clampf(depth, 0.0f, static_cast<float>(nz - 1));
  const auto layer = static_cast<std::size_t>(
      clamped_depth * static_cast<float>(nlayers) / static_cast<float>(nz));
  return std::min(layer, nlayers - 1);
}

std::size_t layer_index_from_depth(std::size_t depth, std::size_t nz,
                                   std::size_t nlayers) {
  return layer_index_from_depth(static_cast<float>(depth), nz, nlayers);
}

std::size_t grid_cell_count(const GridSpec& grid) {
  return grid.nx * grid.nz * grid.ny;
}

std::size_t velocity_index(const GridSpec& grid, std::size_t i, std::size_t k,
                           std::size_t j) {
  return j * grid.nz * grid.nx + k * grid.nx + i;
}

void set_velocity_along_y(std::vector<float>& vp, const GridSpec& grid,
                          std::size_t i, std::size_t k, float velocity) {
  for (std::size_t j = 0; j < grid.ny; ++j) {
    vp[velocity_index(grid, i, k, j)] = velocity;
  }
}

std::vector<float> build_layer_velocities(float vp_top, float vp_bottom,
                                          std::size_t nlayers) {
  std::vector<float> vp_layer(nlayers);
  for (std::size_t layer_idx = 0; layer_idx < nlayers; ++layer_idx) {
    vp_layer[layer_idx] = vp_top +
                          (vp_bottom - vp_top) * static_cast<float>(layer_idx) /
                              static_cast<float>(nlayers - 1);
  }
  return vp_layer;
}

void fill_layered_background(std::vector<float>& vp, const GridSpec& grid,
                             float vp_top, float vp_bottom,
                             std::size_t nlayers) {
  std::vector<float> vp_layer = build_layer_velocities(vp_top, vp_bottom, nlayers);

  for (std::size_t k = 0; k < grid.nz; ++k) {
    const std::size_t layer = layer_index_from_depth(k, grid.nz, nlayers);
    float vel = vp_layer[layer];
    for (std::size_t i = 0; i < grid.nx; ++i) {
      set_velocity_along_y(vp, grid, i, k, vel);
    }
  }
}

void build_constant_model(std::vector<float>& vp, float velocity) {
  std::fill(vp.begin(), vp.end(), velocity);
}

void build_circle_model(std::vector<float>& vp, const GridSpec& grid, float vp_top,
                        float vp_bottom) {
  std::fill(vp.begin(), vp.end(), vp_top);
  float cx = grid_center_x(grid);
  float cz = grid_center_z(grid);
  float radius = default_circle_radius(grid);

  for (std::size_t k = 0; k < grid.nz; ++k) {
    for (std::size_t i = 0; i < grid.nx; ++i) {
      float dx = static_cast<float>(i) - cx;
      float dz = static_cast<float>(k) - cz;
      if (dx * dx + dz * dz <= radius * radius) {
        set_velocity_along_y(vp, grid, i, k, vp_bottom);
      }
    }
  }
}

void build_circle_lens_model(std::vector<float>& vp, const GridSpec& grid,
                             float vp_top, float vp_bottom) {
  std::fill(vp.begin(), vp.end(), vp_top);
  float cx = scaled_nx(grid, kCircleLensCenterXRatio);
  float cz = scaled_nz(grid, kCircleLensCenterZRatio);
  float sx = scaled_nx(grid, kCircleLensSigmaRatio);
  float sz = scaled_nz(grid, kCircleLensSigmaRatio);

  for (std::size_t k = 0; k < grid.nz; ++k) {
    for (std::size_t i = 0; i < grid.nx; ++i) {
      float dx = (static_cast<float>(i) - cx) / sx;
      float dz = (static_cast<float>(k) - cz) / sz;
      float gaussian = std::exp(-(dx * dx + dz * dz));
      set_velocity_along_y(vp, grid, i, k,
                           vp_top + (vp_bottom - vp_top) * gaussian);
    }
  }
}

void build_salt_dome_model(std::vector<float>& vp, const GridSpec& grid,
                           float vp_top, float vp_bottom) {
  constexpr float kSaltVelocity = 4500.0f;

  for (std::size_t k = 0; k < grid.nz; ++k) {
    float depth_ratio = ratio(k, grid.nz - 1);
    float bg_vel = vp_top + (vp_bottom - vp_top) * depth_ratio;
    for (std::size_t i = 0; i < grid.nx; ++i) {
      set_velocity_along_y(vp, grid, i, k, bg_vel);
    }
  }

  float cx = grid_center_x(grid);
  float base_radius = scaled_nx(grid, kSaltBaseRadiusRatio);
  float top_z = scaled_nz(grid, kSaltTopDepthRatio);
  float base_z = scaled_nz(grid, kSaltBaseDepthRatio);

  for (std::size_t k = 0; k < grid.nz; ++k) {
    float z = static_cast<float>(k);
    if (z < top_z) continue;

    float z_norm = (z - top_z) / (base_z - top_z);
    z_norm = clampf(z_norm, 0.0f, 1.0f);

    float taper = std::sqrt(z_norm);
    float radius = base_radius * taper;
    if (z_norm < 0.3f) {
      radius *= 0.6f + 0.4f * (z_norm / 0.3f);
    }

    for (std::size_t i = 0; i < grid.nx; ++i) {
      float dx = static_cast<float>(i) - cx;
      if (dx * dx <= radius * radius) {
        set_velocity_along_y(vp, grid, i, k, kSaltVelocity);
      }
    }
  }
}

void build_fault_model(std::vector<float>& vp, const GridSpec& grid, float vp_top,
                       float vp_bottom, std::size_t nlayers) {
  float fault_x = scaled_nx(grid, kFaultXRatio);
  float dip = degrees_to_radians(kFaultDipDegrees);
  float throw_amount = scaled_nz(grid, kFaultThrowRatio);

  std::vector<float> vp_layer = build_layer_velocities(vp_top, vp_bottom, nlayers);

  for (std::size_t k = 0; k < grid.nz; ++k) {
    for (std::size_t i = 0; i < grid.nx; ++i) {
      float fault_at_depth = fault_x - static_cast<float>(k) / std::tan(dip);

      float effective_z = static_cast<float>(k);
      if (static_cast<float>(i) > fault_at_depth) {
        effective_z -= throw_amount;
      }

      const std::size_t layer =
          layer_index_from_depth(effective_z, grid.nz, nlayers);
      float vel = vp_layer[layer];
      set_velocity_along_y(vp, grid, i, k, vel);
    }
  }
}

}  // namespace

void ensure_velocity_size_matches_grid_or_throw(const std::vector<float>& vp,
                                                 const GridSpec& grid) {
  const std::size_t expected = grid_cell_count(grid);
  if (vp.size() != expected) {
    throw std::runtime_error("velocity vector size mismatch with grid dimensions");
  }
}

void fill_preset_velocity(std::vector<float>& vp, ModelPreset preset,
                          const GridSpec& grid, float vp_top,
                          float vp_bottom, std::size_t nlayers) {
  ensure_velocity_size_matches_grid_or_throw(vp, grid);
  switch (preset) {
    case ModelPreset::Constant:
      build_constant_model(vp, vp_top);
      break;

    case ModelPreset::Layers:
      fill_layered_background(vp, grid, vp_top, vp_bottom, nlayers);
      break;

    case ModelPreset::Circle:
      build_circle_model(vp, grid, vp_top, vp_bottom);
      break;

    case ModelPreset::CircleLens:
      build_circle_lens_model(vp, grid, vp_top, vp_bottom);
      break;

    case ModelPreset::SaltDome:
      build_salt_dome_model(vp, grid, vp_top, vp_bottom);
      break;

    case ModelPreset::Fault:
      build_fault_model(vp, grid, vp_top, vp_bottom, nlayers);
      break;

    case ModelPreset::Marmousi2D:
      throw std::runtime_error("Preset requires external data file, use from_file()");
  }
}

}  // namespace rtm3d::model_internal
