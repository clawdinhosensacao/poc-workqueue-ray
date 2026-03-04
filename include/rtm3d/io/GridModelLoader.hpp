/**
 * @file GridModelLoader.hpp
 * @brief Grid model loading utilities for seismic velocity data.
 *
 * Provides functions to load 2D velocity models from JSON files
 * with optional decimation and cropping.
 */

#pragma once

#include <cstddef>
#include <string>

#include "rtm3d/model/GridModel2D.hpp"

namespace rtm3d {

/**
 * @brief Options for loading and preprocessing grid models.
 */
struct GridLoadOptions {
  std::size_t decim_x = 1;   ///< Decimation factor in X (1 = no decimation)
  std::size_t decim_z = 1;   ///< Decimation factor in Z (1 = no decimation)
  std::size_t crop_x = 0;    ///< Crop size in X (0 = no crop, use full model)
  std::size_t crop_z = 0;    ///< Crop size in Z (0 = no crop, use full model)
};

/**
 * @brief Load a 2D grid model from JSON array files.
 *
 * Reads coordinate arrays (x, z) and a 2D velocity array from separate
 * JSON files, applying optional decimation and cropping.
 *
 * @param x_file Path to X coordinates JSON array
 * @param z_file Path to Z coordinates JSON array
 * @param values_file Path to 2D velocity values JSON array [nz][nx]
 * @param opts Loading options (decimation, cropping)
 * @return Loaded GridModel2D
 * @throws std::runtime_error on file errors or size mismatches
 */
GridModel2D load_grid_model_from_json_arrays(const std::string& x_file,
                                             const std::string& z_file,
                                             const std::string& values_file,
                                             const GridLoadOptions& opts);

}  // namespace rtm3d
