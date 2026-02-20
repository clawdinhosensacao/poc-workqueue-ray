#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace rtm3d {

void write_pgm(const std::string& path, const std::vector<float>& img,
               std::size_t nx, std::size_t nz);

}
