#include <cmath>
#include <filesystem>
#include <iostream>

#include "rtm3d/ImageIO.hpp"
#include "rtm3d/MarmousiLoader.hpp"
#include "rtm3d/RtmEngine.hpp"

#define CHECK(cond)                                                                            \
  do {                                                                                         \
    if (!(cond)) {                                                                             \
      std::cerr << "CHECK failed: " #cond " at line " << __LINE__ << "\n";                  \
      return 1;                                                                                \
    }                                                                                          \
  } while (0)

int main() {
  using namespace rtm3d;

  auto w = ricker_wavelet(200, 0.001f, 15.0f);
  float peak = -1e9f;
  for (float v : w) peak = std::max(peak, v);
  CHECK(peak > 0.9f);

  auto model = load_marmousi_js("data/x.json", "data/z.json", "data/vel.json", 20, 20, 50, 30);
  CHECK(model.nx > 10 && model.nz > 10);

  RtmConfig cfg;
  cfg.ny = 12;
  cfg.nt = 60;
  cfg.receiver_stride = 4;
  cfg.pml = 4;
  auto img = run_single_shot_rtm(model, cfg);

  float l1 = 0.0f;
  for (float v : img.inline_xz) l1 += std::abs(v);
  CHECK(l1 > 0.0f);

  std::filesystem::create_directories("output");
  write_pgm("output/test_inline.pgm", img.inline_xz, img.nx, img.nz);
  CHECK(std::filesystem::exists("output/test_inline.pgm"));

  std::cout << "tests ok\n";
  return 0;
}
