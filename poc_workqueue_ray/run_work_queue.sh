#!/usr/bin/env bash
set -euo pipefail

TASKS=${1:-40}
BYTES=${2:-4194304}
WORKERS=${3:-4}
PORT=${4:-9123}
RUN_ID=${5:-$(date +%Y%m%d_%H%M%S)}

ROOT=$(cd "$(dirname "$0")" && pwd)
OUTDIR="$ROOT/runs/$RUN_ID/work_queue"
mkdir -p "$OUTDIR/results" "$OUTDIR/logs"

make -C "$ROOT" all >"$OUTDIR/logs/build.log" 2>&1

MANAGER_LOG="$OUTDIR/logs/manager.log"
WORKER_BIN="$ROOT/../cctools/work_queue/src/work_queue_worker"

"$ROOT/wq_manager" "$TASKS" "$BYTES" "$PORT" "$OUTDIR/results" >"$MANAGER_LOG" 2>&1 &
MANAGER_PID=$!

sleep 1

PIDS=()
for i in $(seq 1 "$WORKERS"); do
  "$WORKER_BIN" 127.0.0.1 "$PORT" >"$OUTDIR/logs/worker_$i.log" 2>&1 &
  PIDS+=("$!")
done

set +e
wait "$MANAGER_PID"
MGR_RC=$?
set -e

for p in "${PIDS[@]}"; do
  kill "$p" >/dev/null 2>&1 || true
done

SUMMARY_LINE=$(grep '^SUMMARY ' "$MANAGER_LOG" | tail -n 1 || true)
echo "$SUMMARY_LINE" | tee "$OUTDIR/summary.txt"
echo "manager_rc=$MGR_RC" | tee -a "$OUTDIR/summary.txt"
echo "output_dir=$OUTDIR" | tee -a "$OUTDIR/summary.txt"

exit "$MGR_RC"
