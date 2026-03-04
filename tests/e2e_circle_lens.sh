#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

# Verify circle_lens scenario runs and produces valid output
./build/rtm3d_cli --config configs/synthetic_circle_lens.json

python3 - <<'PY'
import json
import numpy as np
from pathlib import Path

output = Path('output/migrated_inline.pgm')
assert output.exists(), 'missing circle_lens output'

# Basic sanity: file has content
assert output.stat().st_size > 100, 'output too small'

print('e2e_circle_lens_ok')
PY
