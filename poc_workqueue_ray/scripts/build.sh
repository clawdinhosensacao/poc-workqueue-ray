#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CCTOOLS="$ROOT/../cctools"

mkdir -p "$ROOT/bin" "$ROOT/results"

# build payload task (C++)
g++ -O3 -std=c++17 "$ROOT/src/compress_task.cpp" -lz -o "$ROOT/bin/compress_task"

# build Work Queue manager (C++ with CCTools static libs)
g++ -O2 -std=c++17 "$ROOT/src/workqueue_manager.cpp" \
  -I"$CCTOOLS/work_queue/src" \
  -I"$CCTOOLS/dttools/src" \
  "$CCTOOLS/work_queue/src/libwork_queue.a" \
  "$CCTOOLS/dttools/src/libdttools.a" \
  -lm -ldl -pthread -lz -o "$ROOT/bin/workqueue_manager"

echo "Build complete."
