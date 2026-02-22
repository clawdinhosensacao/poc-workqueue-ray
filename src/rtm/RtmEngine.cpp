#include "rtm3d/rtm/RtmEngine.hpp"

#include <algorithm>
#include <stdexcept>

#include "Acquisition.hpp"
#include "Boundary.hpp"
#include "Geometry.hpp"
#include "Imaging.hpp"
#include "Propagation.hpp"
#include "Validation.hpp"
#include "Wavelet.hpp"
#include "rtm3d/core/Volume3D.hpp"

namespace rtm3d {
namespace {

void forward_source_propagation(const GridModel2D& model, const RtmConfig& cfg, const Volume3D& vel,
                                const std::vector<float>& damp, const std::vector<float>& wavelet,
                                std::size_t sx, std::size_t sy, std::size_t sz,
                                const std::vector<std::size_t>& rx, std::vector<float>& src_snaps,
                                std::vector<float>& rec_data) {
  const auto n = vel.size();
  std::vector<float> src_prev(n, 0.0f), src_cur(n, 0.0f), src_nxt(n, 0.0f);

  for (std::size_t it = 0; it < cfg.nt; ++it) {
    rtm_internal::step_fd3d(vel, damp, cfg.dt, model.dx, cfg.dy, model.dz, src_prev, src_cur, src_nxt);
    src_nxt[vel.index(sx, sy, sz)] += wavelet[it];

    rtm_internal::record_receivers(vel, sy, sz, rx, src_nxt, rec_data, it);
    std::copy(src_nxt.begin(), src_nxt.end(), src_snaps.begin() + it * n);

    src_prev.swap(src_cur);
    src_cur.swap(src_nxt);
  }
}

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
    rtm_internal::step_fd3d(vel, damp, cfg.dt, model.dx, cfg.dy, model.dz, rec_prev, rec_cur, rec_nxt);

    rtm_internal::inject_receivers(vel, sy, sz, rx, rec_data, it, rec_nxt);

    const auto* src = src_snaps.data() + it * n;
    rtm_internal::accumulate_cross_correlation_image(src, rec_nxt, image);

    rec_prev.swap(rec_cur);
    rec_cur.swap(rec_nxt);
  }
}

}  // namespace

std::vector<float> ricker_wavelet(std::size_t nt, float dt, float f0) {
  return rtm_internal::ricker_wavelet(nt, dt, f0);
}

MigrationResult run_single_shot_rtm(const GridModel2D& model, const RtmConfig& cfg) {
  rtm_internal::validate_cfg(model, cfg);

  const Volume3D vel = rtm_internal::make_velocity_volume(model, cfg);
  const auto n = vel.size();

  const auto damp = rtm_internal::make_damp(vel.nx(), vel.ny(), vel.nz(), cfg.pml);
  const auto wavelet = ricker_wavelet(cfg.nt, cfg.dt, cfg.f0);

  const auto shot = rtm_internal::make_default_shot_geometry(vel, cfg.receiver_stride);
  std::vector<float> src_snaps(cfg.nt * n, 0.0f);
  std::vector<float> rec_data(cfg.nt * shot.rx.size(), 0.0f);

  forward_source_propagation(model, cfg, vel, damp, wavelet, shot.sx, shot.sy, shot.sz, shot.rx,
                             src_snaps, rec_data);

  std::vector<float> image(n, 0.0f);
  receiver_backpropagation_and_imaging(model, cfg, vel, damp, shot.sy, shot.sz, shot.rx, src_snaps,
                                       rec_data, image);

  MigrationResult out;
  out.nx = vel.nx();
  out.nz = vel.nz();
  out.inline_xz = rtm_internal::extract_inline_xz(vel, image);
  return out;
}

}  // namespace rtm3d
