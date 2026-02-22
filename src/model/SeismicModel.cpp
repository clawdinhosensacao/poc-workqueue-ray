#include "rtm3d/model/SeismicModel.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace rtm3d {

SeismicModel SeismicModel::from_preset(ModelPreset preset, const GridSpec& grid,
                                        float vp_top, float vp_bottom,
                                        std::size_t nlayers) {
  SeismicModel model(grid);
  model.vp_.resize(grid.nx * grid.nz * grid.ny);

  switch (preset) {
    case ModelPreset::Constant:
      std::fill(model.vp_.begin(), model.vp_.end(), vp_top);
      break;

    case ModelPreset::Layers: {
      // Layered model with velocity gradient
      std::vector<float> vp_layer(nlayers);
      for (std::size_t i = 0; i < nlayers; ++i) {
        vp_layer[i] = vp_top + (vp_bottom - vp_top) * static_cast<float>(i) /
                                    static_cast<float>(nlayers - 1);
      }
      for (std::size_t k = 0; k < grid.nz; ++k) {
        std::size_t layer = std::min(k * nlayers / grid.nz, nlayers - 1);
        float vel = vp_layer[layer];
        for (std::size_t i = 0; i < grid.nx; ++i) {
          for (std::size_t j = 0; j < grid.ny; ++j) {
            std::size_t idx = j * grid.nz * grid.nx + k * grid.nx + i;
            model.vp_[idx] = vel;
          }
        }
      }
      break;
    }

    case ModelPreset::Circle: {
      // Circle anomaly (camembert model)
      std::fill(model.vp_.begin(), model.vp_.end(), vp_top);
      float cx = static_cast<float>(grid.nx) / 2.0f;
      float cz = static_cast<float>(grid.nz) / 2.0f;
      float radius = static_cast<float>(std::min(grid.nx, grid.nz)) / 4.0f;
      for (std::size_t k = 0; k < grid.nz; ++k) {
        for (std::size_t i = 0; i < grid.nx; ++i) {
          float dx = static_cast<float>(i) - cx;
          float dz = static_cast<float>(k) - cz;
          if (dx * dx + dz * dz <= radius * radius) {
            for (std::size_t j = 0; j < grid.ny; ++j) {
              std::size_t idx = j * grid.nz * grid.nx + k * grid.nx + i;
              model.vp_[idx] = vp_bottom;
            }
          }
        }
      }
      break;
    }

    case ModelPreset::CircleLens: {
      // Gaussian lens anomaly
      std::fill(model.vp_.begin(), model.vp_.end(), vp_top);
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
            std::size_t idx = j * grid.nz * grid.nx + k * grid.nx + i;
            model.vp_[idx] = vp_top + (vp_bottom - vp_top) * gaussian;
          }
        }
      }
      break;
    }

    case ModelPreset::SaltDome:
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
