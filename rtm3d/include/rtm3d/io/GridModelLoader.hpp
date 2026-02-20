#pragma once

#include <cstddef>
#include <string>

#include "rtm3d/model/GridModel2D.hpp"

namespace rtm3d {

struct GridLoadOptions {
  std::size_t decim_x = 1;
  std::size_t decim_z = 1;
  std::size_t crop_x = 0;  // 0 means no crop
  std::size_t crop_z = 0;  // 0 means no crop
};

GridModel2D load_grid_model_from_json_arrays(const std::string& x_file,
                                             const std::string& z_file,
                                             const std::string& values_file,
                                             const GridLoadOptions& opts);

}  // namespace rtm3d
