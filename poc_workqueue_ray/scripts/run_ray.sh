#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TASKS="${TASKS:-40}"
N="${N:-1000000}"
CPUS="${CPUS:-4}"

python3 "$ROOT/src/ray_bag.py" --tasks "$TASKS" --n "$N" --cpus "$CPUS" > "$ROOT/results/ray.log" 2>&1
