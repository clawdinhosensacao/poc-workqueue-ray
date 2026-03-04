#!/bin/bash
#
# Quick visualization workflow: generate synthetic data + migrate + convert to PNG
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

# Default parameters
SCENARIO="${SCENARIO:-salt_dome}"
NX="${NX:-80}"
NZ="${NZ:-48}"
NT="${NT:-100}"
N_SHOTS="${N_SHOTS:-1}"
SEED="${SEED:-42}"
OUT_DIR="${OUT_DIR:-output/quick_viz}"

echo "=== Quick Visualization Workflow ==="
echo "Scenario: $SCENARIO | Size: ${NX}x${NZ} | Shots: $N_SHOTS | Seed: $SEED"

# 1. Generate synthetic model
echo "[1/4] Generating synthetic model..."
python3 scripts/generate_synthetic_model.py \
    --out-dir data/synthetic \
    --nx "$NX" --nz "$NZ" --nt "$NT" \
    --scenario "$SCENARIO" \
    --n-shots "$N_SHOTS" \
    --seed "$SEED"

# 2. Run RTM migration
echo "[2/4] Running RTM migration..."
./build/rtm3d_cli --config configs/synthetic_benchmark.json

# 3. Convert to PNG
echo "[3/4] Converting to PNG..."
mkdir -p "$OUT_DIR"
python3 scripts/float32_to_png.py \
    --input artifacts/synthetic_migrated_inline.bin \
    --meta artifacts/synthetic_migrated_inline.bin.json \
    --output "$OUT_DIR/migrated.png"

# 4. Compute metrics
echo "[4/4] Computing quality metrics..."
python3 scripts/e2e_metrics.py \
    --input artifacts/synthetic_migrated_inline.bin \
    --meta artifacts/synthetic_migrated_inline.bin.json \
    --output "$OUT_DIR/metrics.json"

echo ""
echo "=== Done! ==="
echo "Output: $OUT_DIR/"
ls -la "$OUT_DIR/"
