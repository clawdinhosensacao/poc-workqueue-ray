/**
 * @file Volume3D.hpp
 * @brief 3D volume container for seismic wavefield data.
 *
 * Provides efficient storage and indexing for 3D seismic volumes
 * used in RTM propagation and imaging.
 */

#pragma once

#include <cstddef>
#include <vector>

namespace rtm3d {

/**
 * @brief 3D volume container with row-major storage.
 *
 * Stores a 3D scalar field (e.g., velocity, wavefield, image)
 * with efficient linear indexing. Memory layout is [iz][iy][ix]
 * for cache-friendly depth-wise access in RTM kernels.
 *
 * @example
 * @code
 * Volume3D wavefield(100, 50, 80);  // nx=100, ny=50, nz=80
 * wavefield(10, 25, 40) = 1.0f;      // Set value at (x,y,z)
 * float v = wavefield(10, 25, 40);   // Get value
 * @endcode
 */
class Volume3D {
 public:
  /**
   * @brief Construct a 3D volume with given dimensions.
   * @param nx Number of grid points in X (inline) direction
   * @param ny Number of grid points in Y (crossline) direction
   * @param nz Number of grid points in Z (depth) direction
   * @param init Initial value for all elements (default 0.0)
   */
  Volume3D(std::size_t nx, std::size_t ny, std::size_t nz, float init = 0.0f)
      : nx_(nx), ny_(ny), nz_(nz), data_(nx * ny * nz, init) {}

  /**
   * @brief Compute linear index from 3D coordinates.
   * @param ix X coordinate
   * @param iy Y coordinate
   * @param iz Z coordinate
   * @return Linear index into data array
   */
  std::size_t index(std::size_t ix, std::size_t iy, std::size_t iz) const {
    return (iz * ny_ + iy) * nx_ + ix;
  }

  /// @brief Access element at (ix, iy, iz)
  float& operator()(std::size_t ix, std::size_t iy, std::size_t iz) { return data_[index(ix, iy, iz)]; }
  
  /// @brief Const access element at (ix, iy, iz)
  const float& operator()(std::size_t ix, std::size_t iy, std::size_t iz) const {
    return data_[index(ix, iy, iz)];
  }

  std::size_t nx() const { return nx_; }  ///< Grid points in X
  std::size_t ny() const { return ny_; }  ///< Grid points in Y
  std::size_t nz() const { return nz_; }  ///< Grid points in Z
  std::size_t size() const { return data_.size(); }  ///< Total number of elements

  std::vector<float>& raw() { return data_; }  ///< Mutable access to underlying data
  const std::vector<float>& raw() const { return data_; }  ///< Const access to underlying data

 private:
  std::size_t nx_ = 0, ny_ = 0, nz_ = 0;
  std::vector<float> data_;
};

}  // namespace rtm3d
