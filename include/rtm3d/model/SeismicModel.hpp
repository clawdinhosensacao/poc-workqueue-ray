#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace rtm3d {

/// Time axis specification (Devito-inspired)
struct TimeAxis {
  float start = 0.0f;
  float step = 0.001f;    // dt in seconds
  std::size_t num = 100;  // nt

  float stop() const { return step * static_cast<float>(num - 1) + start; }
};

/// Grid specification for seismic models
struct GridSpec {
  std::size_t nx = 101;
  std::size_t nz = 101;
  std::size_t ny = 1;     // 1 for 2D, >1 for 3D
  float dx = 10.0f;       // meters
  float dz = 10.0f;
  float dy = 10.0f;
  std::size_t nbl = 10;   // boundary layers (PML width)
};

/// Source wavelet types (Devito-inspired)
enum class WaveletType { Ricker, Gabor, DGauss };

/// Source configuration
struct SourceConfig {
  std::size_t sx = 0;           // source x index
  std::size_t sz = 2;           // source z index (usually near surface)
  std::size_t sy = 0;           // source y index (for 3D)
  WaveletType wavelet = WaveletType::Ricker;
  float f0 = 16.0f;             // peak frequency (Hz)
  float delay = 0.0f;           // firing delay (seconds)
};

/// Receiver configuration
struct ReceiverConfig {
  std::size_t rz = 2;           // receiver depth index
  std::size_t ryo = 0;          // receiver y offset (for 3D)
  std::size_t first_rx = 2;     // first receiver x index
  std::size_t last_rx = 0;      // last receiver x index (0 = nx-2)
  std::size_t stride = 1;       // receiver spacing in grid points
};

/// Preset model types (Devito-inspired)
enum class ModelPreset {
  Constant,       // uniform velocity
  Layers,         // n-layer model with velocity gradient
  Circle,         // circular anomaly (camembert)
  Marmousi2D,     // Marmousi benchmark (requires data file)
  SaltDome,       // salt dome structure
  CircleLens      // circular lens anomaly
};

/// Seismic model with velocity and grid (Devito-inspired SeismicModel)
class SeismicModel {
public:
  /// Create from preset
  static SeismicModel from_preset(ModelPreset preset, const GridSpec& grid,
                                   float vp_top = 1500.0f, float vp_bottom = 3500.0f,
                                   std::size_t nlayers = 3);

  /// Create from raw velocity array
  static SeismicModel from_velocity(const std::vector<float>& vp,
                                     const GridSpec& grid);

  /// Create by loading from file
  static SeismicModel from_file(const std::string& path, const GridSpec& grid);

  // Accessors
  const std::vector<float>& velocity() const { return vp_; }
  std::vector<float>& velocity() { return vp_; }
  const GridSpec& grid() const { return grid_; }
  float max_velocity() const;
  float min_velocity() const;

  /// Validate model for RTM (CFL check, etc.)
  void validate_for_rtm(const TimeAxis& time) const;

private:
  SeismicModel(const GridSpec& grid) : grid_(grid) {}
  GridSpec grid_;
  std::vector<float> vp_;
};

}  // namespace rtm3d
