#include <cmath>

#include <gtest/gtest.h>

#include "rtm3d/core/Volume3D.hpp"
#include "rtm3d/model/GridModel2D.hpp"
#include "rtm3d/rtm/RtmEngine.hpp"
#include "../src/rtm/ReceiverImaging.hpp"
#include "../src/rtm/Receivers.hpp"
#include "../src/rtm/SourcePropagation.hpp"

TEST(RtmPropagationPipeline, ForwardAndBackpropProduceImageEnergy) {
  rtm3d::GridModel2D model;
  model.nx = 16;
  model.nz = 12;
  model.dx = 10.0f;
  model.dz = 10.0f;
  model.values = std::vector<float>(model.nx * model.nz, 1800.0f);

  rtm3d::RtmConfig cfg;
  cfg.ny = 8;
  cfg.nt = 20;
  cfg.dt = 0.001f;
  cfg.receiver_stride = 3;

  rtm3d::Volume3D vel(model.nx, cfg.ny, model.nz, 1800.0f);
  std::vector<float> damp(vel.size(), 1.0f);
  std::vector<float> wavelet(cfg.nt, 0.0f);
  wavelet[0] = 1.0f;

  const std::size_t sx = vel.nx() / 2;
  const std::size_t sy = vel.ny() / 2;
  const std::size_t sz = 2;
  const auto rx = rtm3d::rtm_internal::make_receiver_positions(vel, cfg.receiver_stride);

  std::vector<float> src_snaps(cfg.nt * vel.size(), 0.0f);
  std::vector<float> rec_data(cfg.nt * rx.size(), 0.0f);
  rtm3d::rtm_internal::forward_source_propagation(model, cfg, vel, damp, wavelet, sx, sy, sz, rx,
                                                  src_snaps, rec_data);

  double rec_l1 = 0.0;
  for (float v : rec_data) rec_l1 += std::abs(v);
  EXPECT_GT(rec_l1, 0.0);

  std::vector<float> image(vel.size(), 0.0f);
  rtm3d::rtm_internal::receiver_backpropagation_and_imaging(model, cfg, vel, damp, sy, sz, rx,
                                                            src_snaps, rec_data, image);

  double img_l1 = 0.0;
  for (float v : image) img_l1 += std::abs(v);
  EXPECT_GT(img_l1, 0.0);
}
