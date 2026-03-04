#include "Wavelet.hpp"

#include <cmath>
#include <stdexcept>

namespace rtm3d::rtm_internal {

std::vector<float> ricker_wavelet(std::size_t nt, float dt, float f0) {
  if (nt < 2 || dt <= 0.0f || f0 <= 0.0f) throw std::runtime_error("invalid wavelet arguments");

  std::vector<float> w(nt, 0.0f);
  const float t0 = 1.0f / f0;
  constexpr float kPi = 3.14159265358979323846f;
  for (std::size_t it = 0; it < nt; ++it) {
    const float t = static_cast<float>(it) * dt - t0;
    const float a = kPi * f0 * t;
    const float a2 = a * a;
    w[it] = (1.0f - 2.0f * a2) * std::exp(-a2);
  }
  return w;
}

}  // namespace rtm3d::rtm_internal
