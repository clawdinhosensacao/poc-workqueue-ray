/**
 * @file ArrayModelLoader.hpp
 * @brief JSON array loading utilities.
 *
 * Provides simple functions to load 1D and 2D arrays from JSON files.
 * Used internally by GridModelLoader.
 */

#pragma once

#include <string>
#include <vector>

namespace rtm3d {

/**
 * @brief Load a 1D array from a JSON file.
 *
 * Expects a JSON array of numbers, e.g., [1.0, 2.0, 3.0]
 *
 * @param path Path to JSON file
 * @return Vector of float values
 * @throws std::runtime_error on file errors or invalid JSON
 */
std::vector<float> load_array_1d_json(const std::string& path);

/**
 * @brief Load a 2D array from a JSON file.
 *
 * Expects a JSON array of arrays, e.g., [[1.0, 2.0], [3.0, 4.0]]
 *
 * @param path Path to JSON file
 * @return 2D vector (outer = rows, inner = columns)
 * @throws std::runtime_error on file errors or invalid JSON
 */
std::vector<std::vector<float>> load_array_2d_json(const std::string& path);

}  // namespace rtm3d
