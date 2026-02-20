#include "rtm3d/rtm/RtmEngine.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "rtm3d/core/Volume3D.hpp"

namespace rtm3d {
namespace {

void validate_cfg(const GridModel2D& model, const RtmConfig& cfg) {
  if (model.nx < 8 || model.nz < 8) throw std::runtime_error("model too small");
  if (model.dx <= 0.0f || model.dz <= 0.0f) throw std::runtime_error("invalid model spacing");
  if (cfg.ny < 4 || cfg.nt < 2) throw std::runtime_error("ny/nt too small");
  if (cfg.dy <= 0.0f || cfg.dt <= 0.0f || cfg.f0 <= 0.0f) throw std::runtime_error("invalid RTM scalar parameter");
  if (cfg.receiver_stride == 0) throw std::runtime_error("receiver_stride must be > 0");
  if (cfg.pml == 0) throw std::runtime_error("pml must be > 0");
}

void step_fd3d(const Volume3D& vel, const std::vector<float>& damp, float dt, float dx, float dy,
               float dz, const std::vector<float>& prev, const std::vector<float>& cur,
               std::vector<float>& nxt) {
  std::fill(nxt.begin(), nxt.end(), 0.0f);

  for (std::size_t iz = 1; iz + 1 < vel.nz(); ++iz) {
    for (std::size_t iy = 1; iy + 1 < vel.ny(); ++iy) {
      for (std::size_t ix = 1; ix + 1 < vel.nx(); ++ix) {
        const auto i = vel.index(ix, iy, iz);
        const float d2x = (cur[vel.index(ix + 1, iy, iz)] - 2.0f * cur[i] + cur[vel.index(ix - 1, iy, iz)]) / (dx * dx);
        const float d2y = (cur[vel.index(ix, iy + 1, iz)] - 2.0f * cur[i] + cur[vel.index(ix, iy - 1, iz)]) / (dy * dy);
        const float d2z = (cur[vel.index(ix, iy, iz + 1)] - 2.0f * cur[i] + cur[vel.index(ix, iy, iz - 1)]) / (dz * dz);
        const float lap = d2x + d2y + d2z;
        const float v = vel.raw()[i];
        nxt[i] = (2.0f * cur[i] - prev[i] + (v * v) * (dt * dt) * lap) * damp[i];
      }
    }
  }
}

std::vector<float> make_damp(std::size_t nx, std::size_t ny, std::size_t nz, std::size_t pml) {
  std::vector<float> d(nx * ny * nz, 1.0f);
  for (std::size_t iz = 0; iz < nz; ++iz) {
    for (std::size_t iy = 0; iy < ny; ++iy) {
      for (std::size_t ix = 0; ix < nx; ++ix) {
        const auto dist = std::min({ix, nx - 1 - ix, iy, ny - 1 - iy, iz, nz - 1 - iz});
        float coeff = 1.0f;
        if (dist < pml) {
          const float x = static_cast<float>(pml - dist) / static_cast<float>(pml);
          coeff = std::exp(-0.03f * x * x);
        }
        d[(iz * ny + iy) * nx + ix] = coeff;
      }
    }
  }
  return d;
}

}  // namespace

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

MigrationResult run_single_shot_rtm(const GridModel2D& model, const RtmConfig& cfg) {
  validate_cfg(model, cfg);

  Volume3D vel(model.nx, cfg.ny, model.nz, 1500.0f);
  for (std::size_t iz = 0; iz < model.nz; ++iz) {
    for (std::size_t iy = 0; iy < cfg.ny; ++iy) {
      for (std::size_t ix = 0; ix < model.nx; ++ix) {
        vel(ix, iy, iz) = model.values[iz * model.nx + ix];
      }
    }
  }

  const auto n = vel.size();
  const auto damp = make_damp(vel.nx(), vel.ny(), vel.nz(), cfg.pml);

  std::vector<float> src_prev(n, 0.0f), src_cur(n, 0.0f), src_nxt(n, 0.0f);
  std::vector<float> rec_prev(n, 0.0f), rec_cur(n, 0.0f), rec_nxt(n, 0.0f);

  std::vector<float> src_snaps(cfg.nt * n, 0.0f);
  const auto wavelet = ricker_wavelet(cfg.nt, cfg.dt, cfg.f0);

  const std::size_t sx = vel.nx() / 2;
  const std::size_t sy = vel.ny() / 2;
  const std::size_t sz = 2;

  const std::size_t nrec = std::max<std::size_t>(2, vel.nx() / cfg.receiver_stride);
  std::vector<float> rec_data(cfg.nt * nrec, 0.0f);

  for (std::size_t it = 0; it < cfg.nt; ++it) {
    step_fd3d(vel, damp, cfg.dt, model.dx, cfg.dy, model.dz, src_prev, src_cur, src_nxt);
    src_nxt[vel.index(sx, sy, sz)] += wavelet[it];

    for (std::size_t ir = 0; ir < nrec; ++ir) {
      const std::size_t rx = std::min(1 + ir * cfg.receiver_stride, vel.nx() - 2);
      rec_data[it * nrec + ir] = src_nxt[vel.index(rx, sy, sz)];
    }

    std::copy(src_nxt.begin(), src_nxt.end(), src_snaps.begin() + it * n);
    src_prev.swap(src_cur);
    src_cur.swap(src_nxt);
  }

  std::vector<float> image(n, 0.0f);
  for (std::size_t rit = 0; rit < cfg.nt; ++rit) {
    const std::size_t it = cfg.nt - 1 - rit;
    step_fd3d(vel, damp, cfg.dt, model.dx, cfg.dy, model.dz, rec_prev, rec_cur, rec_nxt);

    for (std::size_t ir = 0; ir < nrec; ++ir) {
      const std::size_t rx = std::min(1 + ir * cfg.receiver_stride, vel.nx() - 2);
      rec_nxt[vel.index(rx, sy, sz)] += rec_data[it * nrec + ir];
    }

    const auto* src = src_snaps.data() + it * n;
    for (std::size_t i = 0; i < n; ++i) image[i] += src[i] * rec_nxt[i];

    rec_prev.swap(rec_cur);
    rec_cur.swap(rec_nxt);
  }

  MigrationResult out;
  out.nx = vel.nx();
  out.nz = vel.nz();
  out.inline_xz.resize(out.nx * out.nz, 0.0f);
  const std::size_t ymid = vel.ny() / 2;
  for (std::size_t iz = 0; iz < out.nz; ++iz) {
    for (std::size_t ix = 0; ix < out.nx; ++ix) out.inline_xz[iz * out.nx + ix] = image[vel.index(ix, ymid, iz)];
  }
  return out;
}

}  // namespace rtm3d
