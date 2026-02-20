#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <zfp.h>

struct Args {
    uint64_t seed = 1234;
    size_t n = 1'000'000;
    double tolerance = 1e-3;
    std::string out;
};

Args parse_args(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for " << name << "\n";
                std::exit(2);
            }
            return argv[++i];
        };
        if (k == "--seed") a.seed = std::stoull(next("--seed"));
        else if (k == "--n") a.n = static_cast<size_t>(std::stoull(next("--n")));
        else if (k == "--tolerance") a.tolerance = std::stod(next("--tolerance"));
        else if (k == "--out") a.out = next("--out");
        else {
            std::cerr << "Unknown arg: " << k << "\n";
            std::exit(2);
        }
    }
    return a;
}

int main(int argc, char** argv) {
    Args args = parse_args(argc, argv);

    auto t0 = std::chrono::steady_clock::now();

    std::vector<float> input(args.n);
    std::mt19937_64 rng(args.seed);
    std::uniform_real_distribution<float> dist(-1000.0f, 1000.0f);
    for (auto& v : input) v = dist(rng);

    zfp_field* field = zfp_field_1d(input.data(), zfp_type_float, input.size());
    if (!field) {
        std::cerr << "zfp_field_1d failed\n";
        return 1;
    }

    zfp_stream* zfp = zfp_stream_open(nullptr);
    if (!zfp) {
        zfp_field_free(field);
        std::cerr << "zfp_stream_open failed\n";
        return 1;
    }

    zfp_stream_set_accuracy(zfp, args.tolerance);

    size_t max_size = zfp_stream_maximum_size(zfp, field);
    std::vector<unsigned char> compressed(max_size);
    bitstream* stream = stream_open(compressed.data(), compressed.size());
    if (!stream) {
        zfp_stream_close(zfp);
        zfp_field_free(field);
        std::cerr << "stream_open failed\n";
        return 1;
    }

    zfp_stream_set_bit_stream(zfp, stream);
    zfp_stream_rewind(zfp);

    auto c0 = std::chrono::steady_clock::now();
    size_t zfp_size = zfp_compress(zfp, field);
    auto c1 = std::chrono::steady_clock::now();

    if (zfp_size == 0) {
        stream_close(stream);
        zfp_stream_close(zfp);
        zfp_field_free(field);
        std::cerr << "zfp_compress failed\n";
        return 1;
    }

    if (!args.out.empty()) {
        std::ofstream ofs(args.out, std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(compressed.data()), static_cast<std::streamsize>(zfp_size));
    }

    stream_close(stream);
    zfp_stream_close(zfp);
    zfp_field_free(field);

    auto t1 = std::chrono::steady_clock::now();
    double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double comp_ms = std::chrono::duration<double, std::milli>(c1 - c0).count();

    const size_t input_bytes = input.size() * sizeof(float);
    double ratio = static_cast<double>(input_bytes) / static_cast<double>(zfp_size);

    std::cout << "seed=" << args.seed
              << " n=" << args.n
              << " tolerance=" << args.tolerance
              << " input_bytes=" << input_bytes
              << " compressed_bytes=" << zfp_size
              << " ratio=" << ratio
              << " comp_ms=" << comp_ms
              << " total_ms=" << total_ms
              << "\n";
    return 0;
}
