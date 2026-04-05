# rtm3d-cli synthetic realistic benchmark

C++20 CLI baseline for 3D acoustic RTM with a reproducible synthetic benchmark pipeline.

## Reproduce end-to-end (4 commands)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DRTM3D_BUILD_TESTS=ON && cmake --build build -j
python3 scripts/generate_synthetic_model.py --out-dir data/synthetic
./build/rtm3d_cli --config configs/synthetic_benchmark.json
python3 scripts/float32_to_png.py --input artifacts/synthetic_migrated_inline.bin --meta artifacts/synthetic_migrated_inline.bin.json --output artifacts/synthetic_migrated_inline.png
python3 scripts/visualize_synthetic.py --data-dir data/synthetic --out-dir artifacts/synthetic_preview --shot-index 1
```

Generated artifacts:
- `data/synthetic/velocity_model.bin` + `.json`
- `data/synthetic/shot_0001_gather.bin` + `.json`
- `data/synthetic/shot_0001.segy_like`
- `artifacts/synthetic_migrated_inline.bin` + `.json`
- `artifacts/synthetic_migrated_inline.png`

## Synthetic data previews

Velocity model (synthetic):

![Synthetic velocity model](docs/assets/velocity_model.png)

Shot gather (synthetic):

![Synthetic shot gather](docs/assets/shot_0001_gather.png)

## Synthetic model generator
`scripts/generate_synthetic_model.py` creates a geology-inspired velocity model with:
- depth velocity gradient
- meandering channel low-velocity body
- multiple lenses (positive/negative anomalies)
- fault-like displacement perturbation
- weak correlated heterogeneity

Outputs are float32 plus metadata JSON and JSON arrays (`x.json`, `z.json`, `vel.json`) for the existing loader path.

## Synthetic acquisition generator
The same script also creates one synthetic shot gather with:
- plausible surface geometry (central shot, regular receiver spread)
- Ricker wavelet source
- moveout-like reflectivity response + light noise

Deliverables:
- `shot_0001_gather.bin` (float32 raw, row-major `[n_receivers][nt]`)
- `shot_0001_gather.bin.json` (shape/sampling/geometry)
- `shot_0001.segy_like` (strict binary+header structure)

### SEG-Y-like note
`shot_0001.segy_like` is intentionally **SEG-Y-like**, not full standards-compliant SEG-Y:
- 3200-byte textual header (ASCII padded)
- 400-byte binary header (sample interval, samples/trace, format code)
- per trace: 240-byte trace header + IEEE float32 big-endian samples

This avoids adding new dependencies while keeping exchange-friendly structure documented and deterministic.

## Config-driven dataset path
`rtm3d_cli` supports config JSON with `data_dir` / `x_file` / `z_file` / `values_file` (no hardcoded runtime paths).
See `configs/synthetic_benchmark.json`.

## Benchmark recipes

Small/fast profile (local quick checks):
```bash
python3 scripts/generate_synthetic_model.py --out-dir data/synthetic --nx 128 --nz 80 --nt 220 --n-shots 2 --scenario layered_fault --seed 17
./build/rtm3d_cli --config configs/synthetic_benchmark.json
python3 scripts/visualize_synthetic.py --data-dir data/synthetic --out-dir artifacts/synthetic_preview --shot-index 1
```

Medium profile (richer structure + multi-shot):
```bash
python3 scripts/generate_synthetic_model.py --out-dir data/synthetic --nx 192 --nz 112 --nt 420 --n-shots 3 --scenario salt_dome --snr-db 24 --seed 17
./build/rtm3d_cli --config configs/synthetic_benchmark.json
python3 scripts/visualize_synthetic.py --data-dir data/synthetic --out-dir artifacts/synthetic_preview --shot-index 2
```

## Multi-shot RTM Migration

Run RTM with multiple source positions for improved subsurface illumination:

```bash
# CLI option
./build/rtm3d_cli --config configs/synthetic_benchmark.json --n-shots 3

# Or via config JSON
./build/rtm3d_cli --config configs/synthetic_circle_lens_multishot.json
```

Multi-shot migration stacks images from each shot position, improving signal-to-noise ratio and coverage.

### API Usage

```cpp
#include "rtm3d/rtm/RtmEngine.hpp"

rtm3d::GridModel2D model = load_model(...);
rtm3d::RtmConfig cfg;

// Single-shot (default)
auto result = rtm3d::run_single_shot_rtm(model, cfg);

// Multi-shot with custom positions
std::vector<rtm3d::ShotPosition> shots = {
    {.sx = 20, .sz = 2},
    {.sx = 40, .sz = 2},
    {.sx = 60, .sz = 2}
};
auto result = rtm3d::run_multi_shot_rtm(model, cfg, shots);
```

## CI/CD

GitHub Actions workflow runs on every push:
- Build with g++ (C++20)
- Unit tests (155 tests from 25 test suites)
- E2E synthetic benchmark
- Quality metrics validation
- Python reference cross-validation (similarity metrics)

```bash
# Local CI simulation
make test
python3 scripts/e2e_metrics.py --input artifacts/synthetic_migrated_inline.bin \
    --meta artifacts/synthetic_migrated_inline.bin.json --fail-on-threshold
```

## Canonical Devito RTM Parity (Phase 2)

Use `scripts/devito_canonical_parity.py` for a canonical parity check against `rtm3d-cli`.

This script uses Devito operators for the complete RTM chain:
1. forward propagation + source injection,
2. receiver recording,
3. reverse-time propagation + receiver injection,
4. cross-correlation imaging condition.

### Prerequisites

```bash
pip install devito numpy scipy
```

Build `rtm3d-cli` first:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Quick smoke check for canonical parity script wiring:

```bash
make parity-smoke
```

### Usage

```bash
python3 scripts/devito_canonical_parity.py \
  --nx 80 --nz 60 --dx 10 --dz 10 \
  --nt 180 --dt 0.0005 --f0 16 --pml 10 \
  --model layered --ny 1 --dy 10 \
  --cli-bin ./build/rtm3d_cli
```

Optional output (full report with schema version + generation timestamp + run config + metrics + threshold checks):

```bash
python3 scripts/devito_canonical_parity.py --metrics-out artifacts/devito_parity_metrics.json
```

CI threshold mode (returns non-zero if thresholds fail):

```bash
python3 scripts/devito_canonical_parity.py \
  --fail-on-threshold --min-ncc 0.60 --min-ssim 0.50 --max-nrmse 0.85
```

SSIM tuning (optional):

```bash
python3 scripts/devito_canonical_parity.py --ssim-window 9
```

Larger SSIM windows smooth local detail more aggressively; default is `7`.

Exit codes:
- `0`: parity run completed (and thresholds passed, if enabled)
- `1`: runtime/config error (e.g., missing Devito, invalid args)
- `2`: thresholds enabled and at least one metric failed

Reported metrics:
- **NCC** (Normalized Cross-Correlation)
- **SSIM** (Structural Similarity Index)
- **NRMSE** (Normalized RMSE)

Quick interpretation guide:
- **Good alignment**: NCC ≥ 0.60, SSIM ≥ 0.50, NRMSE ≤ 0.85
- **Excellent alignment**: NCC ≥ 0.80, SSIM ≥ 0.70, NRMSE ≤ 0.50
- **Investigate**: any metric outside the configured thresholds

### Deprecated scripts

The following scripts are kept for compatibility but are deprecated in favor of the canonical parity script:
- `scripts/devito_comparison.py`
- `scripts/compare_devito_rtm3d.py`
- `scripts/devito_validation.py`

## Tests
Quick unified local verification (build + unit + parity smoke + static):
```bash
make check
```

I/O benchmark harness quick checks:
```bash
make io-bench-test      # py_compile + unit tests for benchmark harness
make io-bench-fast      # quick deterministic run (seed=0), writes artifacts/io_format_benchmark.fast.md
make io-bench           # full deterministic run (seed=0), writes artifacts/io_format_benchmark.md
```

Unit + e2e (CTest path):
```bash
ctest --test-dir build --output-on-failure
```

E2E test (`tests/e2e_synthetic.sh`) validates generation determinism, multi-shot artifacts, migration output integrity, and quality/stability metrics.
