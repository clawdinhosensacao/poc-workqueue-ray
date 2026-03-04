#!/usr/bin/env python3
"""Quick visualizer for synthetic benchmark artifacts.

Generates PNG previews for:
- velocity model (velocity_model.bin + velocity_model.bin.json)
- shot gather (shot_0001_gather.bin + shot_0001_gather.bin.json)

No third-party plotting dependency required.
"""
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
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(png)


def to_uint8(arr: np.ndarray, p_lo: float = 1.0, p_hi: float = 99.0) -> np.ndarray:
    lo, hi = np.percentile(arr, [p_lo, p_hi])
    if hi <= lo:
        lo, hi = float(np.min(arr)), float(np.max(arr) + 1e-6)
    norm = np.clip((arr - lo) / (hi - lo), 0.0, 1.0)
    return (norm * 255.0).astype(np.uint8)


def visualize_model(data_dir: Path, out_dir: Path) -> Path:
    meta = json.loads((data_dir / "velocity_model.bin.json").read_text())
    nx = int(meta["nx"])
    nz = int(meta["nz"])
    model = np.fromfile(data_dir / "velocity_model.bin", dtype="<f4")
    if model.size != nx * nz:
        raise RuntimeError(f"velocity shape mismatch {model.size} != {nx*nz}")
    img = model.reshape(nz, nx)
    out = out_dir / "velocity_model.png"
    png_gray8(out, to_uint8(img, 1.0, 99.0))
    return out


def visualize_gather(data_dir: Path, out_dir: Path, shot_index: int) -> Path:
    stem = f"shot_{shot_index:04d}_gather"
    meta = json.loads((data_dir / f"{stem}.bin.json").read_text())
    nrec = int(meta["n_receivers"])
    nt = int(meta["nt"])
    g = np.fromfile(data_dir / f"{stem}.bin", dtype="<f4")
    if g.size != nrec * nt:
        raise RuntimeError(f"gather shape mismatch {g.size} != {nrec*nt}")

    # Stored as [n_receivers][nt]; plot as [time][receiver] for readability.
    gather = g.reshape(nrec, nt).T
    out = out_dir / f"shot_{shot_index:04d}_gather.png"
    png_gray8(out, to_uint8(gather, 0.5, 99.5))
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--data-dir", default="data/synthetic", help="Synthetic data directory")
    ap.add_argument("--out-dir", default="artifacts/synthetic_preview", help="Output preview directory")
    ap.add_argument("--shot-index", type=int, default=1, help="Shot index to preview (1-based)")
    args = ap.parse_args()

    data_dir = Path(args.data_dir)
    out_dir = Path(args.out_dir)

    if args.shot_index < 1:
        raise ValueError("--shot-index must be >= 1")

    model_png = visualize_model(data_dir, out_dir)
    gather_png = visualize_gather(data_dir, out_dir, shot_index=args.shot_index)

    print(model_png)
    print(gather_png)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
