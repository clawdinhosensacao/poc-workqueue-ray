#!/usr/bin/env python3
"""
Canonical Devito RTM parity pipeline vs rtm3d-cli.

This script implements a full RTM reference path using Devito operators:
1) Forward modeling with source injection
2) Receiver recording
3) Reverse-time propagation with receiver injection
4) Cross-correlation imaging condition

It then runs rtm3d-cli on the same model and reports parity metrics:
- NCC (normalized cross-correlation)
- SSIM (structural similarity)
- NRMSE (normalized RMSE)

Usage example:
    python3 scripts/devito_canonical_parity.py --nx 80 --nz 60 --nt 180 --dt 0.0005 --f0 16 --pml 10
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np


def normalized_cross_correlation(a: np.ndarray, b: np.ndarray) -> float:
    a0 = a - np.mean(a)
    b0 = b - np.mean(b)
    den = np.linalg.norm(a0) * np.linalg.norm(b0)
    if den < 1e-12:
        return 0.0
    return float(np.sum(a0 * b0) / den)


def normalized_rmse(a: np.ndarray, b: np.ndarray) -> float:
    rmse = np.sqrt(np.mean((a - b) ** 2))
    denom = np.max(a) - np.min(a)
    if abs(denom) < 1e-12:
        return float("inf") if rmse > 1e-12 else 0.0
    return float(rmse / denom)


def ssim_simple(a: np.ndarray, b: np.ndarray, window_size: int = 7) -> float:
    try:
        from scipy.ndimage import uniform_filter
    except Exception:
        return (normalized_cross_correlation(a, b) + 1.0) * 0.5

    a = a.astype(np.float64)
    b = b.astype(np.float64)

    c1 = 0.01 ** 2
    c2 = 0.03 ** 2

    mu_a = uniform_filter(a, size=window_size, mode="reflect")
    mu_b = uniform_filter(b, size=window_size, mode="reflect")

    sigma_a_sq = uniform_filter(a * a, size=window_size, mode="reflect") - mu_a * mu_a
    sigma_b_sq = uniform_filter(b * b, size=window_size, mode="reflect") - mu_b * mu_b
    sigma_ab = uniform_filter(a * b, size=window_size, mode="reflect") - mu_a * mu_b

    num = (2.0 * mu_a * mu_b + c1) * (2.0 * sigma_ab + c2)
    den = (mu_a * mu_a + mu_b * mu_b + c1) * (sigma_a_sq + sigma_b_sq + c2)
    return float(np.mean(num / (den + 1e-12)))


def build_velocity(model: str, nz: int, nx: int, vmin: float, vmax: float) -> np.ndarray:
    if model == "constant":
        return np.full((nz, nx), vmin, dtype=np.float32)
    if model == "layered":
        vel = np.zeros((nz, nx), dtype=np.float32)
        for iz in range(nz):
            alpha = iz / max(1, nz - 1)
            vel[iz, :] = vmin + alpha * (vmax - vmin)
        return vel
    if model == "circle":
        vel = np.full((nz, nx), vmin, dtype=np.float32)
        cx, cz = nx // 2, nz // 2
        radius = max(4, min(nx, nz) // 6)
        for iz in range(nz):
            for ix in range(nx):
                if (ix - cx) ** 2 + (iz - cz) ** 2 <= radius ** 2:
                    vel[iz, ix] = vmax
        return vel
    raise ValueError(f"Unsupported model: {model}")


def make_damp(nz: int, nx: int, pml: int) -> np.ndarray:
    damp = np.zeros((nz, nx), dtype=np.float32)
    if pml <= 0:
        return damp

    for iz in range(nz):
        for ix in range(nx):
            di = min(ix, nx - 1 - ix, iz, nz - 1 - iz)
            if di < pml:
                x = (pml - di) / float(pml)
                damp[iz, ix] = 1.5 * x * x
    return damp


def run_devito_canonical_rtm(
    velocity: np.ndarray,
    dx: float,
    dz: float,
    nt: int,
    dt: float,
    f0: float,
    pml: int,
    src_x: int,
    src_z: int,
    rec_z: int,
    space_order: int,
) -> np.ndarray:
    try:
        from devito import Grid, Function, TimeFunction, SparseTimeFunction, Eq, Operator, solve
    except ImportError as exc:
        raise RuntimeError("Devito not available. Install with: pip install devito") from exc

    nz, nx = velocity.shape
    grid = Grid(shape=(nz, nx), extent=((nz - 1) * dz, (nx - 1) * dx))

    m = Function(name="m", grid=grid, space_order=space_order)
    m.data[:] = (1.0 / np.maximum(velocity, 1.0) ** 2).astype(np.float32)

    damp = Function(name="damp", grid=grid)
    damp.data[:] = make_damp(nz, nx, pml)

    u = TimeFunction(name="u", grid=grid, time_order=2, space_order=space_order, save=nt)

    src = SparseTimeFunction(name="src", grid=grid, npoint=1, nt=nt)
    src.coordinates.data[0, 0] = src_z * dz
    src.coordinates.data[0, 1] = src_x * dx

    t = np.arange(nt, dtype=np.float32) * dt
    t0 = 1.5 / f0
    tau = t - t0
    src.data[:, 0] = ((1.0 - 2.0 * (np.pi * f0 * tau) ** 2) * np.exp(-(np.pi * f0 * tau) ** 2)).astype(np.float32)

    rec = SparseTimeFunction(name="rec", grid=grid, npoint=nx, nt=nt)
    rec.coordinates.data[:, 0] = rec_z * dz
    rec.coordinates.data[:, 1] = np.arange(nx, dtype=np.float32) * dx

    pde = m * u.dt2 - u.laplace + damp * u.dt
    u_fwd = Eq(u.forward, solve(pde, u.forward))

    fwd_op = Operator([u_fwd] + src.inject(field=u.forward, expr=src * dt**2 / m) + rec.interpolate(expr=u))
    fwd_op(time_m=0, time_M=nt - 2, dt=dt)

    v = TimeFunction(name="v", grid=grid, time_order=2, space_order=space_order, save=nt)
    image = Function(name="image", grid=grid)

    pde_bwd = m * v.dt2 - v.laplace + damp * v.dt.T
    v_bwd = Eq(v.backward, solve(pde_bwd, v.backward))

    img_eq = Eq(image, image + u * v)
    bwd_op = Operator([v_bwd] + rec.inject(field=v.backward, expr=rec * dt**2 / m) + [img_eq])
    bwd_op(time_m=1, time_M=nt - 2, dt=dt)

    return np.array(image.data, copy=True)


def run_rtm3d_cli(
    velocity: np.ndarray,
    dx: float,
    dz: float,
    nt: int,
    dt: float,
    f0: float,
    pml: int,
    ny: int,
    dy: float,
    cli_bin: str,
) -> np.ndarray:
    nz, nx = velocity.shape

    with tempfile.TemporaryDirectory() as tmpdir:
        td = Path(tmpdir)
        (td / "x.json").write_text(json.dumps((np.arange(nx) * dx).tolist()))
        (td / "z.json").write_text(json.dumps((np.arange(nz) * dz).tolist()))
        (td / "vel.json").write_text(json.dumps(velocity.tolist()))

        config = {
            "data_dir": str(td),
            "ny": int(ny),
            "dy": float(dy),
            "nt": int(nt),
            "dt": float(dt),
            "f0": float(f0),
            "pml": int(pml),
            "n_shots": 1,
            "output": str(td / "migrated"),
            "output_format": "float32_raw",
        }
        (td / "config.json").write_text(json.dumps(config))

        run = subprocess.run(
            [cli_bin, "--config", str(td / "config.json")],
            capture_output=True,
            text=True,
            timeout=300,
        )
        if run.returncode != 0:
            raise RuntimeError(f"rtm3d_cli failed: {run.stderr or run.stdout}")

        out = td / "migrated"
        if not out.exists():
            raise RuntimeError("rtm3d_cli did not produce output file")

        return np.fromfile(out, dtype=np.float32).reshape((nz, nx))


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Canonical Devito RTM parity against rtm3d-cli")
    p.add_argument("--nx", type=int, default=80)
    p.add_argument("--nz", type=int, default=60)
    p.add_argument("--dx", type=float, default=10.0)
    p.add_argument("--dz", type=float, default=10.0)
    p.add_argument("--nt", type=int, default=180)
    p.add_argument("--dt", type=float, default=0.0005)
    p.add_argument("--f0", type=float, default=16.0)
    p.add_argument("--pml", type=int, default=10)
    p.add_argument("--ny", type=int, default=1)
    p.add_argument("--dy", type=float, default=10.0)
    p.add_argument("--model", choices=["constant", "layered", "circle"], default="layered")
    p.add_argument("--vmin", type=float, default=1500.0)
    p.add_argument("--vmax", type=float, default=3000.0)
    p.add_argument("--src-x", type=int, default=-1)
    p.add_argument("--src-z", type=int, default=2)
    p.add_argument("--rec-z", type=int, default=2)
    p.add_argument("--space-order", type=int, default=4)
    p.add_argument("--cli-bin", type=str, default="./build/rtm3d_cli")
    p.add_argument("--metrics-out", type=str, default="")
    return p.parse_args()


def main() -> int:
    args = parse_args()

    if args.src_x < 0:
        args.src_x = args.nx // 2

    velocity = build_velocity(args.model, args.nz, args.nx, args.vmin, args.vmax)

    print("[1/3] Running canonical Devito RTM pipeline...")
    devito_image = run_devito_canonical_rtm(
        velocity=velocity,
        dx=args.dx,
        dz=args.dz,
        nt=args.nt,
        dt=args.dt,
        f0=args.f0,
        pml=args.pml,
        src_x=args.src_x,
        src_z=args.src_z,
        rec_z=args.rec_z,
        space_order=args.space_order,
    )

    print("[2/3] Running rtm3d-cli on same model...")
    cli_image = run_rtm3d_cli(
        velocity=velocity,
        dx=args.dx,
        dz=args.dz,
        nt=args.nt,
        dt=args.dt,
        f0=args.f0,
        pml=args.pml,
        ny=args.ny,
        dy=args.dy,
        cli_bin=args.cli_bin,
    )

    print("[3/3] Computing parity metrics...")
    # z-score normalize both before comparison to reduce scale bias
    a = (devito_image - np.mean(devito_image)) / (np.std(devito_image) + 1e-10)
    b = (cli_image - np.mean(cli_image)) / (np.std(cli_image) + 1e-10)

    metrics = {
        "ncc": normalized_cross_correlation(a, b),
        "ssim": ssim_simple(a, b),
        "nrmse": normalized_rmse(a, b),
    }

    print(json.dumps(metrics, indent=2))

    if args.metrics_out:
        Path(args.metrics_out).write_text(json.dumps(metrics, indent=2))
        print(f"Saved metrics to: {args.metrics_out}")

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
