#pragma once

/**
 * @file SeismicModel.hpp
 * @brief Devito-inspired seismic velocity model with preset scenarios.
 *
 * This module provides a clean API for creating and managing velocity models
 * for seismic imaging. The design follows Devito's SeismicModel pattern with
 * preset model types for quick benchmarking and testing.
 *
 * @example Creating a layered model:
 * @code
 * #include "rtm3d/model/SeismicModel.hpp"
 *
 * rtm3d::GridSpec grid{.nx = 100, .nz = 60, .ny = 1, .dx = 10.0f, .dz = 10.0f};
 * auto model = rtm3d::SeismicModel::from_preset(
 *     rtm3d::ModelPreset::Layers, grid, 1500.0f, 3500.0f, 3);
 *
 * // Access velocity field
 * const auto& vel = model.velocity();
 * std::cout << "v_min=" << model.min_velocity() << " v_max=" << model.max_velocity() << "\n";
 * @endcode
 */

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace rtm3d {

/**
 * @brief Time axis specification for wave propagation.
 *
 * Defines the temporal discretization for RTM simulations.
 */
struct TimeAxis {
  float start = 0.0f;           ///< Start time (seconds)
  float step = 0.001f;          ///< Time step dt (seconds)
  std::size_t num = 100;        ///< Number of time steps (nt)

  /// Calculate stop time
  float stop() const { return step * static_cast<float>(num - 1) + start; }
};

/**
 * @brief Grid specification for seismic models.
 *
 * Defines the spatial discretization of the velocity model.
 */
struct GridSpec {
  std::size_t nx = 101;         ///< Grid points in X (inline) direction
  std::size_t nz = 101;         ///< Grid points in Z (depth) direction
  std::size_t ny = 1;           ///< Grid points in Y (crossline) direction (1 for 2D)
  float dx = 10.0f;             ///< Grid spacing in X (meters)
  float dz = 10.0f;             ///< Grid spacing in Z (meters)
  float dy = 10.0f;             ///< Grid spacing in Y (meters)
  std::size_t nbl = 10;         ///< Number of boundary layers (PML width)
};

/**
 * @brief Source wavelet types.
 *
 * Supported source wavelet functions for seismic modeling.
 */
enum class WaveletType {
  Ricker,   ///< Ricker (Mexican hat) wavelet - most common
  Gabor,    ///< Gabor wavelet
  DGauss    ///< Derivative of Gaussian
};

/**
 * @brief Source configuration for seismic acquisition.
 */
struct SourceConfig {
  std::size_t sx = 0;           ///< Source X position (grid index)
  std::size_t sz = 2;           ///< Source Z position (grid index, near surface)
  std::size_t sy = 0;           ///< Source Y position (grid index, for 3D)
  WaveletType wavelet = WaveletType::Ricker;  ///< Wavelet type
  float f0 = 16.0f;             ///< Peak frequency (Hz)
  float delay = 0.0f;           ///< Firing delay (seconds)
};

/**
 * @brief Receiver configuration for seismic acquisition.
 */
struct ReceiverConfig {
  std::size_t rz = 2;           ///< Receiver depth (grid index)
  std::size_t ryo = 0;          ///< Receiver Y offset (grid index)
  std::size_t first_rx = 2;     ///< First receiver X position (grid index)
  std::size_t last_rx = 0;      ///< Last receiver X position (0 = nx-2)
  std::size_t stride = 1;       ///< Receiver spacing (grid points)
};

/**
 * @brief Preset velocity model types (Devito-inspired).
 *
 * Each preset creates a different geological scenario for testing
 * and benchmarking RTM algorithms.
 */
enum class ModelPreset {
  Constant,     ///< Uniform velocity field (simple testing)
  Layers,       ///< N-layer model with velocity gradient
  Circle,       ///< Circular anomaly (camembert model)
  Marmousi2D,   ///< Marmousi benchmark (requires external data)
  SaltDome,     ///< Salt dome structure
  CircleLens,   ///< Gaussian lens anomaly
  Fault         ///< Normal fault with offset layers
};

/**
 * @brief Seismic velocity model with grid specification.
 *
 * Manages a 3D velocity field with associated grid parameters.
 * Provides factory methods for creating models from presets,
 * raw data, or files.
 *
 * @note Memory layout is row-major [nz][nx] for 2D slices.
 */
class SeismicModel {
public:
  /**
   * @brief Create a velocity model from a preset.
   *
   * Factory method to create common geological scenarios for testing.
   *
   * @param preset Model type to create
   * @param grid Grid specification
   * @param vp_top Top layer velocity (m/s) for layered models
   * @param vp_bottom Bottom layer velocity (m/s) for layered models
   * @param nlayers Number of layers for layered models
   * @return SeismicModel with populated velocity field
   * @throws std::runtime_error if preset requires external data
   */
  static SeismicModel from_preset(ModelPreset preset, const GridSpec& grid,
                                   float vp_top = 1500.0f, float vp_bottom = 3500.0f,
                                   std::size_t nlayers = 3);

  /**
   * @brief Create a model from a raw velocity array.
   *
   * @param vp Velocity values (m/s), size must match grid.nx * grid.nz * grid.ny
   * @param grid Grid specification
   * @return SeismicModel with the provided velocity field
   * @throws std::runtime_error if array size doesn't match grid dimensions
   */
  static SeismicModel from_velocity(const std::vector<float>& vp,
                                     const GridSpec& grid);

  /**
   * @brief Load a velocity model from a binary file.
   *
   * @param path Path to float32 binary file
   * @param grid Grid specification (must match file dimensions)
   * @return SeismicModel with loaded velocity field
   * @throws std::runtime_error if file cannot be read or size mismatches
   */
  static SeismicModel from_file(const std::string& path, const GridSpec& grid);

  /// Get read-only access to velocity field
  const std::vector<float>& velocity() const { return vp_; }

  /// Get read-write access to velocity field
  std::vector<float>& velocity() { return vp_; }

  /// Get grid specification
  const GridSpec& grid() const { return grid_; }

  /// Get maximum velocity value
  float max_velocity() const;

  /// Get minimum velocity value
  float min_velocity() const;

  /**
   * @brief Validate model for RTM simulation.
   *
   * Checks CFL stability condition and other constraints.
   *
   * @param time Time axis specification
   * @throws std::runtime_error if validation fails
   */
  void validate_for_rtm(const TimeAxis& time) const;

private:
  SeismicModel(const GridSpec& grid) : grid_(grid) {}
  GridSpec grid_;
  std::vector<float> vp_;
};

}  // namespace rtm3d
