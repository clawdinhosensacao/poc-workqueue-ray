#include "rtm3d/rtm/RtmEngine.hpp"

#include "Acquisition.hpp"
#include "Boundary.hpp"
#include "Geometry.hpp"
#include "ReceiverImaging.hpp"
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

}  // namespace rtm3d
