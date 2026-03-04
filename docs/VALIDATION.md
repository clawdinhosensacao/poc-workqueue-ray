# Validation notes

## Environment
- Compiler: `g++` (GNU C++)
- Build route used in this environment: `make`
- CMake files are provided, but `cmake` binary was not available in this runtime.

## Commands executed
```bash
./scripts/download_marmousi.sh data
make -j2 build/rtm3d_cli build/rtm3d_tests
./build/rtm3d_tests --gtest_brief=1
./build/rtm3d_cli --data-dir data --decim-x 20 --decim-z 20 --crop-x 80 --crop-z 50 \
  --ny 24 --dy 20 --dt 0.0015 --nt 120 --f0 12 --pml 8 --receiver-stride 5 \
  --output output/migrated_inline.pgm
```

## Test result
- GoogleTest: **11/11 tests passed**.

## Real-data run result
- Output generated: `output/migrated_inline.pgm`
- Output SHA-256: `d085c0d0c611c4adf7dea84757148f60939b9c7648e96f90ab52fa6da2490f85`
- Output size: 781 bytes

## Input-data checksums
From `data/SHA256SUMS.txt`:
- `x.json`: `876dcab2e16cb9ae9b8b8337d93911e99f32807e7885fed92a53ff07298f19bf`
- `z.json`: `9eff7a6e86ff4939e1b36633dc7bd42da06b787dc71389cd2e3c5ba945469811`
- `vel.json`: `a0090fe3a9a19d7333526469d27aee7ddc9c6e6d33ba58004068cb6e95bc58e0`
