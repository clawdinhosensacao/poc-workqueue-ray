/**
 * @file BinaryModelLoader.hpp
 * @brief High-performance binary velocity model loading with memory-mapped I/O.
 *
 * Provides efficient loading of large velocity models from binary files.
 * Uses memory-mapped I/O for maximum performance on large datasets.
 *
 * Binary format:
 * - Header (optional JSON sidecar: .json)
 * - Data: float32 values in row-major order [nz][nx][ny]
 *
 * @example Loading a velocity model:
 * @code
 * #include "rtm3d/io/BinaryModelLoader.hpp"
 *
 * // Simple load (reads entire file into memory)
 * auto model = rtm3d::load_binary_velocity("velocity.bin", grid);
 *
 * // Memory-mapped load (zero-copy, OS-managed)
 * auto mmap_model = rtm3d::mmap_binary_velocity("velocity.bin", grid);
 * @endcode
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rtm3d/model/SeismicModel.hpp"

namespace rtm3d {

/**
 * @brief Binary file header for velocity models.
 *
 * Optional header embedded at the start of binary files.
 * If not present, dimensions are read from sidecar JSON.
 */
struct BinaryVelocityHeader {
  std::uint32_t magic = 0x56454C33;  // "VEL3" in little-endian
  std::uint32_t version = 1;
  std::uint32_t nx = 0;
  std::uint32_t nz = 0;
  std::uint32_t ny = 1;
  float dx = 10.0f;
  float dz = 10.0f;
  float dy = 10.0f;
  std::uint32_t reserved[8] = {0};
};

/**
 * @brief Load a velocity model from a binary float32 file.
 *
 * Reads the entire file into memory. Simple and reliable.
 *
 * @param path Path to binary file (float32, row-major [nz][nx][ny])
 * @param grid Grid specification (must match file dimensions)
 * @return SeismicModel with loaded velocity field
 * @throws std::runtime_error on file errors or size mismatch
 */
SeismicModel load_binary_velocity(const std::string& path, const GridSpec& grid);

/**
 * @brief Load a velocity model from a binary file with header.
 *
 * Reads dimensions from the embedded header.
 *
 * @param path Path to binary file with BinaryVelocityHeader
 * @return SeismicModel with loaded velocity field
 * @throws std::runtime_error on file errors or invalid header
 */
SeismicModel load_binary_velocity_with_header(const std::string& path);

/**
 * @brief Write a velocity model to a binary file.
 *
 * @param path Output path
 * @param model SeismicModel to write
 * @param include_header If true, prepend BinaryVelocityHeader
 * @throws std::runtime_error on write errors
 */
void write_binary_velocity(const std::string& path, const SeismicModel& model,
                           bool include_header = true);

/**
 * @brief Memory-mapped velocity model view.
 *
 * Provides zero-copy access to a memory-mapped binary velocity file.
 * The OS handles paging; no explicit loading required.
 *
 * @note The mapped memory remains valid as long as this object exists.
 */
class MappedVelocityModel {
public:
  /**
   * @brief Map a binary velocity file into memory.
   *
   * @param path Path to binary file
   * @param grid Grid specification (must match file dimensions)
   * @throws std::runtime_error on mapping errors
   */
  MappedVelocityModel(const std::string& path, const GridSpec& grid);

  /// Unmap and close the file
  ~MappedVelocityModel();

  // Non-copyable, movable
  MappedVelocityModel(const MappedVelocityModel&) = delete;
  MappedVelocityModel& operator=(const MappedVelocityModel&) = delete;
  MappedVelocityModel(MappedVelocityModel&&) noexcept;
  MappedVelocityModel& operator=(MappedVelocityModel&&) noexcept;

  /// Get pointer to velocity data (float32, row-major)
  const float* data() const { return data_; }

  /// Get element at (i, k, j) indices
  float at(std::size_t i, std::size_t k, std::size_t j = 0) const {
    return data_[j * grid_.nz * grid_.nx + k * grid_.nx + i];
  }

  /// Get grid specification
  const GridSpec& grid() const { return grid_; }

  /// Get total number of velocity values
  std::size_t size() const { return grid_.nx * grid_.nz * grid_.ny; }

  /// Get size in bytes
  std::size_t size_bytes() const { return size() * sizeof(float); }

private:
  GridSpec grid_;
  float* data_;
  void* mapping_;
  std::size_t mapping_size_;
  int fd_;
};

/**
 * @brief Create a memory-mapped velocity model.
 *
 * Convenience function for creating MappedVelocityModel.
 *
 * @param path Path to binary file
 * @param grid Grid specification
 * @return Unique pointer to mapped model
 */
std::unique_ptr<MappedVelocityModel> mmap_binary_velocity(const std::string& path,
                                                          const GridSpec& grid);

}  // namespace rtm3d
