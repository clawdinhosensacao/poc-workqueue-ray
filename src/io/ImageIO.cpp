#include "rtm3d/io/ImageIO.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <numeric>
#include <stdexcept>

namespace rtm3d {

static void validate_shape(const std::vector<float>& image, std::size_t nx, std::size_t nz) {
  if (nx == 0 || nz == 0) throw std::runtime_error("invalid image shape");
  if (image.size() != nx * nz) throw std::runtime_error("image size mismatch");
}

void write_pgm(const std::string& path, const std::vector<float>& image, std::size_t nx,
               std::size_t nz) {
  validate_shape(image, nx, nz);

  // Find maximum absolute value for normalization
  float max_abs = std::accumulate(image.begin(), image.end(), 1e-9f,
                                   [](float m, float v) { return std::max(m, std::abs(v)); });

  // Normalize to buffer first (avoids per-byte write calls)
  std::vector<unsigned char> buffer(nx * nz);
  const float scale = 255.0f / max_abs;
  for (std::size_t i = 0; i < image.size(); ++i) {
    const float n = 0.5f + 0.5f * image[i];
    buffer[i] = static_cast<unsigned char>(std::clamp(n * scale, 0.0f, 255.0f));
  }

  std::ofstream f(path, std::ios::binary);
  if (!f) throw std::runtime_error("cannot write image: " + path);

  f << "P5\n" << nx << " " << nz << "\n255\n";
  f.write(reinterpret_cast<const char*>(buffer.data()),
          static_cast<std::streamsize>(buffer.size()));
}

void write_float32_raw(const std::string& path, const std::vector<float>& image,
                       std::size_t nx, std::size_t nz) {
  validate_shape(image, nx, nz);

  std::ofstream f(path, std::ios::binary);
  if (!f) throw std::runtime_error("cannot write image: " + path);
  f.write(reinterpret_cast<const char*>(image.data()),
          static_cast<std::streamsize>(image.size() * sizeof(float)));

  const std::string hdr_path = path + ".json";
  std::ofstream h(hdr_path);
  if (!h) throw std::runtime_error("cannot write header: " + hdr_path);
  h << "{\n"
    << "  \"dtype\": \"float32\",\n"
    << "  \"layout\": \"row-major [nz][nx]\",\n"
    << "  \"nx\": " << nx << ",\n"
    << "  \"nz\": " << nz << "\n"
    << "}\n";
}

}  // namespace rtm3d
