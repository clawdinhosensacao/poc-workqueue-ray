#!/usr/bin/env python3
"""Generate realistic-ish synthetic benchmark data for rtm3d-cli.

Outputs:
- model axis/value JSON (for current C++ loader)
- model float32 raw + metadata JSON
- one synthetic shot gather float32 raw + metadata JSON
- SEG-Y-like deliverable (textual header + binary header + trace headers + IEEE float samples)
"""
from __future__ import annotations

import argparse
import json
import math
import os
import struct
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import List, Tuple

import numpy as np


@dataclass
class ModelMeta:
    nx: int
    nz: int
    dx: float
    dz: float
    dtype: str = "float32"
    order: str = "row-major [nz][nx]"
    units: str = "m/s"


@dataclass
class GatherMeta:
    n_receivers: int
    nt: int
    dt: float
    shot_x: float
    shot_z: float
    receiver_x0: float
    receiver_dx: float
    dtype: str = "float32"
    order: str = "row-major [n_receivers][nt]"
    units: str = "arbitrary amplitude"


def ricker(nt: int, dt: float, f0: float) -> np.ndarray:
    t = np.arange(nt, dtype=np.float32) * dt
    t0 = 1.0 / f0
    a = math.pi * f0 * (t - t0)
    a2 = a * a
    return ((1.0 - 2.0 * a2) * np.exp(-a2)).astype(np.float32)


def gaussian2d(xx: np.ndarray, zz: np.ndarray, x0: float, z0: float, sx: float, sz: float) -> np.ndarray:
    return np.exp(-(((xx - x0) / sx) ** 2 + ((zz - z0) / sz) ** 2)).astype(np.float32)


def build_velocity(nx: int, nz: int, dx: float, dz: float, seed: int) -> np.ndarray:
    rng = np.random.default_rng(seed)
    x = np.arange(nx, dtype=np.float32) * dx
    z = np.arange(nz, dtype=np.float32) * dz
    xx, zz = np.meshgrid(x, z)

    # Layered depth trend (compaction)
    vel = (1500.0 + 0.85 * zz).astype(np.float32)

    # Meandering channel (slow anomaly)
    center = 0.55 * nx * dx + 0.12 * nx * dx * np.sin(2.0 * math.pi * zz / (nz * dz * 0.8))
    channel = np.exp(-((xx - center) / (0.10 * nx * dx)) ** 2).astype(np.float32)
    vel -= 220.0 * channel * np.exp(-((zz - 0.35 * nz * dz) / (0.22 * nz * dz)) ** 2)

    # Lenses (fast and slow)
    for _ in range(5):
        x0 = rng.uniform(0.15, 0.85) * nx * dx
        z0 = rng.uniform(0.2, 0.8) * nz * dz
        sx = rng.uniform(0.05, 0.14) * nx * dx
        sz = rng.uniform(0.05, 0.14) * nz * dz
        amp = rng.uniform(-220.0, 260.0)
        vel += amp * gaussian2d(xx, zz, x0, z0, sx, sz)

    # Fault-like offset perturbation
    fault_x = 0.62 * nx * dx
    throw = 0.035 * nz * dz
    shifted_zz = np.where(xx > fault_x, zz + throw, zz)
    fault_term = (0.22 * shifted_zz - 0.22 * zz).astype(np.float32)
    vel += fault_term

    # Small correlated heterogeneity
    noise = rng.normal(0.0, 1.0, size=(nz, nx)).astype(np.float32)
    noise = (noise + np.roll(noise, 1, 0) + np.roll(noise, -1, 0) + np.roll(noise, 1, 1) + np.roll(noise, -1, 1)) / 5.0
    vel += 35.0 * noise

    return np.clip(vel, 1450.0, 5200.0).astype(np.float32)


def synthesize_gather(vel: np.ndarray, dx: float, dz: float, nt: int, dt: float, f0: float) -> Tuple[np.ndarray, GatherMeta]:
    nz, nx = vel.shape
    nrec = max(24, nx // 2)
    shot_ix = nx // 2
    shot_iz = 2

    rec_ix0 = 2
    rec_dx = max(1, (nx - 4) // nrec)
    rec_ix = np.array([min(nx - 3, rec_ix0 + i * rec_dx) for i in range(nrec)], dtype=np.int32)

    w = ricker(nt, dt, f0)
    g = np.zeros((nrec, nt), dtype=np.float32)

    # Single-scattering-inspired synthetic moveout from shallow reflectivity proxy
    imp = np.diff(vel, axis=0, prepend=vel[:1, :])
    imp = imp / (np.max(np.abs(imp)) + 1e-8)

    z_samples = np.arange(6, nz - 4, max(2, nz // 40), dtype=np.int32)
    for ir, rx in enumerate(rec_ix):
        offset = abs(rx - shot_ix) * dx
        for iz in z_samples:
            refl = float(imp[iz, min(nx - 1, (rx + shot_ix) // 2)])
            if abs(refl) < 0.04:
                continue
            vm = float(np.mean(vel[max(1, iz - 2):iz + 2, max(1, min(rx, shot_ix) - 2):min(nx - 1, max(rx, shot_ix) + 2)]))
            twt = 2.0 * math.sqrt((iz * dz) ** 2 + (0.5 * offset) ** 2) / max(vm, 1200.0)
            it0 = int(round(twt / dt))
            if it0 >= nt:
                continue
            i1 = min(nt, it0 + len(w))
            g[ir, it0:i1] += refl * w[: i1 - it0]

    # weak coherent noise + geometric decay shaping
    time_gain = (1.0 + 0.5 * np.linspace(0, 1, nt, dtype=np.float32)).reshape(1, -1)
    g *= time_gain
    g += 0.02 * np.random.default_rng(123).normal(size=g.shape).astype(np.float32)

    meta = GatherMeta(
        n_receivers=int(nrec),
        nt=int(nt),
        dt=float(dt),
        shot_x=float(shot_ix * dx),
        shot_z=float(shot_iz * dz),
        receiver_x0=float(rec_ix[0] * dx),
        receiver_dx=float(rec_dx * dx),
    )
    return g, meta


def write_segy_like(path: Path, gather: np.ndarray, meta: GatherMeta) -> None:
    """Write SEG-Y-like file without external deps.

    Layout:
    - 3200-byte textual header (ASCII padded)
    - 400-byte binary header (big-endian, key fields set)
    - per trace: 240-byte trace header + nt float32 big-endian samples
    """
    ntr, nt = gather.shape
    dt_us = int(round(meta.dt * 1e6))

    with path.open("wb") as f:
        text = (
            "C01 RTM3D SYNTHETIC SEG-Y-LIKE FILE\n"
            "C02 NOT FULL SEG-Y COMPLIANCE; CORE HEADER+TRACE STRUCTURE ONLY\n"
            "C03 SAMPLES: IEEE FLOAT32 BIG-ENDIAN\n"
            "C04 GENERATED BY scripts/generate_synthetic_model.py\n"
        ).encode("ascii", errors="ignore")[:3200]
        f.write(text.ljust(3200, b" "))

        bh = bytearray(400)
        struct.pack_into(">h", bh, 16, dt_us)  # sample interval
        struct.pack_into(">h", bh, 20, nt)  # samples/trace
        struct.pack_into(">h", bh, 24, 5)  # data sample format code 5 = IEEE float (commonly used ext)
        f.write(bh)

        for tr in range(ntr):
            th = bytearray(240)
            struct.pack_into(">i", th, 0, tr + 1)
            struct.pack_into(">i", th, 20, tr + 1)
            struct.pack_into(">i", th, 36, int(meta.shot_x))
            struct.pack_into(">i", th, 40, int(meta.receiver_x0 + tr * meta.receiver_dx))
            struct.pack_into(">h", th, 114, nt)
            struct.pack_into(">h", th, 116, dt_us)
            f.write(th)
            f.write(gather[tr].astype(">f4", copy=False).tobytes())


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out-dir", default="data/synthetic")
    ap.add_argument("--nx", type=int, default=192)
    ap.add_argument("--nz", type=int, default=112)
    ap.add_argument("--dx", type=float, default=10.0)
    ap.add_argument("--dz", type=float, default=10.0)
    ap.add_argument("--nt", type=int, default=420)
    ap.add_argument("--dt", type=float, default=0.001)
    ap.add_argument("--f0", type=float, default=18.0)
    ap.add_argument("--seed", type=int, default=7)
    args = ap.parse_args()

    out = Path(args.out_dir)
    out.mkdir(parents=True, exist_ok=True)

    vel = build_velocity(args.nx, args.nz, args.dx, args.dz, args.seed)
    x = (np.arange(args.nx, dtype=np.float32) * args.dx).tolist()
    z = (np.arange(args.nz, dtype=np.float32) * args.dz).tolist()

    # Existing loader path (JSON)
    (out / "x.json").write_text(json.dumps(x))
    (out / "z.json").write_text(json.dumps(z))
    (out / "vel.json").write_text(json.dumps(vel.tolist()))

    # Float32 model deliverable
    (out / "velocity_model.bin").write_bytes(vel.astype("<f4", copy=False).tobytes())
    (out / "velocity_model.bin.json").write_text(json.dumps(asdict(ModelMeta(args.nx, args.nz, args.dx, args.dz)), indent=2))

    gather, gmeta = synthesize_gather(vel, args.dx, args.dz, args.nt, args.dt, args.f0)
    (out / "shot_0001_gather.bin").write_bytes(gather.astype("<f4", copy=False).tobytes())
    (out / "shot_0001_gather.bin.json").write_text(json.dumps(asdict(gmeta), indent=2))

    write_segy_like(out / "shot_0001.segy_like", gather, gmeta)

    print(f"Synthetic benchmark written to {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
