#include "ReceiverImaging.hpp"

#include "Geometry.hpp"
#include "Imaging.hpp"
#include "Propagation.hpp"

namespace rtm3d::rtm_internal {

void receiver_backpropagation_and_imaging(const GridModel2D& model, const RtmConfig& cfg,
                                          const Volume3D& vel, const std::vector<float>& damp,
                                          std::size_t sy, std::size_t sz,
                                          const std::vector<std::size_t>& rx,
                                          const std::vector<float>& src_snaps,
                                          const std::vector<float>& rec_data,
                                          std::vector<float>& image) {
  const auto n = vel.size();
  std::vector<float> rec_prev(n, 0.0f), rec_cur(n, 0.0f), rec_nxt(n, 0.0f);

  for (std::size_t rit = 0; rit < cfg.nt; ++rit) {
    const std::size_t it = cfg.nt - 1 - rit;
    step_fd3d(vel, damp, cfg.dt, model.dx, cfg.dy, model.dz, rec_prev, rec_cur, rec_nxt);

    inject_receivers(vel, sy, sz, rx, rec_data, it, rec_nxt);

    const auto* src = src_snaps.data() + it * n;
    accumulate_cross_correlation_image(src, rec_nxt, image);

    rec_prev.swap(rec_cur);
    rec_cur.swap(rec_nxt);
  }
}

}  // namespace rtm3d::rtm_internal
