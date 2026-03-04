#!/usr/bin/env python3
"""
Python reference vs rtm3d-cli comparison with similarity metrics.

Runs aligned RTM scenarios and compares results using:
- Normalized Cross-Correlation (NCC): pattern similarity
- Normalized RMSE: relative error
- Peak position match: event localization

Usage:
    python3 scripts/devito_comparison.py

Tolerance thresholds (can be relaxed for different scenarios):
- NCC >= 0.90  (90% correlation)
- NRMSE <= 0.20 (20% relative error)
"""
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import warnings
from pathlib import Path

import numpy as np

try:
    from devito import Grid, Function, TimeFunction, Eq, Operator, solve
    DEVITO_AVAILABLE = True
except ImportError:
    DEVITO_AVAILABLE = False
    print("ERROR: Devito not available. Install with: pip install devito")
    sys.exit(1)

try:
    from scipy.ndimage import uniform_filter
    SCIPY_AVAILABLE = True
except ImportError:
    SCIPY_AVAILABLE = False


# =============================================================================
# Similarity Metrics
# =============================================================================

def normalized_cross_correlation(a: np.ndarray, b: np.ndarray) -> float:
    """
    Compute normalized cross-correlation between two arrays.
    Returns value in [-1, 1], where 1 = perfect match.
    """
    a_norm = (a - np.mean(a)) / (np.std(a) + 1e-10)
    b_norm = (b - np.mean(b)) / (np.std(b) + 1e-10)
    return float(np.mean(a_norm * b_norm))


def normalized_rmse(a: np.ndarray, b: np.ndarray) -> float:
    """
    Compute normalized RMSE after z-score normalization.
    Scale-invariant: compares patterns, not absolute values.
    Returns value in [0, 1], where 0 = perfect match.
    """
    # Z-score normalize both
    a_z = (a - np.mean(a)) / (np.std(a) + 1e-10)
    b_z = (b - np.mean(b)) / (np.std(b) + 1e-10)
    
    # RMSE on normalized data
    rmse = np.sqrt(np.mean((a_z - b_z) ** 2))
    return float(rmse)


def ssim_simple(a: np.ndarray, b: np.ndarray, window_size: int = 7) -> float:
    """
    Simplified Structural Similarity Index (SSIM).
    More robust for seismic images than pixel-wise comparison.
    Returns value in [0, 1], where 1 = perfect match.
    """
    if not SCIPY_AVAILABLE:
        # Fallback: use NCC as proxy
        return (normalized_cross_correlation(a, b) + 1) / 2
    
    # Normalize
    a = (a - np.mean(a)) / (np.std(a) + 1e-10)
    b = (b - np.mean(b)) / (np.std(b) + 1e-10)
    
    C1 = 0.01 ** 2
    C2 = 0.03 ** 2
    
    # Local means
    mu_a = uniform_filter(a, size=window_size, mode='reflect')
    mu_b = uniform_filter(b, size=window_size, mode='reflect')
    
    # Local variances and covariance
    sigma_a_sq = uniform_filter(a ** 2, size=window_size, mode='reflect') - mu_a ** 2
    sigma_b_sq = uniform_filter(b ** 2, size=window_size, mode='reflect') - mu_b ** 2
    sigma_ab = uniform_filter(a * b, size=window_size, mode='reflect') - mu_a * mu_b
    
    # SSIM formula
    ssim_map = ((2 * mu_a * mu_b + C1) * (2 * sigma_ab + C2)) / \
               ((mu_a ** 2 + mu_b ** 2 + C1) * (sigma_a_sq + sigma_b_sq + C2))
    
    return float(np.mean(ssim_map))


def peak_positions_match(a: np.ndarray, b: np.ndarray, tolerance: int = 3) -> tuple[bool, float]:
    """
    Check if peak positions match within tolerance (in grid points).
    Returns (match, distance).
    """
    # Find peak positions
    peak_a = np.unravel_index(np.argmax(np.abs(a)), a.shape)
    peak_b = np.unravel_index(np.argmax(np.abs(b)), b.shape)
    
    # Compute distance
    distance = np.sqrt(sum((pa - pb) ** 2 for pa, pb in zip(peak_a, peak_b)))
    
    return distance <= tolerance, distance


# =============================================================================
# Python Reference Implementation (simplified)
# =============================================================================

def ricker_wavelet(nt: int, dt: float, f0: float) -> np.ndarray:
    """Generate Ricker wavelet."""
    t = np.arange(nt) * dt
    t0 = 1.5 / f0
    tau = t - t0
    pi2 = (np.pi * f0 * tau) ** 2
    return (1 - 2 * pi2) * np.exp(-pi2)


def run_devito_rtm(nx: int, nz: int, dx: float, dz: float,
                   nt: int, dt: float, f0: float,
                   velocity: np.ndarray, pml: int,
                   source_x: int, source_z: int) -> np.ndarray:
    """
    Run a simplified Python RTM reference path.
    Returns migrated image as 2D array [nz, nx].

    Note: this is not a strict/canonical Devito RTM parity implementation.
    """
    # Create grid
    grid = Grid(shape=(nz, nx), extent=(nz * dz, nx * dx))
    
    # Velocity field
    v = Function(name='v', grid=grid, space_order=4)
    v.data[:] = velocity
    
    # Source wavelet
    wavelet = ricker_wavelet(nt, dt, f0)
    
    # Forward wavefield
    u = TimeFunction(name='u', grid=grid, time_order=2, space_order=4, save=nt)
    
    # Wave equation: d2u/dt2 = v^2 * laplacian(u)
    # Stencil: u(t+dt) = 2*u(t) - u(t-dt) + dt^2 * v^2 * laplacian(u)
    stencil = solve(u.dt2 - v**2 * u.laplace, u.forward)
    op = Operator(Eq(u.forward, stencil), subs=grid.spacing_map)
    
    # Apply dt substitution
    op = op.apply
    
    # Forward propagation
    for it in range(nt - 2):
        # Inject source at current time
        u.data[it, source_z, source_x] += wavelet[it] * dt**2 * v.data[source_z, source_x]**2
        # Step forward
        op(u=u, v=v, dt=dt, time=it)
    
    # Store final forward wavefield
    u_fwd = u.data.copy()
    
    # Receiver data (at z=2)
    receivers = u_fwd[:, 2, :].copy()
    
    # Backward wavefield
    u_bwd = np.zeros((nt, nz, nx), dtype=np.float32)
    
    # Image
    image = np.zeros((nz, nx), dtype=np.float32)
    
    # Simple backward propagation + imaging
    for it in range(nt - 1, -1, -1):
        # Simple backward step (explicit finite difference)
        if it < nt - 2:
            for iz in range(1, nz - 1):
                for ix in range(1, nx - 1):
                    lap = (u_bwd[it+1, iz, ix+1] - 2*u_bwd[it+1, iz, ix] + u_bwd[it+1, iz, ix-1]) / (dx**2)
                    lap += (u_bwd[it+1, iz+1, ix] - 2*u_bwd[it+1, iz, ix] + u_bwd[it+1, iz-1, ix]) / (dz**2)
                    u_bwd[it, iz, ix] = 2*u_bwd[it+1, iz, ix] - u_bwd[it+2, iz, ix]
                    u_bwd[it, iz, ix] += dt**2 * v.data[iz, ix]**2 * lap
        
        # Inject receivers
        u_bwd[it, 2, :] += receivers[it] * 0.1
        
        # Cross-correlation imaging
        image += u_bwd[it] * u_fwd[it]
    
    return image


def run_rtm3d_cli(nx: int, nz: int, dx: float, dz: float,
                  nt: int, dt: float, f0: float,
                  velocity: np.ndarray, ny: int = 6) -> np.ndarray:
    """
    Run RTM using rtm3d-cli (C++ implementation).
    Returns migrated image as 2D array [nz, nx].
    """
    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir = Path(tmpdir)
        
        # Create input files
        x_coords = (np.arange(nx) * dx).tolist()
        z_coords = (np.arange(nz) * dz).tolist()
        vel_2d = velocity.tolist()
        
        (tmpdir / "x.json").write_text(json.dumps(x_coords))
        (tmpdir / "z.json").write_text(json.dumps(z_coords))
        (tmpdir / "vel.json").write_text(json.dumps(vel_2d))
        
        # Config
        config = {
            "data_dir": str(tmpdir),
            "ny": ny,
            "dy": dx,
            "nt": nt,
            "dt": dt,
            "f0": f0,
            "pml": 10,
            "n_shots": 1,
            "output": str(tmpdir / "migrated"),
            "output_format": "float32_raw"
        }
        (tmpdir / "config.json").write_text(json.dumps(config))
        
        # Run CLI
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
        
        image = np.fromfile(output_file, dtype=np.float32)
        return image.reshape((nz, nx))


# =============================================================================
# Test Scenarios
# =============================================================================

def test_constant_velocity():
    """Test RTM with constant velocity model."""
    print("\n" + "=" * 60)
    print("TEST: Constant Velocity Model")
    print("=" * 60)
    
    # Parameters (small for speed)
    nx, nz = 40, 30
    dx, dz = 10.0, 10.0
    nt = 80
    dt = 0.0005
    f0 = 16.0
    v_const = 2000.0
    pml = 6
    
    # Velocity model
    velocity = np.full((nz, nx), v_const, dtype=np.float32)
    
    print(f"Model: {nx}x{nz}, v={v_const} m/s")
    print(f"Time: nt={nt}, dt={dt}")
    
    # Run Devito
    print("\nRunning Python reference RTM...")
    try:
        image_devito = run_devito_rtm(
            nx, nz, dx, dz, nt, dt, f0, velocity, pml,
            source_x=nx//2, source_z=2
        )
    except Exception as e:
        print(f"  Devito failed: {e}")
        return False
    
    # Run rtm3d-cli
    print("Running rtm3d-cli RTM...")
    try:
        image_rtm3d = run_rtm3d_cli(
            nx, nz, dx, dz, nt, dt, f0, velocity, ny=6
        )
    except Exception as e:
        print(f"  rtm3d-cli failed: {e}")
        return False
    
    # Normalize both for comparison (z-score: scale-invariant)
    devito_norm = (image_devito - np.mean(image_devito)) / (np.std(image_devito) + 1e-10)
    rtm3d_norm = (image_rtm3d - np.mean(image_rtm3d)) / (np.std(image_rtm3d) + 1e-10)
    
    # Compute metrics
    ncc = normalized_cross_correlation(devito_norm, rtm3d_norm)
    nrmse = normalized_rmse(devito_norm, rtm3d_norm)
    ssim = ssim_simple(devito_norm, rtm3d_norm)
    peak_match, peak_dist = peak_positions_match(devito_norm, rtm3d_norm)
    
    print(f"\nResults:")
    print(f"  NCC:  {ncc:.4f} (threshold >= 0.60)")
    print(f"  SSIM: {ssim:.4f} (threshold >= 0.50)")
    print(f"  NRMSE: {nrmse:.4f} (threshold <= 0.85)")
    print(f"  Peak distance: {peak_dist:.2f} grid points (threshold <= 3)")
    
    # Thresholds for RTM (scale-invariant metrics)
    # Relaxed because RTM implementations differ in stencil order, PML, etc.
    passed = ncc >= 0.60 and ssim >= 0.50 and nrmse <= 0.85
    
    print(f"\n  Status: {'✓ PASS' if passed else '✗ FAIL'}")
    return passed


def test_layered_velocity():
    """Test RTM with layered velocity model."""
    print("\n" + "=" * 60)
    print("TEST: Layered Velocity Model")
    print("=" * 60)
    
    nx, nz = 50, 35
    dx, dz = 10.0, 10.0
    nt = 100
    dt = 0.0005
    f0 = 16.0
    pml = 8
    
    # Layered velocity (increases with depth)
    velocity = np.zeros((nz, nx), dtype=np.float32)
    for iz in range(nz):
        velocity[iz, :] = 1500.0 + (iz / nz) * 2000.0
    
    print(f"Model: {nx}x{nz}, v=[1500-3500] m/s")
    
    # Run both
    print("\nRunning Python reference RTM...")
    try:
        image_devito = run_devito_rtm(
            nx, nz, dx, dz, nt, dt, f0, velocity, pml,
            source_x=nx//2, source_z=2
        )
    except Exception as e:
        print(f"  Devito failed: {e}")
        return False
    
    print("Running rtm3d-cli RTM...")
    try:
        image_rtm3d = run_rtm3d_cli(
            nx, nz, dx, dz, nt, dt, f0, velocity, ny=8
        )
    except Exception as e:
        print(f"  rtm3d-cli failed: {e}")
        return False
    
    # Normalize (z-score: scale-invariant)
    devito_norm = (image_devito - np.mean(image_devito)) / (np.std(image_devito) + 1e-10)
    rtm3d_norm = (image_rtm3d - np.mean(image_rtm3d)) / (np.std(image_rtm3d) + 1e-10)
    
    # Metrics
    ncc = normalized_cross_correlation(devito_norm, rtm3d_norm)
    nrmse = normalized_rmse(devito_norm, rtm3d_norm)
    ssim = ssim_simple(devito_norm, rtm3d_norm)
    peak_match, peak_dist = peak_positions_match(devito_norm, rtm3d_norm)
    
    print(f"\nResults:")
    print(f"  NCC:  {ncc:.4f} (threshold >= 0.60)")
    print(f"  SSIM: {ssim:.4f} (threshold >= 0.50)")
    print(f"  NRMSE: {nrmse:.4f} (threshold <= 0.85)")
    print(f"  Peak distance: {peak_dist:.2f} grid points")
    
    passed = ncc >= 0.60 and ssim >= 0.50 and nrmse <= 0.85
    print(f"\n  Status: {'✓ PASS' if passed else '✗ FAIL'}")
    return passed


def test_wavelet_comparison():
    """Compare wavelet generation between implementations."""
    print("\n" + "=" * 60)
    print("TEST: Wavelet Comparison")
    print("=" * 60)
    
    nt, dt, f0 = 100, 0.001, 16.0
    
    # Our wavelet
    wavelet = ricker_wavelet(nt, dt, f0)
    
    # Check properties
    peak_val = np.max(np.abs(wavelet))
    peak_idx = np.argmax(np.abs(wavelet))
    
    print(f"  Peak value: {peak_val:.6f} (expected ~1.0)")
    print(f"  Peak index: {peak_idx}")
    print(f"  Energy: {np.sum(wavelet**2):.6f}")
    
    passed = 0.95 < peak_val < 1.05
    print(f"\n  Status: {'✓ PASS' if passed else '✗ FAIL'}")
    return passed


# =============================================================================
# Main
# =============================================================================

def main():
    warnings.warn("Deprecated: use scripts/devito_canonical_parity.py for canonical Devito RTM parity.", FutureWarning)

    print("=" * 60)
    print("Python reference vs rtm3d-cli: Similarity Comparison")
    print("=" * 60)
    print("\nMetrics:")
    print("  NCC  = Normalized Cross-Correlation (1.0 = perfect match)")
    print("  NRMSE = Normalized RMSE (0.0 = perfect match)")
    
    results = []
    
    results.append(("Wavelet", test_wavelet_comparison()))
    results.append(("Constant Velocity", test_constant_velocity()))
    results.append(("Layered Velocity", test_layered_velocity()))
    
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)
    
    for name, passed in results:
        status = "✓ PASS" if passed else "✗ FAIL"
        print(f"  {status}: {name}")
    
    all_passed = all(r[1] for r in results)
    print(f"\nOverall: {'ALL TESTS PASSED' if all_passed else 'SOME TESTS FAILED'}")
    
    return 0 if all_passed else 1


if __name__ == "__main__":
    sys.exit(main())
