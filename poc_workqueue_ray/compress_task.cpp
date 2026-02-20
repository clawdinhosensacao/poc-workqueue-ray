#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <zlib.h>

struct Args {
    uint64_t seed = 0;
    size_t bytes = 4 * 1024 * 1024;
    int level = 6;
    std::string output = "out.bin";
    std::string stats = "stats.json";
};

bool parse_args(int argc, char** argv, Args& a) {
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        if (k == "--seed" && i + 1 < argc) a.seed = std::stoull(argv[++i]);
        else if (k == "--bytes" && i + 1 < argc) a.bytes = std::stoull(argv[++i]);
        else if (k == "--level" && i + 1 < argc) a.level = std::stoi(argv[++i]);
        else if (k == "--output" && i + 1 < argc) a.output = argv[++i];
        else if (k == "--stats" && i + 1 < argc) a.stats = argv[++i];
        else {
            std::cerr << "Unknown/invalid arg: " << k << "\n";
            return false;
        }
    }
    return true;
}

int main(int argc, char** argv) {
    Args args;
    if (!parse_args(argc, argv, args)) return 2;

    auto t0 = std::chrono::steady_clock::now();
    std::vector<uint8_t> input(args.bytes);
    std::mt19937_64 gen(args.seed);
    for (size_t i = 0; i < args.bytes; ++i) input[i] = static_cast<uint8_t>(gen() & 0xFF);
    auto t1 = std::chrono::steady_clock::now();

    uLongf out_bound = compressBound(args.bytes);
    std::vector<uint8_t> output(out_bound);
    int rc = compress2(output.data(), &out_bound, input.data(), args.bytes, args.level);
    auto t2 = std::chrono::steady_clock::now();
    if (rc != Z_OK) {
        std::cerr << "compress2 failed with code " << rc << "\n";
        return 1;
    }

    std::ofstream bin(args.output, std::ios::binary);
    bin.write(reinterpret_cast<const char*>(output.data()), static_cast<std::streamsize>(out_bound));
    bin.close();

    auto gen_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    auto comp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t0).count();

    std::ofstream st(args.stats);
    st << "{\n"
       << "  \"seed\": " << args.seed << ",\n"
       << "  \"input_bytes\": " << args.bytes << ",\n"
       << "  \"compressed_bytes\": " << out_bound << ",\n"
       << "  \"ratio\": " << (double)out_bound / (double)args.bytes << ",\n"
       << "  \"gen_ms\": " << gen_ms << ",\n"
       << "  \"comp_ms\": " << comp_ms << ",\n"
       << "  \"total_ms\": " << total_ms << "\n"
       << "}\n";

    std::cout << "seed=" << args.seed << " input=" << args.bytes << " compressed=" << out_bound
              << " total_ms=" << total_ms << "\n";
    return 0;
}
