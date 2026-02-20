#!/usr/bin/env bash
set -euo pipefail

TASKS=${1:-40}
BYTES=${2:-4194304}
WORKERS=${3:-4}
RUN_ID=${4:-$(date +%Y%m%d_%H%M%S)}

ROOT=$(cd "$(dirname "$0")" && pwd)
OUTDIR="$ROOT/runs/$RUN_ID/ray"
mkdir -p "$OUTDIR"

python3 "$ROOT/ray_benchmark.py" --tasks "$TASKS" --bytes "$BYTES" --workers "$WORKERS" --out "$OUTDIR/ray_results.json" | tee "$OUTDIR/ray.log"
