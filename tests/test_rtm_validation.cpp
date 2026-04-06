#include <stdexcept>

#include <gtest/gtest.h>

#include "rtm3d/model/GridModel2D.hpp"
#include "rtm3d/rtm/RtmEngine.hpp"
#include "../src/rtm/Validation.hpp"

TEST(RtmValidation, AcceptsReasonableConfiguration) {
  rtm3d::GridModel2D model{.nx = 16,
                           .nz = 12,
                           .dx = 10.0f,
                           .dz = 10.0f,
                           .values = std::vector<float>(16 * 12, 1800.0f)};
  rtm3d::RtmConfig cfg;
  cfg.ny = 8;
  cfg.nt = 40;
  cfg.receiver_stride = 2;
  cfg.pml = 4;

  EXPECT_NO_THROW(rtm3d::rtm_internal::validate_cfg(model, cfg));
}

TEST(RtmValidation, RejectsSmallOrInvalidInputs) {
  rtm3d::GridModel2D model{.nx = 16,
                           .nz = 12,
                           .dx = 10.0f,
                           .dz = 10.0f,
                           .values = std::vector<float>(16 * 12, 1800.0f)};
  rtm3d::RtmConfig cfg;

  auto bad_model = model;
  bad_model.nx = 4;
  EXPECT_THROW(rtm3d::rtm_internal::validate_cfg(bad_model, cfg), std::runtime_error);

  auto bad_cfg = cfg;
  bad_cfg.receiver_stride = 0;
  EXPECT_THROW(rtm3d::rtm_internal::validate_cfg(model, bad_cfg), std::runtime_error);

  bad_cfg = cfg;
  bad_cfg.pml = 0;
  EXPECT_THROW(rtm3d::rtm_internal::validate_cfg(model, bad_cfg), std::runtime_error);
}

TEST(RtmValidation, RejectsSmallNz) {
  rtm3d::GridModel2D model{.nx = 16, .nz = 4, .dx = 10.0f, .dz = 10.0f,
                           .values = std::vector<float>(16 * 4, 1800.0f)};
  rtm3d::RtmConfig cfg;
  cfg.ny = 8;
  cfg.nt = 40;

  EXPECT_THROW(rtm3d::rtm_internal::validate_cfg(model, cfg), std::runtime_error);
}

TEST(RtmValidation, RejectsNegativeSpacing) {
  rtm3d::GridModel2D model{.nx = 16, .nz = 12, .dx = 10.0f, .dz = 10.0f,
                           .values = std::vector<float>(16 * 12, 1800.0f)};
  rtm3d::RtmConfig cfg;
  cfg.ny = 8;
  cfg.nt = 40;
  cfg.pml = 4;
  cfg.receiver_stride = 2;

  auto bad_model = model;
  bad_model.dx = -1.0f;
  EXPECT_THROW(rtm3d::rtm_internal::validate_cfg(bad_model, cfg), std::runtime_error);

  bad_model = model;
  bad_model.dz = -5.0f;
  EXPECT_THROW(rtm3d::rtm_internal::validate_cfg(bad_model, cfg), std::runtime_error);
}

TEST(RtmValidation, RejectsZeroNy) {
  rtm3d::GridModel2D model{.nx = 16, .nz = 12, .dx = 10.0f, .dz = 10.0f,
                           .values = std::vector<float>(16 * 12, 1800.0f)};
  rtm3d::RtmConfig cfg;
  cfg.ny = 0;
  cfg.nt = 40;
  cfg.pml = 4;
  cfg.receiver_stride = 2;

  EXPECT_THROW(rtm3d::rtm_internal::validate_cfg(model, cfg), std::runtime_error);
}

TEST(RtmValidation, RejectsSmallNt) {
  rtm3d::GridModel2D model{.nx = 16, .nz = 12, .dx = 10.0f, .dz = 10.0f,
                           .values = std::vector<float>(16 * 12, 1800.0f)};
  rtm3d::RtmConfig cfg;
  cfg.ny = 8;
  cfg.nt = 1;  // Too few time steps
  cfg.pml = 4;
  cfg.receiver_stride = 2;

  EXPECT_THROW(rtm3d::rtm_internal::validate_cfg(model, cfg), std::runtime_error);
}

TEST(RtmValidation, RejectsInvalidScalarParameters) {
  rtm3d::GridModel2D model{.nx = 16, .nz = 12, .dx = 10.0f, .dz = 10.0f,
                           .values = std::vector<float>(16 * 12, 1800.0f)};
  rtm3d::RtmConfig cfg;
  cfg.ny = 8;
  cfg.nt = 40;
  cfg.pml = 4;
  cfg.receiver_stride = 2;

  // Invalid dy
  auto bad_cfg = cfg;
  bad_cfg.dy = 0.0f;
  EXPECT_THROW(rtm3d::rtm_internal::validate_cfg(model, bad_cfg), std::runtime_error);

  // Invalid dt
  bad_cfg = cfg;
  bad_cfg.dt = -0.001f;
  EXPECT_THROW(rtm3d::rtm_internal::validate_cfg(model, bad_cfg), std::runtime_error);

  // Invalid f0
  bad_cfg = cfg;
  bad_cfg.f0 = 0.0f;
  EXPECT_THROW(rtm3d::rtm_internal::validate_cfg(model, bad_cfg), std::runtime_error);
}

TEST(RtmValidation, AcceptsValidNy1) {
  // ny=1 should be valid (2D case)
  rtm3d::GridModel2D model{.nx = 16, .nz = 12, .dx = 10.0f, .dz = 10.0f,
                           .values = std::vector<float>(16 * 12, 1800.0f)};
  rtm3d::RtmConfig cfg;
  cfg.ny = 1;
  cfg.dy = 10.0f;
  cfg.nt = 40;
  cfg.pml = 4;
  cfg.receiver_stride = 2;

  EXPECT_NO_THROW(rtm3d::rtm_internal::validate_cfg(model, cfg));
}
