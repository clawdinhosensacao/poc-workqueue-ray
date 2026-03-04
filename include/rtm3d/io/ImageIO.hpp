/**
 * @file ImageIO.hpp
 * @brief Image output utilities for RTM results.
 *
 * Provides functions to write migrated images to various formats
 * including PGM (8-bit grayscale) and raw float32 binary.
 */

#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace rtm3d {

/**
 * @brief Write image as 8-bit PGM file.
 *
 * Normalizes the image to [0, 255] and writes as PGM format.
 * Useful for quick visualization of migration results.
 *
 * @param path Output file path
 * @param image Image data (row-major [nz][nx])
 * @param nx Number of columns
 * @param nz Number of rows
 */
void write_pgm(const std::string& path, const std::vector<float>& image, std::size_t nx,
               std::size_t nz);

/**
 * @brief Write image as raw float32 binary with JSON header.
 *
 * Writes the image as raw float32 bytes, plus a JSON header file
 * with shape metadata. Suitable for further processing in Python.
 *
 * @param path Output file path (header will be path + ".json")
 * @param image Image data (row-major [nz][nx])
 * @param nx Number of columns
 * @param nz Number of rows
 */
void write_float32_raw(const std::string& path, const std::vector<float>& image,
                       std::size_t nx, std::size_t nz);

}  // namespace rtm3d
