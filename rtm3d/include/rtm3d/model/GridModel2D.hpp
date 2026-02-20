#pragma once

#include <cstddef>
#include <vector>

namespace rtm3d {

struct GridModel2D {
  std::size_t nx{};
  std::size_t nz{};
  float dx{};
  float dz{};
  std::vector<float> values;  // [nz][nx]
};

}  // namespace rtm3d
