#include "rtm3d/io/ImageIO.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace rtm3d {

static void validate_shape(const std::vector<float>& image, std::size_t nx, std::size_t nz) {
  if (nx == 0 || nz == 0) throw std::runtime_error("invalid image shape");
  if (image.size() != nx * nz) throw std::runtime_error("image size mismatch");
}

void write_pgm(const std::string& path, const std::vector<float>& image, std::size_t nx,
               std::size_t nz) {
  validate_shape(image, nx, nz);

  float max_abs = 1e-9f;
  for (float v : image) max_abs = std::max(max_abs, std::abs(v));

  std::ofstream f(path, std::ios::binary);
  if (!f) throw std::runtime_error("cannot write image: " + path);

  f << "P5\n" << nx << " " << nz << "\n255\n";
  for (float v : image) {
    const float n = 0.5f + 0.5f * (v / max_abs);
    const auto c = static_cast<unsigned char>(std::clamp(n, 0.0f, 1.0f) * 255.0f);
    f.write(reinterpret_cast<const char*>(&c), 1);
  }
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
