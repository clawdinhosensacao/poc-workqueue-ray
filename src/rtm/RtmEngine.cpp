#include "rtm3d/rtm/RtmEngine.hpp"

#include <stdexcept>

#include "Acquisition.hpp"
#include "Boundary.hpp"
#include "Geometry.hpp"
#include "ReceiverImaging.hpp"
#include "Receivers.hpp"
#include "ResultBuilder.hpp"
#include "SourcePropagation.hpp"
#include "Validation.hpp"
#include "Wavelet.hpp"
#include "rtm3d/core/Volume3D.hpp"

namespace rtm3d {

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

  rtm_internal::forward_source_propagation(model, cfg, vel, damp, wavelet, shot.sx, shot.sy,
                                           shot.sz, shot.rx, src_snaps, rec_data);

  std::vector<float> image(n, 0.0f);
  rtm_internal::receiver_backpropagation_and_imaging(model, cfg, vel, damp, shot.sy, shot.sz,
                                                     shot.rx, src_snaps, rec_data, image);

  return rtm_internal::build_migration_result(vel, image);
}

MigrationResult run_multi_shot_rtm(const GridModel2D& model, const RtmConfig& cfg,
                                   const std::vector<ShotPosition>& shots) {
  rtm_internal::validate_cfg(model, cfg);

  if (shots.empty()) {
    throw std::runtime_error("shots vector must not be empty");
  }

  const Volume3D vel = rtm_internal::make_velocity_volume(model, cfg);
  const auto n = vel.size();

  const auto damp = rtm_internal::make_damp(vel.nx(), vel.ny(), vel.nz(), cfg.pml);
  const auto wavelet = ricker_wavelet(cfg.nt, cfg.dt, cfg.f0);

  const std::size_t sy = vel.ny() / 2;
  const auto rx = rtm_internal::make_receiver_positions(vel, cfg.receiver_stride);

  std::vector<float> stacked_image(n, 0.0f);
  std::vector<float> src_snaps(cfg.nt * n, 0.0f);
  std::vector<float> rec_data(cfg.nt * rx.size(), 0.0f);

  for (const auto& shot : shots) {
    std::fill(src_snaps.begin(), src_snaps.end(), 0.0f);
    std::fill(rec_data.begin(), rec_data.end(), 0.0f);

    rtm_internal::forward_source_propagation(model, cfg, vel, damp, wavelet, shot.sx, sy,
                                             shot.sz, rx, src_snaps, rec_data);

    rtm_internal::receiver_backpropagation_and_imaging(model, cfg, vel, damp, sy, shot.sz,
                                                       rx, src_snaps, rec_data, stacked_image);
  }

  return rtm_internal::build_migration_result(vel, stacked_image);
}

}  // namespace rtm3d
