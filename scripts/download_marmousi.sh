#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="${1:-data}"
mkdir -p "$OUT_DIR"

BASE="https://raw.githubusercontent.com/mlamoureux/Marmousi/master"

curl -fsSL "$BASE/x.json" -o "$OUT_DIR/x.json"
curl -fsSL "$BASE/z.json" -o "$OUT_DIR/z.json"
curl -fsSL "$BASE/vel.json" -o "$OUT_DIR/vel.json"

sha256sum "$OUT_DIR/x.json" "$OUT_DIR/z.json" "$OUT_DIR/vel.json" > "$OUT_DIR/SHA256SUMS.txt"

echo "Downloaded Marmousi arrays to $OUT_DIR"
echo "Checksums saved to $OUT_DIR/SHA256SUMS.txt"
