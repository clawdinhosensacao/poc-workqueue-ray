#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace rtm3d {

struct Marmousi2D {
  std::size_t nx{};
  std::size_t nz{};
  float dx{};
  float dz{};
  std::vector<float> v;  // [nz][nx]
};

Marmousi2D load_marmousi_js(const std::string& x_file, const std::string& z_file,
                           const std::string& vel_file, std::size_t decim_x,
                           std::size_t decim_z, std::size_t crop_x,
                           std::size_t crop_z);

}  // namespace rtm3d
