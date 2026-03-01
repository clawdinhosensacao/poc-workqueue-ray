#include "rtm3d/io/BinaryModelLoader.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace rtm3d {
namespace {

constexpr std::size_t kHeaderSize = sizeof(BinaryVelocityHeader);

void validate_grid_matches_size(const GridSpec& grid, std::size_t file_bytes,
                                const std::string& path) {
  const std::size_t expected = grid.nx * grid.nz * grid.ny * sizeof(float);
  if (file_bytes != expected) {
    throw std::runtime_error("binary velocity size mismatch: expected " +
                             std::to_string(expected) + " bytes, got " +
                             std::to_string(file_bytes) + " in " + path);
  }
}

std::size_t get_file_size(const std::string& path) {
  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    throw std::runtime_error("cannot stat file: " + path);
  }
  return static_cast<std::size_t>(st.st_size);
}

}  // namespace

SeismicModel load_binary_velocity(const std::string& path, const GridSpec& grid) {
  const std::size_t file_size = get_file_size(path);
  validate_grid_matches_size(grid, file_size, path);

  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("cannot open binary velocity file: " + path);
  }

  std::vector<float> vp(grid.nx * grid.nz * grid.ny);
  file.read(reinterpret_cast<char*>(vp.data()),
            static_cast<std::streamsize>(vp.size() * sizeof(float)));

  if (!file) {
    throw std::runtime_error("failed to read binary velocity file: " + path);
  }

  return SeismicModel::from_velocity(vp, grid);
}

SeismicModel load_binary_velocity_with_header(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("cannot open binary velocity file: " + path);
  }

  BinaryVelocityHeader hdr;
  file.read(reinterpret_cast<char*>(&hdr), kHeaderSize);

  if (!file || hdr.magic != 0x56454C33) {
    throw std::runtime_error("invalid binary velocity header in: " + path);
  }

  GridSpec grid;
  grid.nx = hdr.nx;
  grid.nz = hdr.nz;
  grid.ny = hdr.ny;
  grid.dx = hdr.dx;
  grid.dz = hdr.dz;
  grid.dy = hdr.dy;

  std::vector<float> vp(grid.nx * grid.nz * grid.ny);
  file.read(reinterpret_cast<char*>(vp.data()),
            static_cast<std::streamsize>(vp.size() * sizeof(float)));

  if (!file) {
    throw std::runtime_error("failed to read binary velocity data: " + path);
  }

  return SeismicModel::from_velocity(vp, grid);
}

void write_binary_velocity(const std::string& path, const SeismicModel& model,
                           bool include_header) {
  std::ofstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("cannot write binary velocity file: " + path);
  }

  const auto& grid = model.grid();
  const auto& vp = model.velocity();

  if (include_header) {
    BinaryVelocityHeader hdr;
    hdr.nx = static_cast<std::uint32_t>(grid.nx);
    hdr.nz = static_cast<std::uint32_t>(grid.nz);
    hdr.ny = static_cast<std::uint32_t>(grid.ny);
    hdr.dx = grid.dx;
    hdr.dz = grid.dz;
    hdr.dy = grid.dy;

    file.write(reinterpret_cast<const char*>(&hdr), kHeaderSize);
  }

  file.write(reinterpret_cast<const char*>(vp.data()),
             static_cast<std::streamsize>(vp.size() * sizeof(float)));

  if (!file) {
    throw std::runtime_error("failed to write binary velocity file: " + path);
  }
}

// --- MappedVelocityModel implementation ---

MappedVelocityModel::MappedVelocityModel(const std::string& path, const GridSpec& grid)
    : grid_(grid), data_(nullptr), mapping_(nullptr), mapping_size_(0), fd_(-1) {
  fd_ = open(path.c_str(), O_RDONLY);
  if (fd_ < 0) {
    throw std::runtime_error("cannot open file for mmap: " + path);
  }

  const std::size_t file_size = get_file_size(path);
  validate_grid_matches_size(grid, file_size, path);

  mapping_size_ = file_size;
  mapping_ = mmap(nullptr, mapping_size_, PROT_READ, MAP_PRIVATE, fd_, 0);

  if (mapping_ == MAP_FAILED) {
    close(fd_);
    throw std::runtime_error("mmap failed for: " + path);
  }

  data_ = static_cast<float*>(mapping_);
}

MappedVelocityModel::~MappedVelocityModel() {
  if (mapping_ && mapping_ != MAP_FAILED) {
    munmap(mapping_, mapping_size_);
  }
  if (fd_ >= 0) {
    close(fd_);
  }
}

MappedVelocityModel::MappedVelocityModel(MappedVelocityModel&& other) noexcept
    : grid_(other.grid_),
      data_(other.data_),
      mapping_(other.mapping_),
      mapping_size_(other.mapping_size_),
      fd_(other.fd_) {
  other.data_ = nullptr;
  other.mapping_ = nullptr;
  other.mapping_size_ = 0;
  other.fd_ = -1;
}

MappedVelocityModel& MappedVelocityModel::operator=(MappedVelocityModel&& other) noexcept {
  if (this != &other) {
    if (mapping_ && mapping_ != MAP_FAILED) {
      munmap(mapping_, mapping_size_);
    }
    if (fd_ >= 0) {
      close(fd_);
    }

    grid_ = other.grid_;
    data_ = other.data_;
    mapping_ = other.mapping_;
    mapping_size_ = other.mapping_size_;
    fd_ = other.fd_;

    other.data_ = nullptr;
    other.mapping_ = nullptr;
    other.mapping_size_ = 0;
    other.fd_ = -1;
  }
  return *this;
}

std::unique_ptr<MappedVelocityModel> mmap_binary_velocity(const std::string& path,
                                                          const GridSpec& grid) {
  return std::make_unique<MappedVelocityModel>(path, grid);
}

}  // namespace rtm3d
