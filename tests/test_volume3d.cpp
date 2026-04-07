#include <gtest/gtest.h>

#include "rtm3d/core/Volume3D.hpp"

TEST(Volume3D, ConstructsValidVolume) {
  rtm3d::Volume3D vol(10, 20, 30);

  EXPECT_EQ(vol.nx(), 10u);
  EXPECT_EQ(vol.ny(), 20u);
  EXPECT_EQ(vol.nz(), 30u);
  EXPECT_EQ(vol.size(), 10u * 20u * 30u);
}

TEST(Volume3D, InitializesToZeroByDefault) {
  rtm3d::Volume3D vol(5, 5, 5);

  for (std::size_t i = 0; i < vol.size(); ++i) {
    EXPECT_FLOAT_EQ(vol.raw()[i], 0.0f);
  }
}

TEST(Volume3D, InitializesToCustomValue) {
  rtm3d::Volume3D vol(4, 4, 4, 3.14f);

  for (std::size_t i = 0; i < vol.size(); ++i) {
    EXPECT_FLOAT_EQ(vol.raw()[i], 3.14f);
  }
}

TEST(Volume3D, IndexComputesCorrectLinearOffset) {
  rtm3d::Volume3D vol(3, 4, 2);  // nx=3, ny=4, nz=2

  // Verify layout is [iz][iy][ix]
  // Index at (0,0,0) should be 0
  EXPECT_EQ(vol.index(0, 0, 0), 0u);

  // Index at (1,0,0) should be 1
  EXPECT_EQ(vol.index(1, 0, 0), 1u);

  // Index at (0,1,0) should be nx = 3
  EXPECT_EQ(vol.index(0, 1, 0), 3u);

  // Index at (0,0,1) should be nx * ny = 12
  EXPECT_EQ(vol.index(0, 0, 1), 12u);

  // Index at (2, 3, 1) should be 12 + 3*3 + 2 = 23
  EXPECT_EQ(vol.index(2, 3, 1), 23u);
}

TEST(Volume3D, OperatorParenAccessesCorrectElements) {
  rtm3d::Volume3D vol(3, 2, 2);

  // Set values at specific positions
  vol(0, 0, 0) = 1.0f;
  vol(1, 0, 0) = 2.0f;
  vol(2, 0, 0) = 3.0f;
  vol(0, 1, 0) = 4.0f;
  vol(0, 0, 1) = 10.0f;

  // Verify access
  EXPECT_FLOAT_EQ(vol(0, 0, 0), 1.0f);
  EXPECT_FLOAT_EQ(vol(1, 0, 0), 2.0f);
  EXPECT_FLOAT_EQ(vol(2, 0, 0), 3.0f);
  EXPECT_FLOAT_EQ(vol(0, 1, 0), 4.0f);
  EXPECT_FLOAT_EQ(vol(0, 0, 1), 10.0f);
}

TEST(Volume3D, RawDataModificationIsVisible) {
  rtm3d::Volume3D vol(3, 3, 3);

  // Modify through raw() reference
  vol.raw()[0] = 100.0f;
  vol.raw()[vol.size() - 1] = 200.0f;

  // Verify visible through operator()
  EXPECT_FLOAT_EQ(vol(0, 0, 0), 100.0f);
  EXPECT_FLOAT_EQ(vol(2, 2, 2), 200.0f);
}

TEST(Volume3D, ConstAccessWorks) {
  rtm3d::Volume3D vol(2, 2, 2, 5.0f);
  const rtm3d::Volume3D& cref = vol;

  // Const access should work
  EXPECT_FLOAT_EQ(cref(0, 0, 0), 5.0f);
  EXPECT_FLOAT_EQ(cref(1, 1, 1), 5.0f);

  // Const raw access
  EXPECT_EQ(cref.raw().size(), 8u);
}

TEST(Volume3D, MinimalVolumeWorks) {
  // Smallest reasonable volume
  rtm3d::Volume3D vol(1, 1, 1);

  EXPECT_EQ(vol.size(), 1u);
  vol(0, 0, 0) = 42.0f;
  EXPECT_FLOAT_EQ(vol(0, 0, 0), 42.0f);
}

TEST(Volume3D, LargeVolumeSizeCorrect) {
  std::size_t nx = 100, ny = 50, nz = 80;
  rtm3d::Volume3D vol(nx, ny, nz, 1.0f);
  EXPECT_EQ(vol.size(), nx * ny * nz);
  EXPECT_EQ(vol.nx(), nx);
  EXPECT_EQ(vol.ny(), ny);
  EXPECT_EQ(vol.nz(), nz);
}

TEST(Volume3D, IndexConsistentWithParenOperator) {
  rtm3d::Volume3D vol(5, 4, 3, 0.0f);
  
  for (std::size_t z = 0; z < vol.nz(); ++z) {
    for (std::size_t y = 0; y < vol.ny(); ++y) {
      for (std::size_t x = 0; x < vol.nx(); ++x) {
        std::size_t idx = vol.index(x, y, z);
        float val = static_cast<float>(z * 1000 + y * 10 + x);
        vol.raw()[idx] = val;
        EXPECT_FLOAT_EQ(vol(x, y, z), val);
      }
    }
  }
}

TEST(Volume3D, AllElementsInitialized) {
  rtm3d::Volume3D vol(8, 6, 4, 7.77f);
  
  for (std::size_t i = 0; i < vol.size(); ++i) {
    EXPECT_FLOAT_EQ(vol.raw()[i], 7.77f);
  }
}

TEST(Volume3D, RawDataPointerValid) {
  rtm3d::Volume3D vol(4, 3, 2, 1.5f);
  const std::vector<float>& raw = vol.raw();
  
  EXPECT_EQ(raw.size(), vol.size());
  EXPECT_FLOAT_EQ(raw[0], 1.5f);
  EXPECT_FLOAT_EQ(raw[vol.size() - 1], 1.5f);
}
