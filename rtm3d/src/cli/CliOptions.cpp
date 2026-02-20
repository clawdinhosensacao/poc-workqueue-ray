#include "rtm3d/cli/CliOptions.hpp"

#include <fstream>
#include <regex>
#include <stdexcept>
#include <string>

namespace rtm3d {
namespace {

bool is_flag(const std::string& token) { return token.rfind("--", 0) == 0; }

std::string require_value(int argc, char** argv, int& i) {
  if (i + 1 >= argc) throw std::runtime_error("missing value for " + std::string(argv[i]));
  return argv[++i];
}

std::string slurp_file(const std::string& path) {
  std::ifstream f(path);
  if (!f) throw std::runtime_error("cannot open config file: " + path);
  return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

std::string json_find_string(const std::string& s, const std::string& key) {
  const std::regex rx("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
  std::smatch m;
  if (std::regex_search(s, m, rx)) return m[1].str();
  return "";
}

std::string json_find_number_token(const std::string& s, const std::string& key) {
  const std::regex rx("\\\"" + key + "\\\"\\s*:\\s*([-+0-9eE\\.]+)");
  std::smatch m;
  if (std::regex_search(s, m, rx)) return m[1].str();
  return "";
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

void apply_json_config(CliOptions& o, const std::string& path) {
  const auto s = slurp_file(path);

  if (const auto dir = json_find_string(s, "data_dir"); !dir.empty()) {
    o.x_file = dir + "/x.json";
    o.z_file = dir + "/z.json";
    o.values_file = dir + "/vel.json";
  }
  if (const auto v = json_find_string(s, "x_file"); !v.empty()) o.x_file = v;
  if (const auto v = json_find_string(s, "z_file"); !v.empty()) o.z_file = v;
  if (const auto v = json_find_string(s, "values_file"); !v.empty()) o.values_file = v;
  if (const auto v = json_find_string(s, "output_file"); !v.empty()) o.output_file = v;

  if (const auto v = json_find_string(s, "output_format"); !v.empty()) {
    if (v == "pgm8") o.output_format = OutputFormat::kPgm8;
    else if (v == "float32_raw") o.output_format = OutputFormat::kFloat32Raw;
    else throw std::runtime_error("invalid output_format in config: " + v);
  }

  if (const auto v = json_find_number_token(s, "decim_x"); !v.empty()) o.load.decim_x = parse_num<std::size_t>(v, "decim_x");
  if (const auto v = json_find_number_token(s, "decim_z"); !v.empty()) o.load.decim_z = parse_num<std::size_t>(v, "decim_z");
  if (const auto v = json_find_number_token(s, "crop_x"); !v.empty()) o.load.crop_x = parse_num<std::size_t>(v, "crop_x");
  if (const auto v = json_find_number_token(s, "crop_z"); !v.empty()) o.load.crop_z = parse_num<std::size_t>(v, "crop_z");

  if (const auto v = json_find_number_token(s, "ny"); !v.empty()) o.rtm.ny = parse_num<std::size_t>(v, "ny");
  if (const auto v = json_find_number_token(s, "dy"); !v.empty()) o.rtm.dy = parse_num<float>(v, "dy");
  if (const auto v = json_find_number_token(s, "dt"); !v.empty()) o.rtm.dt = parse_num<float>(v, "dt");
  if (const auto v = json_find_number_token(s, "nt"); !v.empty()) o.rtm.nt = parse_num<std::size_t>(v, "nt");
  if (const auto v = json_find_number_token(s, "f0"); !v.empty()) o.rtm.f0 = parse_num<float>(v, "f0");
  if (const auto v = json_find_number_token(s, "pml"); !v.empty()) o.rtm.pml = parse_num<std::size_t>(v, "pml");
  if (const auto v = json_find_number_token(s, "receiver_stride"); !v.empty())
    o.rtm.receiver_stride = parse_num<std::size_t>(v, "receiver_stride");
}

void validate(const CliOptions& o) {
  if (o.x_file.empty() || o.z_file.empty() || o.values_file.empty()) {
    throw std::runtime_error("x/z/values input files are required (or --data-dir / --config)");
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
         "  --config <file.json>          JSON config file (recommended)\n"
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
         "  --output <path>               Output file path\n"
         "  --output-format <pgm8|float32_raw>\n"
         "Other:\n"
         "  --help                         Show this message\n";
}

CliOptions parse_cli_or_throw(int argc, char** argv) {
  CliOptions o;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") throw std::runtime_error(cli_help());

    if (arg == "--config") {
      apply_json_config(o, require_value(argc, argv, i));
    } else if (arg == "--data-dir") {
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
    } else if (arg == "--output-format") {
      const auto v = require_value(argc, argv, i);
      if (v == "pgm8") o.output_format = OutputFormat::kPgm8;
      else if (v == "float32_raw") o.output_format = OutputFormat::kFloat32Raw;
      else throw std::runtime_error("invalid --output-format: " + v);
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
