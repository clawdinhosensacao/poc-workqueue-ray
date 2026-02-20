#pragma once

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace rtm3d {

class Volume3D {
 public:
  Volume3D() = default;
  Volume3D(std::size_t nx, std::size_t ny, std::size_t nz, float init = 0.0f)
      : nx_(nx), ny_(ny), nz_(nz), data_(nx * ny * nz, init) {}

  float& operator()(std::size_t ix, std::size_t iy, std::size_t iz) {
    return data_.at(index(ix, iy, iz));
  }
  const float& operator()(std::size_t ix, std::size_t iy, std::size_t iz) const {
    return data_.at(index(ix, iy, iz));
  }

  std::size_t index(std::size_t ix, std::size_t iy, std::size_t iz) const {
    return (iz * ny_ + iy) * nx_ + ix;
  }

  std::size_t nx() const { return nx_; }
  std::size_t ny() const { return ny_; }
  std::size_t nz() const { return nz_; }
  std::size_t size() const { return data_.size(); }

  std::vector<float>& raw() { return data_; }
  const std::vector<float>& raw() const { return data_; }

 private:
  std::size_t nx_ = 0, ny_ = 0, nz_ = 0;
  std::vector<float> data_;
};

}  // namespace rtm3d
