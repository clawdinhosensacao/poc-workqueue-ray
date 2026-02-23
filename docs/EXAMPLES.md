# RTM Examples Gallery

This document demonstrates the capabilities of `rtm3d-cli` with various Devito-inspired velocity model presets.

## Overview

Each example shows:
1. **Velocity model**: Generated using `generate_synthetic_model.py` with preset scenarios
2. **RTM migration**: Run using `rtm3d_cli` with multi-shot support
3. **Output**: Migrated inline XZ slice in PGM format

## Preset Models (Devito-inspired)

| Preset | Description | Use Case |
|--------|-------------|----------|
| `constant` | Uniform velocity field | Basic testing, calibration |
| `layers` | N-layer model with velocity gradient | Stratigraphic imaging |
| `circle` | Circular anomaly (camembert) | Diffraction testing |
| `circle_lens` | Gaussian lens anomaly | Focusing effects |
| `salt_dome` | Salt body with complex structure | Subsalt imaging |
| `layered_fault` | Layered model with fault | Structural imaging |

---

## Example 1: Constant Velocity

**Purpose**: Basic sanity check and calibration

```bash
# Generate model
python3 scripts/generate_synthetic_model.py \
    --out-dir data/demo_constant \
    --nx 80 --nz 48 --nt 100 \
    --scenario constant \
    --vp-top 2000 --n-shots 1

# Run RTM
./build/rtm3d_cli --config configs/demo_constant.json
```

**Velocity**: Uniform 2000 m/s

**Output**: `output/demos/constant.pgm`

---

## Example 2: Layered Model

**Purpose**: Stratigraphic imaging with velocity gradient

```bash
# Generate model
python3 scripts/generate_synthetic_model.py \
    --out-dir data/demo_layers \
    --nx 80 --nz 48 --nt 100 \
    --scenario layers \
    --vp-top 1500 --vp-bottom 3500 \
    --n-shots 3

# Run RTM (multi-shot)
./build/rtm3d_cli --config configs/demo_layers.json
```

**Velocity**: 3 layers from 1500 m/s (top) to 3500 m/s (bottom)

**Multi-shot**: 3 sources for improved illumination

**Output**: `output/demos/layers.pgm`

---

## Example 3: Circle (Camembert)

**Purpose**: Diffraction and imaging of circular anomaly

```bash
# Generate model
python3 scripts/generate_synthetic_model.py \
    --out-dir data/demo_circle \
    --nx 80 --nz 48 --nt 100 \
    --scenario circle \
    --vp-top 1500 --vp-bottom 2500 \
    --n-shots 1

# Run RTM
./build/rtm3d_cli --config configs/demo_circle.json
```

**Velocity**: 1500 m/s background with 2500 m/s circular anomaly

**Output**: `output/demos/circle.pgm`

---

## Example 4: Circle Lens (Gaussian)

**Purpose**: Focusing effects from smooth velocity anomaly

```bash
# Generate model
python3 scripts/generate_synthetic_model.py \
    --out-dir data/demo_circle_lens \
    --nx 80 --nz 48 --nt 100 \
    --scenario circle_lens \
    --vp-top 1800 --vp-bottom 2800 \
    --n-shots 2

# Run RTM (multi-shot)
./build/rtm3d_cli --config configs/demo_circle_lens.json
```

**Velocity**: Gaussian-shaped anomaly (smooth transition)

**Output**: `output/demos/circle_lens.pgm`

---

## Example 5: Salt Dome

**Purpose**: Subsalt imaging with complex velocity structure

```bash
# Generate model
python3 scripts/generate_synthetic_model.py \
    --out-dir data/demo_salt_dome \
    --nx 100 --nz 60 --nt 120 \
    --scenario salt_dome \
    --n-shots 2

# Run RTM (multi-shot)
./build/rtm3d_cli --config configs/demo_salt_dome.json
```

**Velocity**: Complex salt body with high-velocity anomaly (~4500 m/s)

**Output**: `output/demos/salt_dome.pgm`

---

## Quick Start

Run all demos with a single command:

```bash
# Generate all demo models
for preset in constant layers circle circle_lens salt_dome; do
    python3 scripts/generate_synthetic_model.py \
        --out-dir data/demo_${preset} \
        --scenario ${preset} \
        --n-shots 2
done

# Run RTM on all demos
for preset in constant layers circle circle_lens salt_dome; do
    ./build/rtm3d_cli --config configs/demo_${preset}.json
    cp output/migrated_inline.pgm output/demos/${preset}.pgm
done
```

## Output Files

Each run produces:
- `*.pgm` - Migrated inline XZ slice (grayscale image)
- `*.pgm.json` - Metadata (nx, nz, dx, dz)
- `velocity_model.bin` - Float32 velocity model
- `shot_*_gather.bin` - Synthetic shot gathers

## API Usage

```cpp
#include "rtm3d/model/SeismicModel.hpp"
#include "rtm3d/rtm/RtmEngine.hpp"

// Create model from preset (Devito-inspired)
rtm3d::GridSpec grid{.nx = 80, .nz = 48, .ny = 1, .dx = 10.0f, .dz = 10.0f};
auto model = rtm3d::SeismicModel::from_preset(
    rtm3d::ModelPreset::CircleLens, grid, 1500.0f, 2500.0f);

// Configure RTM
rtm3d::RtmConfig cfg;
cfg.nt = 100;
cfg.dt = 0.0008f;
cfg.f0 = 16.0f;

// Run multi-shot RTM
std::vector<rtm3d::ShotPosition> shots = {{.sx = 20, .sz = 2}, {.sx = 60, .sz = 2}};
auto result = rtm3d::run_multi_shot_rtm(model, cfg, shots);
```

## Performance Notes

| Model Size | Shots | Time (approx) |
|------------|-------|---------------|
| 80x48      | 1     | ~0.1s         |
| 80x48      | 3     | ~0.3s         |
| 100x60     | 2     | ~0.2s         |
| 160x96     | 3     | ~1.0s         |

*Timings on typical laptop CPU*
