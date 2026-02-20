#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

mkdir -p data/synthetic artifacts

python3 scripts/generate_synthetic_model.py --out-dir data/synthetic --nx 160 --nz 96 --nt 280 --dt 0.001 --f0 16
./build/rtm3d_cli --config configs/synthetic_benchmark.json
python3 scripts/float32_to_png.py --input artifacts/synthetic_migrated_inline.bin --meta artifacts/synthetic_migrated_inline.bin.json --output artifacts/synthetic_migrated_inline.png

python3 - <<'PY'
import json
import numpy as np
from pathlib import Path

out = Path('artifacts/synthetic_migrated_inline.bin')
meta = Path('artifacts/synthetic_migrated_inline.bin.json')
png = Path('artifacts/synthetic_migrated_inline.png')
assert out.exists(), 'missing float output'
assert meta.exists(), 'missing metadata'
assert png.exists(), 'missing png preview'

m = json.loads(meta.read_text())
arr = np.fromfile(out, dtype='<f4')
assert arr.size == m['nx'] * m['nz'], 'shape mismatch'
assert np.isfinite(arr).all(), 'non-finite values'
energy = float(np.sum(np.abs(arr)))
assert energy > 0.0, 'zero energy image'
print('e2e_ok energy=', energy)
PY
