#include <cstdlib>
#include <iostream>
#include <string>

#include "rtm3d/ImageIO.hpp"
#include "rtm3d/MarmousiLoader.hpp"
#include "rtm3d/RtmEngine.hpp"

int main(int argc, char** argv) {
  try {
    if (argc < 2) {
      std::cerr << "Usage: rtm3d_cli <data_dir> [output.pgm]\n";
      return 1;
    }
    const std::string data_dir = argv[1];
    const std::string out = argc >= 3 ? argv[2] : "output/migrated_inline.pgm";

    auto model = rtm3d::load_marmousi_js(data_dir + "/x.json", data_dir + "/z.json",
                                         data_dir + "/vel.json", 4, 4, 220, 120);

    rtm3d::RtmConfig cfg;
    cfg.ny = 32;
    cfg.nt = 260;
    cfg.dt = 0.0015f;
    cfg.f0 = 12.0f;
    cfg.pml = 10;
    cfg.receiver_stride = 6;

    auto result = rtm3d::run_single_shot_rtm(model, cfg);
    rtm3d::write_pgm(out, result.inline_xz, result.nx, result.nz);

    std::cout << "RTM done. inline image: " << out << "\n";
    std::cout << "grid nx=" << result.nx << " nz=" << result.nz << "\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 2;
  }
}
