#pragma once

/**
 * @file Propagation.hpp
 * @brief 3D acoustic wave propagation kernel (2nd-order FD stencil).
 *
 * ## Governing PDE
 *
 * The acoustic wave equation in 3D:
 *
 *     ∂²u/∂t² = v²(x,y,z) · ∇²u
 *
 * where:
 * - u(x,y,z,t) is the pressure wavefield
 * - v(x,y,z) is the acoustic velocity (m/s)
 * - ∇² = ∂²/∂x² + ∂²/∂y² + ∂²/∂z² is the Laplacian
 *
 * ## Finite Difference Discretization
 *
 * 2nd-order accurate in space, 2nd-order in time:
 *
 *     uⁿ⁺¹ = 2uⁿ - uⁿ⁻¹ + (v·Δt)² · ∇²ₕuⁿ
 *
 * where ∇²ₕ is the discrete Laplacian:
 *
 *     ∇²ₕu = (u[i+1,j,k] - 2u[i,j,k] + u[i-1,j,k])/Δx²
 *          + (u[i,j+1,k] - 2u[i,j,k] + u[i,j-1,k])/Δy²
 *          + (u[i,j,k+1] - 2u[i,j,k] + u[i,j,k-1])/Δz²
 *
 * ## CFL Stability Condition
 *
 * For stability, the time step must satisfy:
 *
 *     Δt ≤ 1/(v_max · √(1/Δx² + 1/Δy² + 1/Δz²))
 *
 * In practice, use Δt ≤ 0.5 · CFL_max for safety margin.
 *
 * @see Validation.cpp for CFL check implementation
 */

#include <vector>

#include "rtm3d/core/Volume3D.hpp"

namespace rtm3d::rtm_internal {

/**
 * @brief Perform one time step of 3D acoustic wave propagation.
 *
 * Implements 2nd-order FD stencil for the acoustic wave equation.
 * Uses 3-point stencil in each spatial direction (6 neighbors total).
 *
 * @param vel      Velocity volume (m/s)
 * @param damp     Damping field for PML boundaries (1.0 interior, <1.0 at edges)
 * @param dt       Time step (seconds)
 * @param dx,dy,dz Grid spacing (meters)
 * @param prev     Wavefield at time n-1
 * @param cur      Wavefield at time n
 * @param nxt      [out] Wavefield at time n+1
 */
void step_fd3d(const Volume3D& vel, const std::vector<float>& damp, float dt, float dx, float dy,
               float dz, const std::vector<float>& prev, const std::vector<float>& cur,
               std::vector<float>& nxt);

}  // namespace rtm3d::rtm_internal
