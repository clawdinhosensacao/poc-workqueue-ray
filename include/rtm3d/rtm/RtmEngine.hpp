#pragma once

/**
 * @file RtmEngine.hpp
 * @brief Main RTM (Reverse Time Migration) API for 3D acoustic wave imaging.
 *
 * This module provides a clean API for running RTM migrations on 2D velocity
 * models extended to 3D volumes. Key features:
 * - Single-shot and multi-shot migration
 * - PML absorbing boundaries
 * - Cross-correlation imaging condition
 * - Automatic CFL stability validation
 *
 * @example Basic usage:
 * @code
 * #include "rtm3d/rtm/RtmEngine.hpp"
 * #include "rtm3d/io/GridModelLoader.hpp"
 *
 * // Load velocity model
 * auto model = rtm3d::load_grid_model_from_json_arrays("x.json", "z.json", "vel.json");
 *
 * // Configure RTM
 * rtm3d::RtmConfig cfg;
 * cfg.ny = 20;
 * cfg.nt = 200;
 * cfg.dt = 0.001f;
 *
 * // Run single-shot migration
 * auto result = rtm3d::run_single_shot_rtm(model, cfg);
 *
 * // Access migrated image
 * for (std::size_t i = 0; i < result.nx * result.nz; ++i) {
 *     std::cout << result.inline_xz[i] << "\n";
 * }
 * @endcode
 */

#include <cstddef>
#include <vector>

#include "rtm3d/model/GridModel2D.hpp"

namespace rtm3d {

/**
 * @brief RTM configuration parameters.
 *
 * Controls the migration process including grid dimensions, time stepping,
 * source frequency, and boundary conditions.
 */
struct RtmConfig {
  std::size_t ny = 32;           ///< Number of grid points in Y (crossline) direction
  float dy = 20.0f;              ///< Grid spacing in Y direction (meters)
  float dt = 0.0015f;            ///< Time step (seconds). Must satisfy CFL condition.
  std::size_t nt = 300;          ///< Number of time steps
  float f0 = 12.0f;              ///< Source peak frequency (Hz)
  std::size_t pml = 10;          ///< PML boundary width (grid points)
  std::size_t receiver_stride = 8;  ///< Receiver spacing (grid points)
};

/**
 * @brief Migration output result.
 *
 * Contains the migrated image as an inline XZ slice extracted from the
 * 3D migration volume at the center Y position.
 */
struct MigrationResult {
  std::size_t nx{};              ///< Number of grid points in X direction
  std::size_t nz{};              ///< Number of grid points in Z direction
  std::vector<float> inline_xz;  ///< Migrated image values (row-major [nz][nx])
};

/**
 * @brief Source position for multi-shot migration.
 *
 * Defines the location of a seismic source in grid coordinates.
 */
struct ShotPosition {
  std::size_t sx;                ///< Source X position (grid index)
  std::size_t sz;                ///< Source Z position (grid index, usually near surface)
};

/**
 * @brief Generate a Ricker wavelet.
 *
 * Creates a Ricker (Mexican hat) wavelet commonly used as a seismic source
 * signature.
 *
 * @param nt Number of time samples
 * @param dt Time step (seconds)
 * @param f0 Peak frequency (Hz)
 * @return Wavelet amplitude values
 */
std::vector<float> ricker_wavelet(std::size_t nt, float dt, float f0);

/**
 * @brief Run single-shot RTM migration.
 *
 * Performs reverse-time migration with a single source position at the
 * center of the model. The source is placed near the surface (z=2).
 *
 * @param model 2D velocity model (extended to 3D internally)
 * @param cfg RTM configuration parameters
 * @return Migration result with inline XZ slice
 * @throws std::runtime_error if configuration is invalid (CFL violation, etc.)
 */
MigrationResult run_single_shot_rtm(const GridModel2D& model, const RtmConfig& cfg);

/**
 * @brief Run multi-shot RTM migration with image stacking.
 *
 * Performs RTM with multiple source positions and stacks the resulting
 * images for improved illumination and signal-to-noise ratio.
 *
 * @param model 2D velocity model (extended to 3D internally)
 * @param cfg RTM configuration parameters
 * @param shots Vector of source positions
 * @return Stacked migration result
 * @throws std::runtime_error if shots vector is empty or config is invalid
 *
 * @example Multi-shot usage:
 * @code
 * std::vector<rtm3d::ShotPosition> shots = {
 *     {.sx = 20, .sz = 2},
 *     {.sx = 40, .sz = 2},
 *     {.sx = 60, .sz = 2}
 * };
 * auto result = rtm3d::run_multi_shot_rtm(model, cfg, shots);
 * @endcode
 */
MigrationResult run_multi_shot_rtm(const GridModel2D& model, const RtmConfig& cfg,
                                   const std::vector<ShotPosition>& shots);

}  // namespace rtm3d
