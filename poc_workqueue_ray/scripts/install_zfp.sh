#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TP="$ROOT/third_party"
TOOLS="$TP/tools"
ZFP_SRC="$TP/zfp-src"
ZFP_BUILD="$TP/zfp-build"
ZFP_PREFIX="$TP/zfp-install"
ZFP_REF="${ZFP_REF:-1.0.1}"
CMAKE_VERSION="${CMAKE_VERSION:-3.29.6}"

mkdir -p "$TP" "$TOOLS"

CMAKE_BIN="$(command -v cmake || true)"
if [[ -z "$CMAKE_BIN" ]]; then
  CMAKE_TGZ="$TOOLS/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz"
  CMAKE_DIR="$TOOLS/cmake-${CMAKE_VERSION}-linux-x86_64"
  if [[ ! -x "$CMAKE_DIR/bin/cmake" ]]; then
    curl -fL "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz" -o "$CMAKE_TGZ"
    tar -xzf "$CMAKE_TGZ" -C "$TOOLS"
  fi
  CMAKE_BIN="$CMAKE_DIR/bin/cmake"
fi

if [[ ! -d "$ZFP_SRC/.git" ]]; then
  git clone --depth 1 --branch "$ZFP_REF" https://github.com/LLNL/zfp.git "$ZFP_SRC"
else
  git -C "$ZFP_SRC" fetch --tags --depth 1 origin "$ZFP_REF" || true
  git -C "$ZFP_SRC" checkout "$ZFP_REF"
fi

"$CMAKE_BIN" -S "$ZFP_SRC" -B "$ZFP_BUILD" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$ZFP_PREFIX" \
  -DBUILD_SHARED_LIBS=OFF \
  -DZFP_WITH_OPENMP=OFF \
  -DZFP_WITH_CUDA=OFF

"$CMAKE_BIN" --build "$ZFP_BUILD" --parallel
"$CMAKE_BIN" --install "$ZFP_BUILD"

echo "Installed zfp to $ZFP_PREFIX"
