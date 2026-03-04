#!/usr/bin/env python3
"""Generate synthetic benchmark data for rtm3d-cli.

Devito-inspired design:
- Preset models: constant, layers, circle, circle_lens, salt_dome, layered_fault
- Clean separation of velocity model and acquisition geometry
- Reproducible via seed control

Outputs:
- velocity_model.bin + .json (float32 raw)
- shot_XXXX_gather.bin + .json (synthetic shot records)
- shot_XXXX.segy_like (SEG-Y-like format)
"""
from __future__ import annotations

import argparse
import json
import math
import struct
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Tuple

import numpy as np


# =============================================================================
# Model Metadata (Devito-inspired)
# =============================================================================

@dataclass
class ModelMeta:
    """Velocity model metadata."""
    nx: int
    nz: int
    dx: float
    dz: float
    scenario: str
    dtype: str = "float32"
    order: str = "row-major [nz][nx]"
    units: str = "m/s"
    v_min: float = 0.0
    v_max: float = 0.0


@dataclass
class GatherMeta:
    """Shot gather metadata."""
    n_receivers: int
    nt: int
    dt: float
    shot_index: int
    shot_x: float
    shot_z: float
    receiver_x0: float
    receiver_dx: float
    snr_db: float
    noise_seed: int
    dtype: str = "float32"
    order: str = "row-major [n_receivers][nt]"
    units: str = "arbitrary amplitude"


# =============================================================================
# Velocity Models (Devito preset_models.py inspired)
# =============================================================================

def gardners_density(vp: np.ndarray) -> np.ndarray:
    """Gardner's relation for density from P-wave velocity (km/s)."""
    vp_kms = vp / 1000.0
    rho = 0.31 * np.power(vp_kms, 0.25) * 1000.0  # kg/m³
    rho = np.where(vp < 1500.0, 1000.0, rho)  # Water density
    return rho.astype(np.float32)


def constant_model(nx: int, nz: int, vp: float = 1500.0) -> np.ndarray:
    """Constant velocity model (Devito: constant-isotropic)."""
    return np.full((nz, nx), vp, dtype=np.float32)


def layers_model(nx: int, nz: int, dx: float, dz: float,
                 vp_top: float = 1500.0, vp_bottom: float = 3500.0,
                 nlayers: int = 3) -> np.ndarray:
    """Layered velocity model (Devito: layers-isotropic)."""
    vp_layers = np.linspace(vp_top, vp_bottom, nlayers)
    vel = np.zeros((nz, nx), dtype=np.float32)
    layer_height = nz // nlayers
    for i, vp in enumerate(vp_layers):
        z_start = i * layer_height
        z_end = (i + 1) * layer_height if i < nlayers - 1 else nz
        vel[z_start:z_end, :] = vp
    return vel


def circle_model(nx: int, nz: int, dx: float, dz: float,
                 vp_bg: float = 1500.0, vp_anomaly: float = 2500.0,
                 radius_frac: float = 0.25) -> np.ndarray:
    """Circular anomaly - camembert model (Devito: circle-isotropic)."""
    vel = np.full((nz, nx), vp_bg, dtype=np.float32)
    cx, cz = nx / 2.0, nz / 2.0
    radius = min(nx, nz) * radius_frac
    
    for k in range(nz):
        for i in range(nx):
            dist = math.sqrt((i - cx)**2 + (k - cz)**2)
            if dist <= radius:
                vel[k, i] = vp_anomaly
    return vel


def circle_lens_model(nx: int, nz: int, dx: float, dz: float,
                      vp_bg: float = 2000.0, vp_anomaly: float = 2800.0,
                      cx_frac: float = 0.48, cz_frac: float = 0.52,
                      sx_frac: float = 0.14, sz_frac: float = 0.14) -> np.ndarray:
    """Gaussian lens anomaly (smooth velocity variation)."""
    vel = np.full((nz, nx), vp_bg, dtype=np.float32)
    cx, cz = nx * cx_frac, nz * cz_frac
    sx, sz = nx * sx_frac, nz * sz_frac
    
    for k in range(nz):
        for i in range(nx):
            dx_norm = (i - cx) / sx
            dz_norm = (k - cz) / sz
            gaussian = math.exp(-(dx_norm**2 + dz_norm**2))
            vel[k, i] = vp_bg + (vp_anomaly - vp_bg) * gaussian
    return vel


def salt_dome_model(nx: int, nz: int, dx: float, dz: float,
                    seed: int = 17) -> np.ndarray:
    """Complex salt dome model with realistic features."""
    rng = np.random.default_rng(seed)
    x = np.arange(nx, dtype=np.float32) * dx
    z = np.arange(nz, dtype=np.float32) * dz
    xx, zz = np.meshgrid(x, z)

    # Depth trend (compaction)
    vel = 1500.0 + 0.85 * zz + 55.0 * np.sin(2.0 * math.pi * xx / (nx * dx * 1.4))
    vel = vel.astype(np.float32)

    # Meandering channel
    center = 0.55 * nx * dx + 0.12 * nx * dx * np.sin(2.0 * math.pi * zz / (nz * dz * 0.8))
    channel = np.exp(-((xx - center) / (0.10 * nx * dx)) ** 2)
    vel -= 220.0 * channel * np.exp(-((zz - 0.35 * nz * dz) / (0.22 * nz * dz)) ** 2)

    # Salt body
    salt = gaussian2d(xx, zz, 0.52 * nx * dx, 0.58 * nz * dz, 0.11 * nx * dx, 0.12 * nz * dz)
    salt_top = 1.0 / (1.0 + np.exp(-(zz - 0.38 * nz * dz) / max(1.0, 0.03 * nz * dz)))
    vel += 620.0 * salt * salt_top.astype(np.float32)

    # Random lenses
    for _ in range(5):
        x0 = rng.uniform(0.15, 0.85) * nx * dx
        z0 = rng.uniform(0.2, 0.8) * nz * dz
        sx = rng.uniform(0.05, 0.14) * nx * dx
        sz = rng.uniform(0.05, 0.14) * nz * dz
        amp = rng.uniform(-220.0, 260.0)
        vel += amp * gaussian2d(xx, zz, x0, z0, sx, sz)

    # Fault
    fault_x = 0.62 * nx * dx
    throw = 0.035 * nz * dz
    shifted_zz = np.where(xx > fault_x, zz + throw, zz)
    vel += (0.22 * shifted_zz - 0.22 * zz).astype(np.float32)

    # Small heterogeneity
    noise = rng.normal(0.0, 1.0, size=(nz, nx)).astype(np.float32)
    noise = (noise + np.roll(noise, 1, 0) + np.roll(noise, -1, 0) + 
             np.roll(noise, 1, 1) + np.roll(noise, -1, 1)) / 5.0
    vel += 35.0 * noise

    return np.clip(vel, 1450.0, 5200.0).astype(np.float32)


def layered_fault_model(nx: int, nz: int, dx: float, dz: float,
                        seed: int = 17) -> np.ndarray:
    """Layered model with fault structure."""
    rng = np.random.default_rng(seed)
    x = np.arange(nx, dtype=np.float32) * dx
    z = np.arange(nz, dtype=np.float32) * dz
    xx, zz = np.meshgrid(x, z)

    # Base layered velocity
    vel = layers_model(nx, nz, dx, dz, vp_top=1600.0, vp_bottom=3200.0, nlayers=5)

    # Add gentle dip
    dip = 0.15 * zz
    vel += 40.0 * np.sin(2.0 * math.pi * dip / (nz * dz))

    # Fault offset
    fault_x = 0.55 * nx * dx
    throw = 0.04 * nz * dz
    shifted_zz = np.where(xx > fault_x, zz + throw, zz)
    vel += (0.18 * shifted_zz - 0.18 * zz).astype(np.float32)

    # Noise
    noise = rng.normal(0.0, 1.0, size=(nz, nx)).astype(np.float32)
    noise = (noise + np.roll(noise, 1, 0) + np.roll(noise, -1, 0)) / 3.0
    vel += 25.0 * noise

    return np.clip(vel, 1500.0, 4000.0).astype(np.float32)


def gaussian2d(xx: np.ndarray, zz: np.ndarray, x0: float, z0: float, 
                sx: float, sz: float) -> np.ndarray:
    """2D Gaussian function."""
    return np.exp(-(((xx - x0) / sx) ** 2 + ((zz - z0) / sz) ** 2)).astype(np.float32)


# =============================================================================
# Acquisition Geometry (Devito-inspired)
# =============================================================================

@dataclass
class AcquisitionGeometry:
    """Source-receiver geometry configuration."""
    # Source position
    shot_ix: int
    shot_iz: int = 2
    
    # Receiver spread
    rec_first_ix: int = 2
    rec_last_ix: int = 0  # 0 means nx - 3
    rec_dix: int = 1  # receiver spacing in grid points
    rec_iz: int = 2  # receiver depth
    
    @property
    def n_receivers(self) -> int:
        return (self.rec_last_ix - self.rec_first_ix) // self.rec_dix + 1


def default_acquisition(nx: int, nz: int, shot_index: int = 1, 
                        n_shots: int = 1) -> AcquisitionGeometry:
    """Create default acquisition geometry."""
    # Distribute shots across the model
    shot_positions = np.linspace(nx // 4, 3 * nx // 4, n_shots, dtype=int)
    shot_ix = int(shot_positions[min(shot_index - 1, n_shots - 1)])
    
    return AcquisitionGeometry(
        shot_ix=shot_ix,
        shot_iz=2,
        rec_first_ix=2,
        rec_last_ix=nx - 3,
        rec_dix=max(1, (nx - 4) // max(24, nx // 4)),
        rec_iz=2
    )


# =============================================================================
# Wavelet and Synthetic Gather Generation
# =============================================================================

def ricker_wavelet(nt: int, dt: float, f0: float) -> np.ndarray:
    """Ricker wavelet (Mexican hat)."""
    t = np.arange(nt, dtype=np.float32) * dt
    t0 = 1.0 / f0
    a = math.pi * f0 * (t - t0)
    a2 = a * a
    return ((1.0 - 2.0 * a2) * np.exp(-a2)).astype(np.float32)


def synthesize_gather(vel: np.ndarray, dx: float, dz: float,
                      acq: AcquisitionGeometry,
                      nt: int, dt: float, f0: float,
                      seed: int, snr_db: float = 26.0,
                      shot_index: int = 1) -> Tuple[np.ndarray, GatherMeta]:
    """Generate synthetic shot gather using single-scattering approximation."""
    nz, nx = vel.shape
    
    # Receiver positions
    rec_last = acq.rec_last_ix if acq.rec_last_ix > 0 else nx - 3
    rec_ix = list(range(acq.rec_first_ix, rec_last + 1, acq.rec_dix))
    nrec = len(rec_ix)
    
    # Wavelet
    w = ricker_wavelet(nt, dt, f0)
    
    # Impedance (reflectivity proxy)
    imp = np.diff(vel, axis=0, prepend=vel[:1, :])
    imp = imp / (np.max(np.abs(imp)) + 1e-8)
    
    # Generate gather
    g = np.zeros((nrec, nt), dtype=np.float32)
    z_samples = np.arange(6, nz - 4, max(2, nz // 40), dtype=int)
    
    for ir, rx in enumerate(rec_ix):
        offset = abs(rx - acq.shot_ix) * dx
        for iz in z_samples:
            # Reflectivity at midpoint
            mid_x = (rx + acq.shot_ix) // 2
            refl = float(imp[iz, min(nx - 1, mid_x)])
            if abs(refl) < 0.04:
                continue
            
            # Travel time (straight ray approximation)
            vm = float(np.mean(vel[max(1, iz-2):iz+2, 
                                   max(1, min(rx, acq.shot_ix)-2):min(nx-1, max(rx, acq.shot_ix)+2)]))
            twt = 2.0 * math.sqrt((iz * dz)**2 + (0.5 * offset)**2) / max(vm, 1200.0)
            it0 = int(round(twt / dt))
            
            if it0 >= nt:
                continue
            i1 = min(nt, it0 + len(w))
            g[ir, it0:i1] += refl * w[:i1 - it0]
    
    # Time gain
    time_gain = (1.0 + 0.5 * np.linspace(0, 1, nt, dtype=np.float32)).reshape(1, -1)
    g *= time_gain
    
    # Add noise
    rng = np.random.default_rng(seed + 101)
    noise = rng.normal(size=g.shape).astype(np.float32)
    signal_rms = float(np.sqrt(np.mean(g * g) + 1e-12))
    noise_rms = float(np.sqrt(np.mean(noise * noise) + 1e-12))
    target_noise_rms = signal_rms / (10.0 ** (snr_db / 20.0))
    g += noise * (target_noise_rms / noise_rms)
    
    meta = GatherMeta(
        n_receivers=nrec,
        nt=nt,
        dt=dt,
        shot_index=shot_index,
        shot_x=float(acq.shot_ix * dx),
        shot_z=float(acq.shot_iz * dz),
        receiver_x0=float(rec_ix[0] * dx),
        receiver_dx=float(acq.rec_dix * dx),
        snr_db=snr_db,
        noise_seed=seed + 101
    )
    
    return g, meta


# =============================================================================
# Output Writers
# =============================================================================

def write_segy_like(path: Path, gather: np.ndarray, meta: GatherMeta) -> None:
    """Write SEG-Y-like file (simplified format)."""
    ntr, nt = gather.shape
    dt_us = int(round(meta.dt * 1e6))
    
    with path.open("wb") as f:
        # Textual header (3200 bytes)
        text = (
            "C01 RTM3D SYNTHETIC SEG-Y-LIKE FILE\n"
            "C02 GENERATED BY generate_synthetic_model.py\n"
            f"C03 SHOT INDEX: {meta.shot_index}\n"
        ).encode("ascii", errors="ignore")[:3200]
        f.write(text.ljust(3200, b" "))
        
        # Binary header (400 bytes)
        bh = bytearray(400)
        struct.pack_into(">h", bh, 16, dt_us)
        struct.pack_into(">h", bh, 20, nt)
        struct.pack_into(">h", bh, 24, 5)  # IEEE float
        f.write(bh)
        
        # Traces
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


# =============================================================================
# Main Entry Point
# =============================================================================

PRESET_MODELS = {
    "constant": constant_model,
    "layers": layers_model,
    "circle": circle_model,
    "circle_lens": circle_lens_model,
    "salt_dome": salt_dome_model,
    "layered_fault": layered_fault_model,
}


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate synthetic RTM benchmark data")
    ap.add_argument("--out-dir", default="data/synthetic")
    ap.add_argument("--nx", type=int, default=160)
    ap.add_argument("--nz", type=int, default=96)
    ap.add_argument("--dx", type=float, default=10.0)
    ap.add_argument("--dz", type=float, default=10.0)
    ap.add_argument("--nt", type=int, default=280)
    ap.add_argument("--dt", type=float, default=0.001)
    ap.add_argument("--f0", type=float, default=16.0)
    ap.add_argument("--seed", type=int, default=17)
    ap.add_argument("--scenario", choices=list(PRESET_MODELS.keys()), default="salt_dome",
                    help="Velocity model preset (Devito-inspired)")
    ap.add_argument("--snr-db", type=float, default=26.0)
    ap.add_argument("--n-shots", type=int, default=1)
    ap.add_argument("--vp-top", type=float, default=1500.0, help="Top velocity for layered model")
    ap.add_argument("--vp-bottom", type=float, default=3500.0, help="Bottom velocity for layered model")
    args = ap.parse_args()
    
    out = Path(args.out_dir)
    out.mkdir(parents=True, exist_ok=True)
    
    # Build velocity model
    model_fn = PRESET_MODELS[args.scenario]
    if args.scenario == "constant":
        vel = model_fn(args.nx, args.nz, args.vp_top)
    elif args.scenario == "circle":
        vel = model_fn(args.nx, args.nz, args.dx, args.dz, args.vp_top, args.vp_bottom)
    elif args.scenario in ("layers",):
        vel = model_fn(args.nx, args.nz, args.dx, args.dz, args.vp_top, args.vp_bottom)
    else:
        vel = model_fn(args.nx, args.nz, args.dx, args.dz, args.seed)
    
    # Output velocity model
    x = (np.arange(args.nx, dtype=np.float32) * args.dx).tolist()
    z = (np.arange(args.nz, dtype=np.float32) * args.dz).tolist()
    
    (out / "x.json").write_text(json.dumps(x))
    (out / "z.json").write_text(json.dumps(z))
    (out / "vel.json").write_text(json.dumps(vel.tolist()))
    
    (out / "velocity_model.bin").write_bytes(vel.astype("<f4", copy=False).tobytes())
    meta = ModelMeta(
        nx=args.nx, nz=args.nz, dx=args.dx, dz=args.dz,
        scenario=args.scenario,
        v_min=float(vel.min()), v_max=float(vel.max())
    )
    (out / "velocity_model.bin.json").write_text(json.dumps(asdict(meta), indent=2))
    
    # Generate shot gathers
    shot_positions = np.linspace(args.nx // 4, 3 * args.nx // 4, args.n_shots, dtype=int)
    
    for i, shot_ix in enumerate(shot_positions, start=1):
        acq = AcquisitionGeometry(shot_ix=int(shot_ix))
        gather, gmeta = synthesize_gather(
            vel, args.dx, args.dz, acq,
            args.nt, args.dt, args.f0,
            args.seed + i - 1, args.snr_db, i
        )
        
        stem = f"shot_{i:04d}"
        (out / f"{stem}_gather.bin").write_bytes(gather.astype("<f4", copy=False).tobytes())
        (out / f"{stem}_gather.bin.json").write_text(json.dumps(asdict(gmeta), indent=2))
        write_segy_like(out / f"{stem}.segy_like", gather, gmeta)
    
    print(f"Generated {args.scenario} model ({args.nx}x{args.nz}) with {args.n_shots} shot(s) in {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
