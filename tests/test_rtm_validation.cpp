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
