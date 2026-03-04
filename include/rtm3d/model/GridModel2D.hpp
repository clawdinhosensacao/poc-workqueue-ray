/**
 * @file GridModel2D.hpp
 * @brief 2D grid model for seismic velocity fields.
 *
 * Simple data structure for 2D velocity models used as input
 * to RTM migration. Extended to 3D internally by the RTM engine.
 */

#pragma once

#include <cstddef>
#include <vector>

namespace rtm3d {

/**
 * @brief 2D velocity model on a regular grid.
 *
 * Represents a seismic velocity field defined on a 2D rectilinear grid.
 * The model is extended to 3D during RTM by replicating in the Y direction.
 *
 * Memory layout: row-major [nz][nx], where z is depth.
 */
struct GridModel2D {
  std::size_t nx{};           ///< Number of grid points in X (inline)
  std::size_t nz{};           ///< Number of grid points in Z (depth)
  float dx{};                 ///< Grid spacing in X (meters)
  float dz{};                 ///< Grid spacing in Z (meters)
  std::vector<float> values;  ///< Velocity values [m/s], row-major [nz][nx]
};

}  // namespace rtm3d
