#include "rtm3d/io/GridModelLoader.hpp"

#include <algorithm>
#include <stdexcept>

#include "rtm3d/io/ArrayModelLoader.hpp"

namespace rtm3d {

GridModel2D load_grid_model_from_json_arrays(const std::string& x_file,
                                             const std::string& z_file,
                                             const std::string& values_file,
                                             const GridLoadOptions& opts) {
  if (opts.decim_x == 0 || opts.decim_z == 0) throw std::runtime_error("decimation must be >= 1");

  const auto x = load_array_1d_json(x_file);
  const auto z = load_array_1d_json(z_file);
  const auto values = load_array_2d_json(values_file);

  if (x.size() < 2 || z.size() < 2) throw std::runtime_error("grid axes must have at least two samples");
  if (values.size() != z.size()) throw std::runtime_error("values row count must match z size");
  for (const auto& row : values) {
    if (row.size() != x.size()) throw std::runtime_error("values col count must match x size");
  }

  const std::size_t nx0 = x.size();
  const std::size_t nz0 = z.size();
  const std::size_t nx_max = (nx0 + opts.decim_x - 1) / opts.decim_x;
  const std::size_t nz_max = (nz0 + opts.decim_z - 1) / opts.decim_z;
  const std::size_t nx = opts.crop_x == 0 ? nx_max : std::min(opts.crop_x, nx_max);
  const std::size_t nz = opts.crop_z == 0 ? nz_max : std::min(opts.crop_z, nz_max);

  GridModel2D out;
  out.nx = nx;
  out.nz = nz;
  out.dx = (x[1] - x[0]) * static_cast<float>(opts.decim_x);
  out.dz = (z[1] - z[0]) * static_cast<float>(opts.decim_z);
  out.values.resize(nx * nz);

  for (std::size_t iz = 0; iz < nz; ++iz) {
    const std::size_t src_z = iz * opts.decim_z;
    for (std::size_t ix = 0; ix < nx; ++ix) {
      const std::size_t src_x = ix * opts.decim_x;
      out.values[iz * nx + ix] = values[src_z][src_x];
    }
  }

  return out;
}

}  // namespace rtm3d
