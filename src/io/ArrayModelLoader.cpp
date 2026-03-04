#include "rtm3d/io/ArrayModelLoader.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace rtm3d {
namespace {

std::string slurp(const std::string& path) {
  std::ifstream f(path);
  if (!f) throw std::runtime_error("cannot open file: " + path);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

bool is_num_char(const char c) {
  return std::isdigit(static_cast<unsigned char>(c)) != 0 || c == '-' || c == '+' || c == '.' ||
         c == 'e' || c == 'E';
}

}  // namespace

std::vector<float> load_array_1d_json(const std::string& path) {
  const auto s = slurp(path);
  const auto b = s.find('[');
  const auto e = s.rfind(']');
  if (b == std::string::npos || e == std::string::npos || b >= e) {
    throw std::runtime_error("invalid 1D JSON array in: " + path);
  }

  std::vector<float> out;
  std::string num;
  for (std::size_t i = b + 1; i < e; ++i) {
    const char c = s[i];
    if (is_num_char(c)) {
      num.push_back(c);
    } else if (!num.empty()) {
      out.push_back(std::stof(num));
      num.clear();
    }
  }
  if (!num.empty()) out.push_back(std::stof(num));
  if (out.empty()) throw std::runtime_error("empty 1D array: " + path);
  return out;
}

std::vector<std::vector<float>> load_array_2d_json(const std::string& path) {
  const auto s = slurp(path);
  if (s.find('[') == std::string::npos) throw std::runtime_error("invalid 2D JSON array in: " + path);

  std::vector<std::vector<float>> rows;
  std::vector<float> row;
  std::string num;
  int depth = 0;

  auto flush_num = [&]() {
    if (!num.empty()) {
      row.push_back(std::stof(num));
      num.clear();
    }
  };

  for (char c : s) {
    if (c == '[') {
      ++depth;
      if (depth == 2) row.clear();
      continue;
    }
    if (c == ']') {
      flush_num();
      if (depth == 2 && !row.empty()) rows.push_back(row);
      --depth;
      continue;
    }
    if (depth >= 2 && is_num_char(c)) {
      num.push_back(c);
    } else {
      flush_num();
    }
  }

  if (rows.empty()) throw std::runtime_error("empty 2D array: " + path);
  return rows;
}

}  // namespace rtm3d
