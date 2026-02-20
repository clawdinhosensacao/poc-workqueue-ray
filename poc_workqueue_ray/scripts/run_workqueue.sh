#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CCTOOLS="$ROOT/../cctools/work_queue/src"
PORT="${PORT:-9123}"
WORKERS="${WORKERS:-4}"
TASKS="${TASKS:-40}"
N="${N:-1000000}"

cd "$ROOT/bin"

MANAGER_LOG="$ROOT/results/workqueue_manager.log"
WORKER_LOG="$ROOT/results/workqueue_workers.log"

rm -f "$MANAGER_LOG" "$WORKER_LOG"

# Start manager
./workqueue_manager --port "$PORT" --tasks "$TASKS" --n "$N" > "$MANAGER_LOG" 2>&1 &
MANAGER_PID=$!

sleep 1

# Start local workers
for i in $(seq 1 "$WORKERS"); do
  "$CCTOOLS/work_queue_worker" -N wq-zfp-poc localhost "$PORT" >> "$WORKER_LOG" 2>&1 &
  echo $! >> "$ROOT/results/workers.pids"
done

wait "$MANAGER_PID"
STATUS=$?

# Cleanup workers
if [[ -f "$ROOT/results/workers.pids" ]]; then
  while read -r pid; do
    kill "$pid" 2>/dev/null || true
  done < "$ROOT/results/workers.pids"
  rm -f "$ROOT/results/workers.pids"
fi

exit "$STATUS"
