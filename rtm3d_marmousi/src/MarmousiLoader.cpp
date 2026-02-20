#include "rtm3d/MarmousiLoader.hpp"

#include <algorithm>
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

std::vector<float> parse_1d_array(const std::string& s) {
  auto b = s.find('[');
  auto e = s.rfind(']');
  if (b == std::string::npos || e == std::string::npos || b >= e) {
    throw std::runtime_error("invalid 1D array content");
  }
  std::vector<float> out;
  std::string num;
  for (std::size_t i = b + 1; i < e; ++i) {
    const char c = s[i];
    if (std::isdigit(c) || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E') {
      num.push_back(c);
    } else if (!num.empty()) {
      out.push_back(std::stof(num));
      num.clear();
    }
  }
  if (!num.empty()) out.push_back(std::stof(num));
  return out;
}

std::vector<std::vector<float>> parse_2d_array(const std::string& s) {
  auto b = s.find('[');
  if (b == std::string::npos) throw std::runtime_error("invalid 2D array content");

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

  for (std::size_t i = b; i < s.size(); ++i) {
    const char c = s[i];
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
    if (depth >= 2 &&
        (std::isdigit(c) || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E')) {
      num.push_back(c);
    } else {
      flush_num();
    }
  }
  return rows;
}

}  // namespace

Marmousi2D load_marmousi_js(const std::string& x_file, const std::string& z_file,
                           const std::string& vel_file, std::size_t decim_x,
                           std::size_t decim_z, std::size_t crop_x,
                           std::size_t crop_z) {
  if (decim_x == 0 || decim_z == 0) throw std::runtime_error("decimation cannot be zero");

  auto x = parse_1d_array(slurp(x_file));
  auto z = parse_1d_array(slurp(z_file));
  auto vel_rows = parse_2d_array(slurp(vel_file));

  if (vel_rows.size() != z.size()) throw std::runtime_error("vel rows != z size");
  if (vel_rows.empty() || vel_rows.front().size() != x.size()) {
    throw std::runtime_error("vel cols != x size");
  }

  const std::size_t nx0 = x.size();
  const std::size_t nz0 = z.size();
  const std::size_t nx_dec = std::min(crop_x, (nx0 + decim_x - 1) / decim_x);
  const std::size_t nz_dec = std::min(crop_z, (nz0 + decim_z - 1) / decim_z);

  Marmousi2D out;
  out.nx = nx_dec;
  out.nz = nz_dec;
  out.dx = (x[1] - x[0]) * static_cast<float>(decim_x);
  out.dz = (z[1] - z[0]) * static_cast<float>(decim_z);
  out.v.resize(out.nx * out.nz);

  for (std::size_t iz = 0; iz < out.nz; ++iz) {
    const std::size_t src_z = iz * decim_z;
    for (std::size_t ix = 0; ix < out.nx; ++ix) {
      const std::size_t src_x = ix * decim_x;
      out.v[iz * out.nx + ix] = vel_rows[src_z][src_x];
    }
  }

  return out;
}

}  // namespace rtm3d
