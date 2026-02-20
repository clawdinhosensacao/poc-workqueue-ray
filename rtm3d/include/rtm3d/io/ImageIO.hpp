#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace rtm3d {

void write_pgm(const std::string& path, const std::vector<float>& image, std::size_t nx,
               std::size_t nz);
void write_float32_raw(const std::string& path, const std::vector<float>& image,
                       std::size_t nx, std::size_t nz);

}  // namespace rtm3d
