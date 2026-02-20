#include "rtm3d/cli/CliOptions.hpp"

#include <stdexcept>
#include <string>

namespace rtm3d {
namespace {

bool is_flag(const std::string& token) { return token.rfind("--", 0) == 0; }

std::string require_value(int argc, char** argv, int& i) {
  if (i + 1 >= argc) throw std::runtime_error("missing value for " + std::string(argv[i]));
  return argv[++i];
}

template <typename T>
T parse_num(const std::string& s, const std::string& name);

template <>
std::size_t parse_num<std::size_t>(const std::string& s, const std::string& name) {
  std::size_t p = 0;
  auto v = std::stoull(s, &p);
  if (p != s.size()) throw std::runtime_error("invalid value for " + name + ": " + s);
  return static_cast<std::size_t>(v);
}

template <>
float parse_num<float>(const std::string& s, const std::string& name) {
  std::size_t p = 0;
  auto v = std::stof(s, &p);
  if (p != s.size()) throw std::runtime_error("invalid value for " + name + ": " + s);
  return v;
}

void validate(const CliOptions& o) {
  if (o.x_file.empty() || o.z_file.empty() || o.values_file.empty()) {
    throw std::runtime_error("x/z/values input files are required (or --data-dir)");
  }
  if (o.load.decim_x == 0 || o.load.decim_z == 0) throw std::runtime_error("decimation must be >= 1");
  if (o.rtm.ny < 4 || o.rtm.nt < 2) throw std::runtime_error("ny>=4 and nt>=2 required");
  if (o.rtm.dy <= 0 || o.rtm.dt <= 0 || o.rtm.f0 <= 0) throw std::runtime_error("dy/dt/f0 must be > 0");
  if (o.rtm.pml == 0) throw std::runtime_error("pml must be > 0");
  if (o.rtm.receiver_stride == 0) throw std::runtime_error("receiver-stride must be > 0");
}

}  // namespace

std::string cli_help() {
  return "Usage: rtm3d_cli [options]\n"
         "Input model:\n"
         "  --data-dir <dir>              Directory containing x.json z.json vel.json\n"
         "  --x-file <path>               X axis JSON array\n"
         "  --z-file <path>               Z axis JSON array\n"
         "  --values-file <path>          2D values JSON array\n"
         "Load options:\n"
         "  --decim-x <n> --decim-z <n>   Decimation factors (>=1)\n"
         "  --crop-x <n> --crop-z <n>     Crop size (0 means full)\n"
         "RTM options:\n"
         "  --ny <n> --dy <m> --dt <s> --nt <n> --f0 <Hz> --pml <n> --receiver-stride <n>\n"
         "Output:\n"
         "  --output <path>               Output PGM file\n"
         "Other:\n"
         "  --help                         Show this message\n";
}

CliOptions parse_cli_or_throw(int argc, char** argv) {
  CliOptions o;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") throw std::runtime_error(cli_help());

    if (arg == "--data-dir") {
      const auto dir = require_value(argc, argv, i);
      o.x_file = dir + "/x.json";
      o.z_file = dir + "/z.json";
      o.values_file = dir + "/vel.json";
    } else if (arg == "--x-file") {
      o.x_file = require_value(argc, argv, i);
    } else if (arg == "--z-file") {
      o.z_file = require_value(argc, argv, i);
    } else if (arg == "--values-file") {
      o.values_file = require_value(argc, argv, i);
    } else if (arg == "--output") {
      o.output_file = require_value(argc, argv, i);
    } else if (arg == "--decim-x") {
      o.load.decim_x = parse_num<std::size_t>(require_value(argc, argv, i), "--decim-x");
    } else if (arg == "--decim-z") {
      o.load.decim_z = parse_num<std::size_t>(require_value(argc, argv, i), "--decim-z");
    } else if (arg == "--crop-x") {
      o.load.crop_x = parse_num<std::size_t>(require_value(argc, argv, i), "--crop-x");
    } else if (arg == "--crop-z") {
      o.load.crop_z = parse_num<std::size_t>(require_value(argc, argv, i), "--crop-z");
    } else if (arg == "--ny") {
      o.rtm.ny = parse_num<std::size_t>(require_value(argc, argv, i), "--ny");
    } else if (arg == "--dy") {
      o.rtm.dy = parse_num<float>(require_value(argc, argv, i), "--dy");
    } else if (arg == "--dt") {
      o.rtm.dt = parse_num<float>(require_value(argc, argv, i), "--dt");
    } else if (arg == "--nt") {
      o.rtm.nt = parse_num<std::size_t>(require_value(argc, argv, i), "--nt");
    } else if (arg == "--f0") {
      o.rtm.f0 = parse_num<float>(require_value(argc, argv, i), "--f0");
    } else if (arg == "--pml") {
      o.rtm.pml = parse_num<std::size_t>(require_value(argc, argv, i), "--pml");
    } else if (arg == "--receiver-stride") {
      o.rtm.receiver_stride = parse_num<std::size_t>(require_value(argc, argv, i), "--receiver-stride");
    } else if (is_flag(arg)) {
      throw std::runtime_error("unknown option: " + arg);
    } else {
      throw std::runtime_error("unexpected positional argument: " + arg);
    }
  }

  validate(o);
  return o;
}

}  // namespace rtm3d
