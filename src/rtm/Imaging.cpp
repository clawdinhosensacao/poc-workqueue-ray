#include "Imaging.hpp"

namespace rtm3d::rtm_internal {

void accumulate_cross_correlation_image(const float* src, const std::vector<float>& rec_field,
                                        std::vector<float>& image) {
  const auto n = image.size();
  for (std::size_t i = 0; i < n; ++i) {
    image[i] += src[i] * rec_field[i];
  }
}

}  // namespace rtm3d::rtm_internal
