# rtm3d-cli synthetic realistic benchmark

C++20 CLI baseline for 3D acoustic RTM with a reproducible synthetic benchmark pipeline.

## Reproduce end-to-end (4 commands)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DRTM3D_BUILD_TESTS=ON && cmake --build build -j
python3 scripts/generate_synthetic_model.py --out-dir data/synthetic
./build/rtm3d_cli --config configs/synthetic_benchmark.json
python3 scripts/float32_to_png.py --input artifacts/synthetic_migrated_inline.bin --meta artifacts/synthetic_migrated_inline.bin.json --output artifacts/synthetic_migrated_inline.png
```

Generated artifacts:
- `data/synthetic/velocity_model.bin` + `.json`
- `data/synthetic/shot_0001_gather.bin` + `.json`
- `data/synthetic/shot_0001.segy_like`
- `artifacts/synthetic_migrated_inline.bin` + `.json`
- `artifacts/synthetic_migrated_inline.png`

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

## Tests
Unit + e2e:
```bash
ctest --test-dir build --output-on-failure
```

E2E test (`tests/e2e_synthetic.sh`) validates: generation → migration of one shot setup → existence of outputs, shape consistency, finite values, non-zero energy.
