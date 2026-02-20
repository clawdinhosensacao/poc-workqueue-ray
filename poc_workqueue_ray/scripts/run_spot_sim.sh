#!/usr/bin/env bash
set -euo pipefail

# Spot/preemptible simulation:
# 1) Work Queue: kill one worker mid-run and verify completion.
# 2) Ray: inject random task failures and enable retries.

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CCTOOLS="${CCTOOLS_BIN_DIR:-$ROOT/../cctools/work_queue/src}"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
TASKS="${TASKS:-30}"
N="${N:-500000}"
WORKERS="${WORKERS:-4}"
PORT="${SPOT_PORT:-9444}"
TOLERANCE="${TOLERANCE:-1e-3}"

WQ_LOG="$ROOT/results/spot_workqueue_manager.log"
WK_LOG="$ROOT/results/spot_workqueue_workers.log"
RAY_LOG="$ROOT/results/spot_ray.log"
PID_FILE="$ROOT/results/spot_workers.pids"
OUT="$ROOT/results/spot_sim_summary.log"

rm -f "$WQ_LOG" "$WK_LOG" "$RAY_LOG" "$PID_FILE" "$OUT"
cp -f "$BUILD_DIR/compress_task" "$BUILD_DIR/workqueue_manager" "$ROOT/results/"
cd "$ROOT/results"

# --- Work Queue preemption simulation ---
./workqueue_manager --port "$PORT" --tasks "$TASKS" --n "$N" --tolerance "$TOLERANCE" --binary compress_task > "$WQ_LOG" 2>&1 &
MANAGER_PID=$!
sleep 1

for _ in $(seq 1 "$WORKERS"); do
  "$CCTOOLS/work_queue_worker" -N wq-zfp-poc localhost "$PORT" >> "$WK_LOG" 2>&1 &
  echo $! >> "$PID_FILE"
done

# Simulate preemption: kill first worker after warmup
sleep 2
PREEMPT_PID=$(head -n1 "$PID_FILE" || true)
if [[ -n "${PREEMPT_PID:-}" ]]; then
  kill "$PREEMPT_PID" 2>/dev/null || true
  echo "spot_event workqueue_preempted_worker_pid=$PREEMPT_PID" >> "$WK_LOG"
fi

set +e
wait "$MANAGER_PID"
WQ_STATUS=$?
set -e

if [[ -f "$PID_FILE" ]]; then
  while read -r pid; do kill "$pid" 2>/dev/null || true; done < "$PID_FILE"
  rm -f "$PID_FILE"
fi

# --- Ray preemption simulation ---
set +e
TASKS="$TASKS" N="$N" CPUS="$WORKERS" TOLERANCE="$TOLERANCE" MAX_RETRIES=5 FAIL_PROB=0.10 \
  "$ROOT/scripts/run_ray.sh"
RAY_STATUS=$?
set -e
cp -f "$ROOT/results/ray.log" "$RAY_LOG"

{
  echo "spot_sim_summary"
  echo "workqueue_exit=$WQ_STATUS"
  grep 'wq_summary' "$WQ_LOG" || true
  echo "ray_exit=$RAY_STATUS"
  grep 'ray_summary' "$RAY_LOG" || true
} | tee "$OUT"

# non-zero only if either failed outright
if [[ "$WQ_STATUS" -ne 0 || "$RAY_STATUS" -ne 0 ]]; then
  exit 1
fi
