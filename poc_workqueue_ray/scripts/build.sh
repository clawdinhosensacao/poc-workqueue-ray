#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
TOOLS_DIR="$ROOT/third_party/tools"
CMAKE_VERSION="${CMAKE_VERSION:-3.29.6}"

if command -v cmake >/dev/null 2>&1; then
  CMAKE_BIN="$(command -v cmake)"
else
  mkdir -p "$TOOLS_DIR"
  CMAKE_DIR="$TOOLS_DIR/cmake-${CMAKE_VERSION}-linux-x86_64"
  CMAKE_TGZ="$TOOLS_DIR/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz"
  if [[ ! -x "$CMAKE_DIR/bin/cmake" ]]; then
    curl -fL "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz" -o "$CMAKE_TGZ"
    tar -xzf "$CMAKE_TGZ" -C "$TOOLS_DIR"
  fi
  CMAKE_BIN="$CMAKE_DIR/bin/cmake"
fi

mkdir -p "$BUILD_DIR" "$ROOT/results"

"$CMAKE_BIN" -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
"$CMAKE_BIN" --build "$BUILD_DIR" --parallel

echo "Build complete: $BUILD_DIR"
