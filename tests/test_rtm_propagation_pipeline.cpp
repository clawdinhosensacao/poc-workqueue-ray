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

TEST(RtmPropagationPipeline, SourceSnapshotsHaveCorrectSize) {
  rtm3d::GridModel2D model;
  model.nx = 12;
  model.nz = 10;
  model.dx = 10.0f;
  model.dz = 10.0f;
  model.values = std::vector<float>(model.nx * model.nz, 2000.0f);

  rtm3d::RtmConfig cfg;
  cfg.ny = 6;
  cfg.nt = 15;
  cfg.dt = 0.001f;
  cfg.receiver_stride = 4;

  rtm3d::Volume3D vel(model.nx, cfg.ny, model.nz, 2000.0f);
  std::vector<float> damp(vel.size(), 1.0f);
  std::vector<float> wavelet(cfg.nt, 0.0f);
  wavelet[0] = 1.0f;

  std::vector<float> src_snaps(cfg.nt * vel.size(), 0.0f);
  std::vector<float> rec_data(cfg.nt * vel.nx(), 0.0f);

  const auto rx = rtm3d::rtm_internal::make_receiver_positions(vel, cfg.receiver_stride);

  rtm3d::rtm_internal::forward_source_propagation(model, cfg, vel, damp, wavelet,
                                                   vel.nx() / 2, vel.ny() / 2, 2,
                                                   rx, src_snaps, rec_data);

  // Snapshot size should be nt * volume size
  EXPECT_EQ(src_snaps.size(), cfg.nt * vel.size());
}

TEST(RtmPropagationPipeline, ReceiverDataSizeCorrect) {
  rtm3d::GridModel2D model;
  model.nx = 20;
  model.nz = 14;
  model.dx = 10.0f;
  model.dz = 10.0f;
  model.values = std::vector<float>(model.nx * model.nz, 1800.0f);

  rtm3d::RtmConfig cfg;
  cfg.ny = 4;
  cfg.nt = 25;
  cfg.dt = 0.001f;
  cfg.receiver_stride = 5;

  rtm3d::Volume3D vel(model.nx, cfg.ny, model.nz, 1800.0f);
  std::vector<float> damp(vel.size(), 1.0f);
  std::vector<float> wavelet(cfg.nt, 0.0f);
  wavelet[0] = 1.0f;

  const auto rx = rtm3d::rtm_internal::make_receiver_positions(vel, cfg.receiver_stride);
  std::vector<float> src_snaps(cfg.nt * vel.size(), 0.0f);
  std::vector<float> rec_data(cfg.nt * rx.size(), 0.0f);

  rtm3d::rtm_internal::forward_source_propagation(model, cfg, vel, damp, wavelet,
                                                   vel.nx() / 2, vel.ny() / 2, 2,
                                                   rx, src_snaps, rec_data);

  // Receiver data size should be nt * num_receivers
  EXPECT_EQ(rec_data.size(), cfg.nt * rx.size());
}

TEST(RtmPropagationPipeline, DifferentSourcePositionsProduceDifferentResults) {
  rtm3d::GridModel2D model;
  model.nx = 24;
  model.nz = 16;
  model.dx = 10.0f;
  model.dz = 10.0f;
  model.values = std::vector<float>(model.nx * model.nz, 2000.0f);

  rtm3d::RtmConfig cfg;
  cfg.ny = 8;
  cfg.nt = 30;
  cfg.dt = 0.001f;
  cfg.receiver_stride = 4;

  rtm3d::Volume3D vel(model.nx, cfg.ny, model.nz, 2000.0f);
  std::vector<float> damp(vel.size(), 1.0f);
  std::vector<float> wavelet(cfg.nt, 0.0f);
  wavelet[0] = 1.0f;

  const auto rx = rtm3d::rtm_internal::make_receiver_positions(vel, cfg.receiver_stride);

  // Source position 1
  std::vector<float> src_snaps1(cfg.nt * vel.size(), 0.0f);
  std::vector<float> rec_data1(cfg.nt * rx.size(), 0.0f);
  rtm3d::rtm_internal::forward_source_propagation(model, cfg, vel, damp, wavelet,
                                                   4, vel.ny() / 2, 2,
                                                   rx, src_snaps1, rec_data1);

  // Source position 2
  std::vector<float> src_snaps2(cfg.nt * vel.size(), 0.0f);
  std::vector<float> rec_data2(cfg.nt * rx.size(), 0.0f);
  rtm3d::rtm_internal::forward_source_propagation(model, cfg, vel, damp, wavelet,
                                                   20, vel.ny() / 2, 2,
                                                   rx, src_snaps2, rec_data2);

  // Results should differ
  bool differ = false;
  for (std::size_t i = 0; i < rec_data1.size(); ++i) {
    if (std::abs(rec_data1[i] - rec_data2[i]) > 1e-10f) {
      differ = true;
      break;
    }
  }
  EXPECT_TRUE(differ);
}

TEST(RtmPropagationPipeline, MinimalConfiguration) {
  rtm3d::GridModel2D model;
  model.nx = 8;
  model.nz = 6;
  model.dx = 10.0f;
  model.dz = 10.0f;
  model.values = std::vector<float>(model.nx * model.nz, 1500.0f);

  rtm3d::RtmConfig cfg;
  cfg.ny = 2;
  cfg.nt = 10;
  cfg.dt = 0.001f;
  cfg.receiver_stride = 2;

  rtm3d::Volume3D vel(model.nx, cfg.ny, model.nz, 1500.0f);
  std::vector<float> damp(vel.size(), 1.0f);
  std::vector<float> wavelet(cfg.nt, 0.0f);
  wavelet[0] = 1.0f;

  const auto rx = rtm3d::rtm_internal::make_receiver_positions(vel, cfg.receiver_stride);
  std::vector<float> src_snaps(cfg.nt * vel.size(), 0.0f);
  std::vector<float> rec_data(cfg.nt * rx.size(), 0.0f);

  EXPECT_NO_THROW(
    rtm3d::rtm_internal::forward_source_propagation(model, cfg, vel, damp, wavelet,
                                                     vel.nx() / 2, vel.ny() / 2, 1,
                                                     rx, src_snaps, rec_data)
  );
}
