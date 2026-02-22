#include "SourcePropagation.hpp"

#include <algorithm>

#include "Geometry.hpp"
#include "Propagation.hpp"

namespace rtm3d::rtm_internal {

void forward_source_propagation(const GridModel2D& model, const RtmConfig& cfg, const Volume3D& vel,
                                const std::vector<float>& damp, const std::vector<float>& wavelet,
                                std::size_t sx, std::size_t sy, std::size_t sz,
                                const std::vector<std::size_t>& rx, std::vector<float>& src_snaps,
                                std::vector<float>& rec_data) {
  const auto n = vel.size();
  std::vector<float> src_prev(n, 0.0f), src_cur(n, 0.0f), src_nxt(n, 0.0f);

  for (std::size_t it = 0; it < cfg.nt; ++it) {
    step_fd3d(vel, damp, cfg.dt, model.dx, cfg.dy, model.dz, src_prev, src_cur, src_nxt);
    src_nxt[vel.index(sx, sy, sz)] += wavelet[it];

    record_receivers(vel, sy, sz, rx, src_nxt, rec_data, it);
    std::copy(src_nxt.begin(), src_nxt.end(), src_snaps.begin() + it * n);

    src_prev.swap(src_cur);
    src_cur.swap(src_nxt);
  }
}

}  // namespace rtm3d::rtm_internal
