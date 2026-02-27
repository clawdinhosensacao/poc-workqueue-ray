#include "rtm3d/model/SeismicModel.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace rtm3d {
namespace {

std::size_t velocity_index(const GridSpec& grid, std::size_t i, std::size_t k,
                           std::size_t j) {
  return j * grid.nz * grid.nx + k * grid.nx + i;
}

void fill_layered_background(std::vector<float>& vp, const GridSpec& grid,
                             float vp_top, float vp_bottom,
                             std::size_t nlayers) {
  std::vector<float> vp_layer(nlayers);
  for (std::size_t layer_idx = 0; layer_idx < nlayers; ++layer_idx) {
    vp_layer[layer_idx] = vp_top +
                          (vp_bottom - vp_top) * static_cast<float>(layer_idx) /
                              static_cast<float>(nlayers - 1);
  }

  for (std::size_t k = 0; k < grid.nz; ++k) {
    std::size_t layer = std::min(k * nlayers / grid.nz, nlayers - 1);
    float vel = vp_layer[layer];
    for (std::size_t i = 0; i < grid.nx; ++i) {
      for (std::size_t j = 0; j < grid.ny; ++j) {
        vp[velocity_index(grid, i, k, j)] = vel;
      }
    }
  }
}

void build_constant_model(std::vector<float>& vp, float velocity) {
  std::fill(vp.begin(), vp.end(), velocity);
}

void build_circle_model(std::vector<float>& vp, const GridSpec& grid, float vp_top,
                        float vp_bottom) {
  std::fill(vp.begin(), vp.end(), vp_top);
  float cx = static_cast<float>(grid.nx) / 2.0f;
  float cz = static_cast<float>(grid.nz) / 2.0f;
  float radius = static_cast<float>(std::min(grid.nx, grid.nz)) / 4.0f;

  for (std::size_t k = 0; k < grid.nz; ++k) {
    for (std::size_t i = 0; i < grid.nx; ++i) {
      float dx = static_cast<float>(i) - cx;
      float dz = static_cast<float>(k) - cz;
      if (dx * dx + dz * dz <= radius * radius) {
        for (std::size_t j = 0; j < grid.ny; ++j) {
          vp[velocity_index(grid, i, k, j)] = vp_bottom;
        }
      }
    }
  }
}

void build_circle_lens_model(std::vector<float>& vp, const GridSpec& grid,
                             float vp_top, float vp_bottom) {
  std::fill(vp.begin(), vp.end(), vp_top);
  float cx = static_cast<float>(grid.nx) * 0.48f;
  float cz = static_cast<float>(grid.nz) * 0.52f;
  float sx = static_cast<float>(grid.nx) * 0.14f;
  float sz = static_cast<float>(grid.nz) * 0.14f;

  for (std::size_t k = 0; k < grid.nz; ++k) {
    for (std::size_t i = 0; i < grid.nx; ++i) {
      float dx = (static_cast<float>(i) - cx) / sx;
      float dz = (static_cast<float>(k) - cz) / sz;
      float gaussian = std::exp(-(dx * dx + dz * dz));
      for (std::size_t j = 0; j < grid.ny; ++j) {
        vp[velocity_index(grid, i, k, j)] =
            vp_top + (vp_bottom - vp_top) * gaussian;
      }
    }
  }
}

void build_salt_dome_model(std::vector<float>& vp, const GridSpec& grid,
                           float vp_top, float vp_bottom) {
  constexpr float kSaltVelocity = 4500.0f;

  for (std::size_t k = 0; k < grid.nz; ++k) {
    float depth_ratio = static_cast<float>(k) / static_cast<float>(grid.nz - 1);
    float bg_vel = vp_top + (vp_bottom - vp_top) * depth_ratio;
    for (std::size_t i = 0; i < grid.nx; ++i) {
      for (std::size_t j = 0; j < grid.ny; ++j) {
        vp[velocity_index(grid, i, k, j)] = bg_vel;
      }
    }
  }

  float cx = static_cast<float>(grid.nx) / 2.0f;
  float base_radius = static_cast<float>(grid.nx) * 0.18f;
  float top_z = static_cast<float>(grid.nz) * 0.12f;
  float base_z = static_cast<float>(grid.nz) * 0.75f;

  for (std::size_t k = 0; k < grid.nz; ++k) {
    float z = static_cast<float>(k);
    if (z < top_z) continue;

    float z_norm = (z - top_z) / (base_z - top_z);
    z_norm = std::max(0.0f, std::min(1.0f, z_norm));

    float taper = std::sqrt(z_norm);
    float radius = base_radius * taper;
    if (z_norm < 0.3f) {
      radius *= 0.6f + 0.4f * (z_norm / 0.3f);
    }

    for (std::size_t i = 0; i < grid.nx; ++i) {
      float dx = static_cast<float>(i) - cx;
      if (dx * dx <= radius * radius) {
        for (std::size_t j = 0; j < grid.ny; ++j) {
          vp[velocity_index(grid, i, k, j)] = kSaltVelocity;
        }
      }
    }
  }
}

void build_fault_model(std::vector<float>& vp, const GridSpec& grid, float vp_top,
                       float vp_bottom, std::size_t nlayers) {
  float fault_x = static_cast<float>(grid.nx) * 0.45f;
  float dip = 60.0f * 3.14159265f / 180.0f;
  float throw_amount = static_cast<float>(grid.nz) * 0.08f;

  std::vector<float> vp_layer(nlayers);
  for (std::size_t layer_idx = 0; layer_idx < nlayers; ++layer_idx) {
    vp_layer[layer_idx] = vp_top +
                          (vp_bottom - vp_top) * static_cast<float>(layer_idx) /
                              static_cast<float>(nlayers - 1);
  }

  for (std::size_t k = 0; k < grid.nz; ++k) {
    for (std::size_t i = 0; i < grid.nx; ++i) {
      float fault_at_depth = fault_x - static_cast<float>(k) / std::tan(dip);

      float effective_z = static_cast<float>(k);
      if (static_cast<float>(i) > fault_at_depth) {
        effective_z -= throw_amount;
      }

      effective_z =
          std::max(0.0f, std::min(static_cast<float>(grid.nz - 1), effective_z));

      std::size_t layer = static_cast<std::size_t>(
          effective_z * static_cast<float>(nlayers) / static_cast<float>(grid.nz));
      layer = std::min(layer, nlayers - 1);
      float vel = vp_layer[layer];

      for (std::size_t j = 0; j < grid.ny; ++j) {
        vp[velocity_index(grid, i, k, j)] = vel;
      }
    }
  }
}

}  // namespace

SeismicModel SeismicModel::from_preset(ModelPreset preset, const GridSpec& grid,
                                       float vp_top, float vp_bottom,
                                       std::size_t nlayers) {
  SeismicModel model(grid);
  model.vp_.resize(grid.nx * grid.nz * grid.ny);

  switch (preset) {
    case ModelPreset::Constant:
      build_constant_model(model.vp_, vp_top);
      break;

    case ModelPreset::Layers:
      fill_layered_background(model.vp_, grid, vp_top, vp_bottom, nlayers);
      break;

    case ModelPreset::Circle:
      build_circle_model(model.vp_, grid, vp_top, vp_bottom);
      break;

    case ModelPreset::CircleLens:
      build_circle_lens_model(model.vp_, grid, vp_top, vp_bottom);
      break;

    case ModelPreset::SaltDome:
      build_salt_dome_model(model.vp_, grid, vp_top, vp_bottom);
      break;

    case ModelPreset::Fault:
      build_fault_model(model.vp_, grid, vp_top, vp_bottom, nlayers);
      break;

    case ModelPreset::Marmousi2D:
      throw std::runtime_error("Preset requires external data file, use from_file()");
  }

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
