#!/usr/bin/env bash
set -euo pipefail

# Simulates an on-prem SLURM-style execution plan.
# If sbatch/srun exist, uses them. Otherwise falls back to local emulation.

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/results/slurm_sim.log"
TASKS="${TASKS:-24}"
N="${N:-400000}"
WORKERS="${WORKERS:-4}"
CPUS="${CPUS:-4}"

{
  echo "[slurm-sim] start $(date -Is)"
  if command -v sbatch >/dev/null 2>&1 && command -v srun >/dev/null 2>&1; then
    echo "[slurm-sim] Detected SLURM tools; using srun wrappers"
    srun -N1 -n1 bash -lc "TASKS=$TASKS N=$N WORKERS=$WORKERS $ROOT/scripts/run_workqueue.sh"
    srun -N1 -n1 bash -lc "TASKS=$TASKS N=$N CPUS=$CPUS $ROOT/scripts/run_ray.sh"
  else
    echo "[slurm-sim] SLURM not detected, running local emulation"
    TASKS=$TASKS N=$N WORKERS=$WORKERS "$ROOT/scripts/run_workqueue.sh"
    TASKS=$TASKS N=$N CPUS=$CPUS "$ROOT/scripts/run_ray.sh"
  fi
  python3 "$ROOT/scripts/summarize_results.py"
  echo "[slurm-sim] done $(date -Is)"
} | tee "$OUT"
