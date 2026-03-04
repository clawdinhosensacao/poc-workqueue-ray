#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

mkdir -p data/synthetic artifacts

python3 scripts/generate_synthetic_model.py --out-dir data/synthetic --nx 160 --nz 96 --nt 280 --dt 0.001 --f0 16 --seed 17 --n-shots 2
sha_a="$(sha256sum data/synthetic/shot_0001_gather.bin | awk '{print $1}')"
python3 scripts/generate_synthetic_model.py --out-dir data/synthetic --nx 160 --nz 96 --nt 280 --dt 0.001 --f0 16 --seed 17 --n-shots 2
sha_b="$(sha256sum data/synthetic/shot_0001_gather.bin | awk '{print $1}')"
[[ "$sha_a" == "$sha_b" ]] || { echo "non-deterministic synthetic gather for same seed"; exit 1; }

./build/rtm3d_cli --config configs/synthetic_benchmark.json
python3 scripts/float32_to_png.py --input artifacts/synthetic_migrated_inline.bin --meta artifacts/synthetic_migrated_inline.bin.json --output artifacts/synthetic_migrated_inline.png
cp artifacts/synthetic_migrated_inline.bin artifacts/synthetic_migrated_inline_seed17.bin
cp artifacts/synthetic_migrated_inline.bin.json artifacts/synthetic_migrated_inline_seed17.bin.json

python3 scripts/generate_synthetic_model.py --out-dir data/synthetic --nx 160 --nz 96 --nt 280 --dt 0.001 --f0 16 --seed 18 --n-shots 2
./build/rtm3d_cli --config configs/synthetic_benchmark.json
cp artifacts/synthetic_migrated_inline.bin artifacts/synthetic_migrated_inline_seed18.bin
cp artifacts/synthetic_migrated_inline.bin.json artifacts/synthetic_migrated_inline_seed18.bin.json

python3 - <<'PY'
import json
import numpy as np
from pathlib import Path

seed17 = Path('artifacts/synthetic_migrated_inline_seed17.bin')
seed18 = Path('artifacts/synthetic_migrated_inline_seed18.bin')
meta17 = Path('artifacts/synthetic_migrated_inline_seed17.bin.json')
png = Path('artifacts/synthetic_migrated_inline.png')
shot2 = Path('data/synthetic/shot_0002_gather.bin')
shot2_meta = Path('data/synthetic/shot_0002_gather.bin.json')
assert seed17.exists(), 'missing seed17 float output'
assert seed18.exists(), 'missing seed18 float output'
assert meta17.exists(), 'missing metadata'
assert png.exists(), 'missing png preview'
assert shot2.exists(), 'missing second-shot gather artifact'
assert shot2_meta.exists(), 'missing second-shot metadata'

m = json.loads(meta17.read_text())
a = np.fromfile(seed17, dtype='<f4')
b = np.fromfile(seed18, dtype='<f4')
assert a.size == m['nx'] * m['nz'], 'shape mismatch seed17'
assert b.size == a.size, 'shape mismatch seed18'
assert np.isfinite(a).all() and np.isfinite(b).all(), 'non-finite values'

# 1) normalized energy
energy_norm = float(np.mean(np.abs(a)))
assert energy_norm > 1e-5, f'low normalized energy: {energy_norm}'

# 2) focus proxy: gradient energy ratio
ax = a.reshape(m['nz'], m['nx'])
gx = np.diff(ax, axis=1)
gz = np.diff(ax, axis=0)
focus = float((np.mean(np.abs(gx)) + np.mean(np.abs(gz))) / (np.mean(np.abs(ax)) + 1e-8))
assert focus > 0.08, f'focus too low: {focus}'

# 3) cross-seed stability: similar total energy (not identical image)
energy_a = float(np.sum(np.abs(a)))
energy_b = float(np.sum(np.abs(b)))
ratio = abs(energy_a - energy_b) / max(energy_a, energy_b, 1e-8)
assert ratio < 0.35, f'seed stability energy drift too high: {ratio}'

shot2m = json.loads(shot2_meta.read_text())
assert int(shot2m.get('shot_index', -1)) == 2, 'invalid shot_index for second shot'

print('e2e_ok energy_norm=', energy_norm, 'focus=', focus, 'seed_energy_drift=', ratio)
PY
