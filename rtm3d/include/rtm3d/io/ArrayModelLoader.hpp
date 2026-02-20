#pragma once

#include <string>
#include <vector>

namespace rtm3d {

std::vector<float> load_array_1d_json(const std::string& path);
std::vector<std::vector<float>> load_array_2d_json(const std::string& path);

}  // namespace rtm3d
