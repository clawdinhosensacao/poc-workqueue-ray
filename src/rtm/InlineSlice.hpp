#pragma once

/**
 * @file InlineSlice.hpp
 * @brief Volume slice extraction utilities for 3D RTM results.
 *
 * Provides functions to extract 2D slices from 3D migration volumes
 * for visualization and analysis.
 */

#include <vector>

#include "rtm3d/core/Volume3D.hpp"

namespace rtm3d::rtm_internal {

/**
 * @brief Extract inline XZ slice at center Y position.
 *
 * Extracts a vertical slice through the volume at the middle Y coordinate.
 * This is the standard view for 2D migration results from 3D volumes.
 *
 * @param vol 3D volume specification
 * @param image 3D image data (size = vol.nx * vol.ny * vol.nz)
 * @return 2D slice of size [nz][nx] (row-major)
 */
std::vector<float> extract_inline_xz(const Volume3D& vol, const std::vector<float>& image);

/**
 * @brief Extract crossline YZ slice at center X position.
 *
 * Extracts a vertical slice perpendicular to the inline direction.
 * Useful for crossline interpretation in 3D surveys.
 *
 * @param vol 3D volume specification
 * @param image 3D image data
 * @return 2D slice of size [nz][ny] (row-major)
 */
std::vector<float> extract_crossline_yz(const Volume3D& vol, const std::vector<float>& image);

/**
 * @brief Extract horizontal depth slice XY at given depth.
 *
 * Extracts a horizontal slice at a specific depth level.
 * Useful for time/depth slice interpretation.
 *
 * @param vol 3D volume specification
 * @param image 3D image data
 * @param iz Depth index (clamped to valid range)
 * @return 2D slice of size [ny][nx] (row-major)
 */
std::vector<float> extract_depth_xy(const Volume3D& vol, const std::vector<float>& image,
                                     std::size_t iz);

}  // namespace rtm3d::rtm_internal
