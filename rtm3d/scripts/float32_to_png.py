#!/usr/bin/env python3
from __future__ import annotations
import argparse
import json
import struct
import zlib
from pathlib import Path

import numpy as np


def png_gray8(path: Path, img: np.ndarray) -> None:
    h, w = img.shape
    raw = b"".join(b"\x00" + img[i].tobytes() for i in range(h))

    def chunk(tag: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    ihdr = struct.pack(">IIBBBBB", w, h, 8, 0, 0, 0, 0)
    data = zlib.compress(raw, level=9)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", data) + chunk(b"IEND", b"")
    path.write_bytes(png)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", required=True)
    ap.add_argument("--meta", required=True)
    ap.add_argument("--output", required=True)
    args = ap.parse_args()

    meta = json.loads(Path(args.meta).read_text())
    nx = int(meta["nx"])
    nz = int(meta["nz"])

    arr = np.fromfile(args.input, dtype="<f4")
    if arr.size != nx * nz:
        raise RuntimeError(f"size mismatch {arr.size} != {nx*nz}")
    img = arr.reshape(nz, nx)

    # robust amplitude scaling
    p1, p99 = np.percentile(img, [1.0, 99.0])
    if p99 <= p1:
        p1, p99 = float(np.min(img)), float(np.max(img) + 1e-6)
    norm = np.clip((img - p1) / (p99 - p1), 0.0, 1.0)
    g = (norm * 255.0).astype(np.uint8)

    out = Path(args.output)
    out.parent.mkdir(parents=True, exist_ok=True)
    png_gray8(out, g)
    print(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
