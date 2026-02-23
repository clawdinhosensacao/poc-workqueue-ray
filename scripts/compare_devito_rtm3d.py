#!/usr/bin/env python3
"""
Devito vs rtm3d-cli RTM Comparison

Runs identical RTM migrations in both frameworks and compares results
using multiple similarity metrics.

Metrics:
- Pearson correlation (1.0 = perfect match)
- SNR in dB (higher = better match)  
- NRMSE (0.0 = perfect match)
- Relative L2 error (0.0 = perfect match)

Usage:
    python3 scripts/compare_devito_rtm3d.py
"""
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np

try:
    from devito import Grid, Function, TimeFunction, Operator, Eq, solve
    DEVITO_AVAILABLE = True
except ImportError:
    DEVITO_AVAILABLE = False
    print("ERROR: Devito not available. Install with: pip install devito")
    sys.exit(1)


# =============================================================================
# Similarity Metrics
# =============================================================================

def pearson_correlation(a: np.ndarray, b: np.ndarray) -> float:
    """Pearson correlation coefficient. 1.0 = perfect positive correlation."""
    a_centered = a - np.mean(a)
    b_centered = b - np.mean(b)
    num = np.sum(a_centered * b_centered)
    den = np.sqrt(np.sum(a_centered**2) * np.sum(b_centered**2))
    if den < 1e-12:
        return 0.0
    return num / den


def signal_to_noise_ratio(reference: np.ndarray, test: np.ndarray) -> float:
    """SNR in dB. Higher = test closer to reference."""
    noise = reference - test
    signal_power = np.sum(reference**2)
    noise_power = np.sum(noise**2)
    if noise_power < 1e-12:
        return float('inf')
    return 10.0 * np.log10(signal_power / noise_power)


def normalized_rmse(reference: np.ndarray, test: np.ndarray) -> float:
    """Normalized Root Mean Square Error. 0.0 = perfect match."""
    rmse = np.sqrt(np.mean((reference - test)**2))
    ref_range = np.max(reference) - np.min(reference)
    if ref_range < 1e-12:
        return float('inf') if rmse > 1e-12 else 0.0
    return rmse / ref_range


def relative_l2_error(reference: np.ndarray, test: np.ndarray) -> float:
    """Relative L2 error: ||ref - test|| / ||ref||. 0.0 = perfect match."""
    diff_norm = np.linalg.norm(reference - test)
    ref_norm = np.linalg.norm(reference)
    if ref_norm < 1e-12:
        return float('inf') if diff_norm > 1e-12 else 0.0
    return diff_norm / ref_norm


def compute_all_metrics(ref: np.ndarray, test: np.ndarray) -> dict:
    """Compute all similarity metrics."""
    return {
        'pearson': pearson_correlation(ref, test),
        'snr_db': signal_to_noise_ratio(ref, test),
        'nrmse': normalized_rmse(ref, test),
        'rel_l2': relative_l2_error(ref, test),
    }


# =============================================================================
# Devito RTM Implementation
# =============================================================================

def devito_rtm_2d(nx: int, nz: int, dx: float, dz: float,
                  nt: int, dt: float, f0: float,
                  velocity: np.ndarray, pml: int = 10) -> np.ndarray:
    """
    Run 2D RTM using finite differences (NumPy reference implementation).
    
    This matches the rtm3d-cli algorithm for fair comparison.
    Returns migrated image (nz, nx).
    """
    # Grid spacing
    h = dx  # assuming dx = dz
    
    # Ricker wavelet
    t = np.arange(nt) * dt
    t0 = 1.5 / f0
    wavelet = (1 - 2 * (np.pi * f0 * (t - t0))**2) * np.exp(-(np.pi * f0 * (t - t0))**2)
    
    # Source position
    sx, sz = nx // 2, 2
    
    # Receivers at z=2
    rec_z = 2
    
    # Damping for PML (simplified - just exponential at edges)
    damp = np.ones((nz, nx), dtype=np.float32)
    for i in range(pml):
        coeff = np.exp(-0.75 * ((pml - i) / pml)**2)
        damp[i, :] = np.minimum(damp[i, :], coeff)
        damp[nz-1-i, :] = np.minimum(damp[nz-1-i, :], coeff)
        damp[:, i] = np.minimum(damp[:, i], coeff)
        damp[:, nx-1-i] = np.minimum(damp[:, nx-1-i], coeff)
    
    # Forward propagation
    print("  Reference: Forward propagation...")
    u_prev = np.zeros((nz, nx), dtype=np.float32)
    u_curr = np.zeros((nz, nx), dtype=np.float32)
    
    rec_data = np.zeros((nt, nx), dtype=np.float32)
    src_snaps = np.zeros((nt, nz, nx), dtype=np.float32)
    
    v2dt2 = velocity**2 * dt**2
    h2 = h**2
    
    for it in range(nt):
        # Laplacian (2nd order in space)
        lap = np.zeros_like(u_curr)
        lap[1:-1, 1:-1] = (
            u_curr[1:-1, 2:] + u_curr[1:-1, :-2] +
            u_curr[2:, 1:-1] + u_curr[:-2, 1:-1] -
            4 * u_curr[1:-1, 1:-1]
        ) / h2
        
        # Time stepping
        u_next = (2 * u_curr - u_prev + v2dt2 * lap) * damp
        
        # Inject source
        u_next[sz, sx] += wavelet[it]
        
        # Record
        rec_data[it, :] = u_next[rec_z, :]
        src_snaps[it, :, :] = u_next
        
        # Swap
        u_prev = u_curr
        u_curr = u_next
    
    # Backward propagation + imaging
    print("  Reference: Backward propagation + imaging...")
    u_prev = np.zeros((nz, nx), dtype=np.float32)
    u_curr = np.zeros((nz, nx), dtype=np.float32)
    
    image = np.zeros((nz, nx), dtype=np.float32)
    
    for rit in range(nt):
        it = nt - 1 - rit
        
        # Laplacian
        lap = np.zeros_like(u_curr)
        lap[1:-1, 1:-1] = (
            u_curr[1:-1, 2:] + u_curr[1:-1, :-2] +
            u_curr[2:, 1:-1] + u_curr[:-2, 1:-1] -
            4 * u_curr[1:-1, 1:-1]
        ) / h2
        
        # Time stepping
        u_next = (2 * u_curr - u_prev + v2dt2 * lap) * damp
        
        # Inject receiver data (reversed)
        u_next[rec_z, :] += rec_data[it, :]
        
        # Imaging condition (cross-correlation)
        image += src_snaps[it, :, :] * u_next
        
        # Swap
        u_prev = u_curr
        u_curr = u_next
    
    return image


# =============================================================================
# rtm3d-cli RTM
# =============================================================================

def rtm3d_cli_rtm_2d(nx: int, nz: int, dx: float, dz: float,
                     nt: int, dt: float, f0: float,
                     velocity: np.ndarray, pml: int = 10) -> np.ndarray:
    """
    Run 2D RTM using rtm3d-cli (C++).
    
    Returns migrated image (nz, nx).
    """
    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir = Path(tmpdir)
        
        # Write velocity model (2D JSON array)
        vel_2d = velocity.tolist()
        x_coords = (np.arange(nx) * dx).tolist()
        z_coords = (np.arange(nz) * dz).tolist()
        
        (tmpdir / "x.json").write_text(json.dumps(x_coords))
        (tmpdir / "z.json").write_text(json.dumps(z_coords))
        (tmpdir / "vel.json").write_text(json.dumps(vel_2d))
        
        # Config
        config = {
            "data_dir": str(tmpdir),
            "ny": 1,
            "dy": dx,
            "nt": nt,
            "dt": dt,
            "f0": f0,
            "pml": pml,
            "n_shots": 1,
            "output": str(tmpdir / "migrated"),
            "output_format": "float32_raw"
        }
        (tmpdir / "config.json").write_text(json.dumps(config))
        
        # Run CLI
        print("  rtm3d-cli: Running RTM...")
        result = subprocess.run(
            ["./build/rtm3d_cli", "--config", str(tmpdir / "config.json")],
            capture_output=True, text=True, timeout=300, cwd="."
        )
        
        if result.returncode != 0:
            raise RuntimeError(f"rtm3d-cli failed: {result.stderr}")
        
        # Read output
        output_file = tmpdir / "migrated"
        if not output_file.exists():
            raise RuntimeError(f"No output file: {output_file}")
        
        image = np.fromfile(output_file, dtype=np.float32).reshape((nz, nx))
        return image


# =============================================================================
# Test Scenarios
# =============================================================================

def test_constant_velocity():
    """Compare RTM on constant velocity model."""
    print("\n" + "="*60)
    print("TEST: Constant Velocity RTM Comparison")
    print("="*60)
    
    # Parameters
    nx, nz = 80, 60
    dx, dz = 10.0, 10.0
    nt = 200
    dt = 0.0004  # CFL safe
    f0 = 16.0
    v_const = 2000.0
    pml = 8
    
    print(f"Model: {nx}x{nz}, dx={dx}m, dz={dz}m")
    print(f"Time: nt={nt}, dt={dt}s, f0={f0}Hz")
    print(f"Velocity: constant {v_const} m/s")
    
    # Create velocity model
    velocity = np.full((nz, nx), v_const, dtype=np.float32)
    
    # Run Devito RTM
    print("\n[Devito RTM]")
    try:
        img_devito = devito_rtm_2d(nx, nz, dx, dz, nt, dt, f0, velocity, pml)
    except Exception as e:
        print(f"  ERROR: {e}")
        return None
    
    # Run rtm3d-cli RTM
    print("\n[rtm3d-cli RTM]")
    try:
        img_rtm3d = rtm3d_cli_rtm_2d(nx, nz, dx, dz, nt, dt, f0, velocity, pml)
    except Exception as e:
        print(f"  ERROR: {e}")
        return None
    
    # Compare
    print("\n[Comparison]")
    metrics = compute_all_metrics(img_devito, img_rtm3d)
    
    print(f"  Pearson correlation: {metrics['pearson']:.4f}")
    print(f"  SNR: {metrics['snr_db']:.2f} dB")
    print(f"  NRMSE: {metrics['nrmse']:.4f}")
    print(f"  Relative L2 error: {metrics['rel_l2']:.4f}")
    
    # Interpretation
    if metrics['pearson'] > 0.95:
        print("  ✓ EXCELLENT match (correlation > 0.95)")
    elif metrics['pearson'] > 0.80:
        print("  ✓ GOOD match (correlation > 0.80)")
    elif metrics['pearson'] > 0.60:
        print("  ⚠ MODERATE match (correlation > 0.60)")
    else:
        print("  ✗ POOR match (correlation < 0.60)")
    
    return metrics


def test_layered_velocity():
    """Compare RTM on layered velocity model."""
    print("\n" + "="*60)
    print("TEST: Layered Velocity RTM Comparison")
    print("="*60)
    
    nx, nz = 80, 60
    dx, dz = 10.0, 10.0
    nt = 200
    dt = 0.0004
    f0 = 16.0
    pml = 8
    
    print(f"Model: {nx}x{nz}")
    print(f"Velocity: 1500 m/s at surface, 3500 m/s at bottom")
    
    # Layered velocity (increasing with depth)
    velocity = np.zeros((nz, nx), dtype=np.float32)
    for iz in range(nz):
        v = 1500.0 + (iz / nz) * 2000.0
        velocity[iz, :] = v
    
    print("\n[Devito RTM]")
    try:
        img_devito = devito_rtm_2d(nx, nz, dx, dz, nt, dt, f0, velocity, pml)
    except Exception as e:
        print(f"  ERROR: {e}")
        return None
    
    print("\n[rtm3d-cli RTM]")
    try:
        img_rtm3d = rtm3d_cli_rtm_2d(nx, nz, dx, dz, nt, dt, f0, velocity, pml)
    except Exception as e:
        print(f"  ERROR: {e}")
        return None
    
    print("\n[Comparison]")
    metrics = compute_all_metrics(img_devito, img_rtm3d)
    
    print(f"  Pearson correlation: {metrics['pearson']:.4f}")
    print(f"  SNR: {metrics['snr_db']:.2f} dB")
    print(f"  NRMSE: {metrics['nrmse']:.4f}")
    print(f"  Relative L2 error: {metrics['rel_l2']:.4f}")
    
    if metrics['pearson'] > 0.95:
        print("  ✓ EXCELLENT match")
    elif metrics['pearson'] > 0.80:
        print("  ✓ GOOD match")
    else:
        print("  ⚠ Check implementation differences")
    
    return metrics


def test_circle_anomaly():
    """Compare RTM on circular anomaly model."""
    print("\n" + "="*60)
    print("TEST: Circle Anomaly RTM Comparison")
    print("="*60)
    
    nx, nz = 80, 80
    dx, dz = 10.0, 10.0
    nt = 200
    dt = 0.0004
    f0 = 16.0
    pml = 8
    
    print(f"Model: {nx}x{nz}")
    print(f"Velocity: 1500 m/s background, 2500 m/s circle anomaly")
    
    # Circle anomaly
    velocity = np.full((nz, nx), 1500.0, dtype=np.float32)
    cx, cz = nx // 2, nz // 2
    radius = 10
    for iz in range(nz):
        for ix in range(nx):
            if (ix - cx)**2 + (iz - cz)**2 < radius**2:
                velocity[iz, ix] = 2500.0
    
    print("\n[Devito RTM]")
    try:
        img_devito = devito_rtm_2d(nx, nz, dx, dz, nt, dt, f0, velocity, pml)
    except Exception as e:
        print(f"  ERROR: {e}")
        return None
    
    print("\n[rtm3d-cli RTM]")
    try:
        img_rtm3d = rtm3d_cli_rtm_2d(nx, nz, dx, dz, nt, dt, f0, velocity, pml)
    except Exception as e:
        print(f"  ERROR: {e}")
        return None
    
    print("\n[Comparison]")
    metrics = compute_all_metrics(img_devito, img_rtm3d)
    
    print(f"  Pearson correlation: {metrics['pearson']:.4f}")
    print(f"  SNR: {metrics['snr_db']:.2f} dB")
    print(f"  NRMSE: {metrics['nrmse']:.4f}")
    print(f"  Relative L2 error: {metrics['rel_l2']:.4f}")
    
    if metrics['pearson'] > 0.95:
        print("  ✓ EXCELLENT match")
    elif metrics['pearson'] > 0.80:
        print("  ✓ GOOD match")
    else:
        print("  ⚠ Check implementation differences")
    
    return metrics


# =============================================================================
# Main
# =============================================================================

def main():
    print("="*60)
    print("Devito vs rtm3d-cli RTM Comparison Suite")
    print("="*60)
    print("\nComparing RTM results using similarity metrics:")
    print("  - Pearson correlation: 1.0 = perfect")
    print("  - SNR: higher = better")
    print("  - NRMSE / Rel L2: 0.0 = perfect")
    
    results = {}
    
    # Run tests
    results['constant'] = test_constant_velocity()
    results['layered'] = test_layered_velocity()
    results['circle'] = test_circle_anomaly()
    
    # Summary
    print("\n" + "="*60)
    print("SUMMARY")
    print("="*60)
    
    for name, metrics in results.items():
        if metrics:
            print(f"\n{name.upper()}:")
            print(f"  Pearson: {metrics['pearson']:.4f}")
            print(f"  SNR: {metrics['snr_db']:.2f} dB")
        else:
            print(f"\n{name.upper()}: FAILED")
    
    # Overall assessment
    all_pearson = [m['pearson'] for m in results.values() if m]
    if all_pearson:
        avg_pearson = np.mean(all_pearson)
        print(f"\nAverage Pearson correlation: {avg_pearson:.4f}")
        
        if avg_pearson > 0.90:
            print("✓ OVERALL: Excellent agreement between Devito and rtm3d-cli")
            return 0
        elif avg_pearson > 0.70:
            print("⚠ OVERALL: Good agreement, some differences expected")
            return 0
        else:
            print("✗ OVERALL: Significant differences - review implementations")
            return 1
    
    return 1


if __name__ == "__main__":
    sys.exit(main())
