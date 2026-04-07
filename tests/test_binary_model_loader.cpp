#include <gtest/gtest.h>

#include <cmath>
#include <cstring>
#include <fstream>
#include <random>
#include <vector>

#include "rtm3d/io/BinaryModelLoader.hpp"

namespace rtm3d {
namespace {

constexpr float kTolerance = 1e-6f;

class TmpFile {
public:
  explicit TmpFile(const std::string& name) : path_("/tmp/" + name) {}
  ~TmpFile() { std::remove(path_.c_str()); }
  const std::string& path() const { return path_; }

private:
  std::string path_;
};

std::vector<float> make_random_velocity(std::size_t n, unsigned seed = 42) {
  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> dist(1500.0f, 4000.0f);
  std::vector<float> v(n);
  for (auto& x : v) x = dist(rng);
  return v;
}

void write_raw_binary(const std::string& path, const std::vector<float>& data) {
  std::ofstream f(path, std::ios::binary);
  ASSERT_TRUE(f) << "failed to create " << path;
  f.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(float));
  ASSERT_TRUE(f) << "failed to write " << path;
}

}  // namespace

TEST(BinaryModelLoader, LoadSimple2DVelocity) {
  TmpFile tmp("test_simple.bin");
  GridSpec grid{.nx = 10, .nz = 8, .ny = 1, .dx = 5.0f, .dz = 5.0f};

  auto expected = make_random_velocity(grid.nx * grid.nz);
  write_raw_binary(tmp.path(), expected);

  auto model = load_binary_velocity(tmp.path(), grid);

  ASSERT_EQ(model.velocity().size(), expected.size());
  for (std::size_t i = 0; i < expected.size(); ++i) {
    EXPECT_NEAR(model.velocity()[i], expected[i], kTolerance);
  }
}

TEST(BinaryModelLoader, Load3DVelocity) {
  TmpFile tmp("test_3d.bin");
  GridSpec grid{.nx = 12, .nz = 10, .ny = 5, .dx = 10.0f, .dz = 10.0f, .dy = 10.0f};

  auto expected = make_random_velocity(grid.nx * grid.nz * grid.ny);
  write_raw_binary(tmp.path(), expected);

  auto model = load_binary_velocity(tmp.path(), grid);

  ASSERT_EQ(model.velocity().size(), expected.size());
  for (std::size_t i = 0; i < expected.size(); ++i) {
    EXPECT_NEAR(model.velocity()[i], expected[i], kTolerance);
  }
}

TEST(BinaryModelLoader, RejectsSizeMismatch) {
  TmpFile tmp("test_mismatch.bin");
  GridSpec grid{.nx = 10, .nz = 10, .ny = 1};

  auto data = make_random_velocity(50);  // Wrong size
  write_raw_binary(tmp.path(), data);

  EXPECT_THROW(load_binary_velocity(tmp.path(), grid), std::runtime_error);
}

TEST(BinaryModelLoader, RejectsMissingFile) {
  GridSpec grid{.nx = 10, .nz = 10, .ny = 1};
  EXPECT_THROW(load_binary_velocity("/nonexistent/path.bin", grid), std::runtime_error);
}

TEST(BinaryModelLoader, LoadWithHeader) {
  TmpFile tmp("test_header.bin");
  GridSpec grid{.nx = 8, .nz = 6, .ny = 3, .dx = 12.0f, .dz = 8.0f, .dy = 10.0f};

  auto data = make_random_velocity(grid.nx * grid.nz * grid.ny);

  // Write header + data
  {
    BinaryVelocityHeader hdr;
    hdr.nx = static_cast<std::uint32_t>(grid.nx);
    hdr.nz = static_cast<std::uint32_t>(grid.nz);
    hdr.ny = static_cast<std::uint32_t>(grid.ny);
    hdr.dx = grid.dx;
    hdr.dz = grid.dz;
    hdr.dy = grid.dy;

    std::ofstream f(tmp.path(), std::ios::binary);
    f.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    f.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(float));
  }

  auto model = load_binary_velocity_with_header(tmp.path());

  EXPECT_EQ(model.grid().nx, grid.nx);
  EXPECT_EQ(model.grid().nz, grid.nz);
  EXPECT_EQ(model.grid().ny, grid.ny);
  EXPECT_FLOAT_EQ(model.grid().dx, grid.dx);
  EXPECT_FLOAT_EQ(model.grid().dz, grid.dz);
  EXPECT_FLOAT_EQ(model.grid().dy, grid.dy);

  for (std::size_t i = 0; i < data.size(); ++i) {
    EXPECT_NEAR(model.velocity()[i], data[i], kTolerance);
  }
}

TEST(BinaryModelLoader, RejectsInvalidHeaderMagic) {
  TmpFile tmp("test_bad_magic.bin");

  BinaryVelocityHeader hdr;
  hdr.magic = 0xDEADBEEF;  // Invalid magic
  hdr.nx = 10;
  hdr.nz = 10;
  hdr.ny = 1;

  std::vector<float> data(100, 1500.0f);

  std::ofstream f(tmp.path(), std::ios::binary);
  f.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
  f.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(float));

  EXPECT_THROW(load_binary_velocity_with_header(tmp.path()), std::runtime_error);
}

TEST(BinaryModelLoader, WriteAndReadRoundTrip) {
  TmpFile tmp("test_roundtrip.bin");
  GridSpec grid{.nx = 20, .nz = 15, .ny = 1, .dx = 10.0f, .dz = 10.0f};

  auto original = SeismicModel::from_velocity(make_random_velocity(grid.nx * grid.nz), grid);

  write_binary_velocity(tmp.path(), original, true);
  auto loaded = load_binary_velocity_with_header(tmp.path());

  ASSERT_EQ(loaded.velocity().size(), original.velocity().size());
  for (std::size_t i = 0; i < original.velocity().size(); ++i) {
    EXPECT_NEAR(loaded.velocity()[i], original.velocity()[i], kTolerance);
  }

  EXPECT_EQ(loaded.grid().nx, original.grid().nx);
  EXPECT_EQ(loaded.grid().nz, original.grid().nz);
  EXPECT_EQ(loaded.grid().ny, original.grid().ny);
}

TEST(BinaryModelLoader, WriteWithoutHeader) {
  TmpFile tmp("test_no_header.bin");
  GridSpec grid{.nx = 10, .nz = 10, .ny = 1};

  auto original = SeismicModel::from_velocity(make_random_velocity(100), grid);

  write_binary_velocity(tmp.path(), original, false);
  auto loaded = load_binary_velocity(tmp.path(), grid);

  ASSERT_EQ(loaded.velocity().size(), original.velocity().size());
  for (std::size_t i = 0; i < original.velocity().size(); ++i) {
    EXPECT_NEAR(loaded.velocity()[i], original.velocity()[i], kTolerance);
  }
}

TEST(MappedVelocityModel, MapSimpleFile) {
  TmpFile tmp("test_mmap.bin");
  GridSpec grid{.nx = 16, .nz = 12, .ny = 1};

  auto expected = make_random_velocity(grid.nx * grid.nz);
  write_raw_binary(tmp.path(), expected);

  auto mapped = mmap_binary_velocity(tmp.path(), grid);

  ASSERT_EQ(mapped->size(), expected.size());
  for (std::size_t i = 0; i < expected.size(); ++i) {
    EXPECT_NEAR(mapped->data()[i], expected[i], kTolerance);
  }
}

TEST(MappedVelocityModel, AccessByIndex) {
  TmpFile tmp("test_mmap_idx.bin");
  GridSpec grid{.nx = 8, .nz = 6, .ny = 3};

  auto data = make_random_velocity(grid.nx * grid.nz * grid.ny);
  write_raw_binary(tmp.path(), data);

  auto mapped = mmap_binary_velocity(tmp.path(), grid);

  // Test 3D index access
  for (std::size_t j = 0; j < grid.ny; ++j) {
    for (std::size_t k = 0; k < grid.nz; ++k) {
      for (std::size_t i = 0; i < grid.nx; ++i) {
        const std::size_t flat = j * grid.nz * grid.nx + k * grid.nx + i;
        EXPECT_NEAR(mapped->at(i, k, j), data[flat], kTolerance);
      }
    }
  }
}

TEST(MappedVelocityModel, RejectsSizeMismatch) {
  TmpFile tmp("test_mmap_mismatch.bin");
  GridSpec grid{.nx = 10, .nz = 10, .ny = 1};

  auto data = make_random_velocity(50);  // Wrong size
  write_raw_binary(tmp.path(), data);

  EXPECT_THROW(mmap_binary_velocity(tmp.path(), grid), std::runtime_error);
}

TEST(MappedVelocityModel, RejectsMissingFile) {
  GridSpec grid{.nx = 10, .nz = 10, .ny = 1};
  EXPECT_THROW(mmap_binary_velocity("/nonexistent/path.bin", grid), std::runtime_error);
}

TEST(BinaryModelLoader, SingleElementModel) {
  TmpFile tmp("test_single.bin");
  GridSpec grid{.nx = 1, .nz = 1, .ny = 1, .dx = 5.0f, .dz = 5.0f};

  std::vector<float> data = {2500.0f};
  write_raw_binary(tmp.path(), data);

  auto model = load_binary_velocity(tmp.path(), grid);
  ASSERT_EQ(model.velocity().size(), 1u);
  EXPECT_NEAR(model.velocity()[0], 2500.0f, kTolerance);
}

TEST(BinaryModelLoader, LargeModel) {
  TmpFile tmp("test_large.bin");
  GridSpec grid{.nx = 100, .nz = 80, .ny = 1, .dx = 10.0f, .dz = 10.0f};

  auto expected = make_random_velocity(grid.nx * grid.nz);
  write_raw_binary(tmp.path(), expected);

  auto model = load_binary_velocity(tmp.path(), grid);
  ASSERT_EQ(model.velocity().size(), expected.size());
}

TEST(BinaryModelLoader, HeaderPreservesGridSpec) {
  TmpFile tmp("test_grid.bin");
  GridSpec grid{.nx = 15, .nz = 12, .ny = 2, .dx = 7.5f, .dz = 6.0f, .dy = 8.0f};

  auto data = make_random_velocity(grid.nx * grid.nz * grid.ny);

  BinaryVelocityHeader hdr;
  hdr.magic = kBinaryVelocityMagic;
  hdr.nx = static_cast<std::uint32_t>(grid.nx);
  hdr.nz = static_cast<std::uint32_t>(grid.nz);
  hdr.ny = static_cast<std::uint32_t>(grid.ny);
  hdr.dx = grid.dx;
  hdr.dz = grid.dz;
  hdr.dy = grid.dy;

  std::ofstream f(tmp.path(), std::ios::binary);
  f.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
  f.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(float));
  f.close();

  auto model = load_binary_velocity_with_header(tmp.path());

  EXPECT_EQ(model.grid().nx, grid.nx);
  EXPECT_EQ(model.grid().nz, grid.nz);
  EXPECT_EQ(model.grid().ny, grid.ny);
}

TEST(BinaryModelLoader, Handles2DModel) {
  TmpFile tmp("test_2d.bin");
  GridSpec grid{.nx = 10, .nz = 8, .ny = 1, .dx = 5.0f, .dz = 5.0f};

  auto expected = make_random_velocity(grid.nx * grid.nz);
  write_raw_binary(tmp.path(), expected);

  auto model = load_binary_velocity(tmp.path(), grid);
  ASSERT_EQ(model.velocity().size(), grid.nx * grid.nz);
}

TEST(MappedVelocityModel, ConstDataAccess) {
  TmpFile tmp("test_mmap_const.bin");
  GridSpec grid{.nx = 8, .nz = 6, .ny = 1};

  auto data = make_random_velocity(grid.nx * grid.nz);
  write_raw_binary(tmp.path(), data);

  const auto mapped = mmap_binary_velocity(tmp.path(), grid);
  ASSERT_NE(mapped, nullptr);

  const float* ptr = mapped->data();
  EXPECT_NE(ptr, nullptr);
  EXPECT_NEAR(ptr[0], data[0], kTolerance);
}

TEST(MappedVelocityModel, SizeMatchesGrid) {
  TmpFile tmp("test_mmap_size.bin");
  GridSpec grid{.nx = 20, .nz = 15, .ny = 3};

  auto data = make_random_velocity(grid.nx * grid.nz * grid.ny);
  write_raw_binary(tmp.path(), data);

  auto mapped = mmap_binary_velocity(tmp.path(), grid);
  EXPECT_EQ(mapped->size(), grid.nx * grid.nz * grid.ny);
}

}  // namespace rtm3d
