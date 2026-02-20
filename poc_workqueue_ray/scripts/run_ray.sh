#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
TASKS="${TASKS:-40}"
N="${N:-1000000}"
CPUS="${CPUS:-4}"
TOLERANCE="${TOLERANCE:-1e-3}"
MAX_RETRIES="${MAX_RETRIES:-0}"
FAIL_PROB="${FAIL_PROB:-0.0}"

python3 "$ROOT/src/ray_bag.py" \
  --tasks "$TASKS" \
  --n "$N" \
  --cpus "$CPUS" \
  --tolerance "$TOLERANCE" \
  --binary "$BUILD_DIR/compress_task" \
  --max-retries "$MAX_RETRIES" \
  --fail-prob "$FAIL_PROB" \
  > "$ROOT/results/ray.log" 2>&1

if [[ "${KEEP_OUTPUTS:-0}" != "1" ]]; then
  rm -f "$ROOT"/results/ray_out_*.bin
fi
