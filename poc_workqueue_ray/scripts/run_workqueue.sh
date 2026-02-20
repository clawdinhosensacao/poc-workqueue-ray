#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CCTOOLS="${CCTOOLS_BIN_DIR:-$ROOT/../cctools/work_queue/src}"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
PORT="${WQ_PORT:-$((20000 + (RANDOM % 10000)))}"
WORKERS="${WORKERS:-4}"
TASKS="${TASKS:-40}"
N="${N:-1000000}"
TOLERANCE="${TOLERANCE:-1e-3}"

MANAGER_LOG="$ROOT/results/workqueue_manager.log"
WORKER_LOG="$ROOT/results/workqueue_workers.log"
PID_FILE="$ROOT/results/workers.pids"

rm -f "$MANAGER_LOG" "$WORKER_LOG" "$PID_FILE"
cp -f "$BUILD_DIR/compress_task" "$BUILD_DIR/workqueue_manager" "$ROOT/results/"
cd "$ROOT/results"

./workqueue_manager --port "$PORT" --tasks "$TASKS" --n "$N" --tolerance "$TOLERANCE" --binary compress_task > "$MANAGER_LOG" 2>&1 &
MANAGER_PID=$!
sleep 1

for _ in $(seq 1 "$WORKERS"); do
  "$CCTOOLS/work_queue_worker" -N wq-zfp-poc localhost "$PORT" >> "$WORKER_LOG" 2>&1 &
  echo $! >> "$PID_FILE"
done

set +e
wait "$MANAGER_PID"
STATUS=$?
set -e

if [[ -f "$PID_FILE" ]]; then
  while read -r pid; do kill "$pid" 2>/dev/null || true; done < "$PID_FILE"
  rm -f "$PID_FILE"
fi

exit "$STATUS"
