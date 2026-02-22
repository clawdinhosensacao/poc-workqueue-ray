# RTM Reference Notes (initial baseline)

Goal: keep this document short and practical. It captures implementation patterns to guide `rtm3d-cli` refactors and benchmark upgrades.

## 1) Practical references (3–5)

1. **Devito seismic tutorials/examples (acoustic modeling + RTM patterns)**
   - https://github.com/devitocodes/devito/tree/master/examples/seismic
   - Concrete code entry points:
     - Preset models: https://github.com/devitocodes/devito/blob/master/examples/seismic/preset_models.py
     - Acoustic operators: https://github.com/devitocodes/devito/blob/master/examples/seismic/acoustic/operators.py
     - Acoustic wave solver: https://github.com/devitocodes/devito/blob/master/examples/seismic/acoustic/wavesolver.py
   - Why relevant: clear separation of forward/adjoint operators, acquisition geometry objects, imaging condition examples.

2. **Devito paper (DSL for finite differences)**
   - Louboutin et al., 2019, *The Devito DSL for automated finite differences in geophysics*.
   - https://doi.org/10.5194/gmd-12-1165-2019
   - Why relevant: good design references for maintainable/stencil-centric implementations.

3. **Classical RTM foundations**
   - Baysal, Kosloff, Sherwood (1983), *Reverse time migration*.
   - Whitmore (1983), *Iterative depth migration by backward time propagation*.
   - Why relevant: conceptual baseline for forward wavefield + backward receiver wavefield + imaging condition.

4. **Open seismic processing ecosystems (implementation patterns + reproducible flows)**
   - Madagascar RSF: http://www.ahay.org/
   - (Typical reference area) user docs + examples: https://www.ahay.org/wiki/Main_Page
   - Why relevant: reproducible processing flows and migration-related tooling patterns.

5. **SEG-Y tooling references for IO compatibility improvements**
   - segyio: https://github.com/equinor/segyio
   - ObsPy SEG-Y: https://docs.obspy.org/packages/obspy.io.segy.html
   - Why relevant: practical guidance for future strict SEG-Y interoperability beyond current SEG-Y-like output.

> Note: this is the initial list; expand with repo-specific code links during later cycles.

## 2) Baseline implementation checklist for `rtm3d-cli`

### Numerics/stencil
- [ ] Explicitly document PDE and stencil order in code/docs.
- [x] Keep FD step kernel isolated and testable.
- [ ] Confirm CFL-related parameter checks (dt vs dx/dz/dy and velocity bounds).

### Boundary conditions / absorbing layers
- [x] Keep damping/PML generation in dedicated module (`Boundary.cpp`).
- [ ] Add tests for boundary damping profile monotonicity and edge behavior.
- [ ] Parameterize damping strength separately from width (pml cells).

### Source model
- [x] Keep wavelet generation isolated (`Wavelet.cpp` - `ricker_wavelet`).
- [ ] Support configurable source type and source depth/position policy.

### Acquisition geometry
- [x] Separate geometry builder (`Acquisition.cpp`) from propagation loops.
- [x] Multi-shot geometry abstraction (`ShotPosition` struct + `run_multi_shot_rtm`).
- [x] CLI option `--n-shots` for multi-shot migration.

### Receiver backpropagation (adjoint)
- [x] Keep injection and stepping as separate functions (`ReceiverImaging.cpp`).
- [x] Tests for receiver injection indexing/consistency (`test_rtm_receivers.cpp`).

### Imaging condition
- [x] Keep cross-correlation imaging condition isolated (`Imaging.cpp`).
- [ ] Add optional illumination compensation hooks.
- [ ] Prepare extension point for alternative imaging conditions.

### Data/IO + metadata
- [x] Persist enough metadata to reproduce run (geometry, wavelet, seed, numerics).
- [x] Keep synthetic generator and migration config versioned.

## 3) Implementation status (2026-02-22)

### Completed modules (extracted from RtmEngine)
| Module | File | Responsibility |
|--------|------|----------------|
| Validation | `Validation.cpp` | Config validation |
| Acquisition | `Acquisition.cpp` | Shot geometry builder |
| Wavelet | `Wavelet.cpp` | Ricker wavelet generation |
| SourcePropagation | `SourcePropagation.cpp` | Forward wave propagation |
| ReceiverImaging | `ReceiverImaging.cpp` | Backpropagation + imaging |
| ResultBuilder | `ResultBuilder.cpp` | Result assembly |
| Receivers | `Receivers.cpp` | Receiver operations |
| InlineSlice | `InlineSlice.cpp` | Slice extraction |
| Boundary | `Boundary.cpp` | PML damping |
| Geometry | `Geometry.cpp` | Velocity volume |

### Test coverage
- 34 tests passing
- 12 RTM-specific test files

### API
```cpp
// Single-shot migration
MigrationResult run_single_shot_rtm(const GridModel2D& model, const RtmConfig& cfg);

// Multi-shot migration with image stacking
MigrationResult run_multi_shot_rtm(const GridModel2D& model, const RtmConfig& cfg,
                                   const std::vector<ShotPosition>& shots);
```

## 4) Devito code patterns mapped to rtm3d-cli

### Forward propagation correspondence

**Devito (Python):**
```python
# From examples/seismic/acoustic/operators.py
def Forward(model, source, receiver, **kwargs):
    # Forward wavefield propagation
    u = wavefield(model.grid)  # Allocate wavefield
    src_term = source.inject(u, src)  # Source injection
    rec_term = receiver.interpolate(u)  # Receiver recording
    op = Operator([src_term, rec_term, stencil])  # Build operator
    op(time=time_range.num-1, dt=dt)
```

**rtm3d-cli (C++):**
```cpp
// src/rtm/SourcePropagation.cpp
void forward_source_propagation(..., const std::vector<float>& wavelet,
                                 std::vector<float>& src_snaps,
                                 std::vector<float>& rec_data) {
    // Time-stepping loop
    for (std::size_t it = 0; it < nt; ++it) {
        inject_source(..., wavelet[it]);     // Source injection
        fd_step_forward(...);                 // Stencil application
        record_receivers(..., rec_data, it);  // Receiver recording
        save_snapshot(src_snaps, it);         // Wavefield checkpoint
    }
}
```

### Imaging condition correspondence

**Devito (Python):**
```python
# Cross-correlation imaging condition
image = Function(name='image', grid=model.grid)
op = Operator([Eq(image, image + u * v)])  # u=forward, v=adjoint
```

**rtm3d-cli (C++):**
```cpp
// src/rtm/Imaging.cpp
void apply_imaging_condition(const Volume3D& vel, std::size_t n,
                             const std::vector<float>& src_snap,
                             std::vector<float>& image, std::size_t it) {
    for (std::size_t i = 0; i < n; ++i) {
        image[i] += src_snap[it * n + i] * adjoint[i];  // Cross-correlation
    }
}
```

### Boundary damping (PML) correspondence

**Devito (Python):**
```python
# Damping profile from examples/seismic/preset_models.py
damp = Function(name='damp', grid=model.grid)
damp.data[:] = damping_profile(nbl, spacing)  # Boundary layer width
```

**rtm3d-cli (C++):**
```cpp
// src/rtm/Boundary.cpp
std::vector<float> make_damp(std::size_t nx, std::size_t ny, std::size_t nz,
                             std::size_t pml) {
    // Gaussian taper at boundaries
    for (std::size_t k = 0; k < pml; ++k) {
        float val = 0.01f * std::pow(k / static_cast<float>(pml), 2);
        damp[k] = val;
    }
}
```

## 5) Actionable tasks from references

| Task | Priority | Source |
|------|----------|--------|
| CFL stability check | Medium | Devito acoustic solver validates dt/dx |
| Illumination compensation | Medium | Devito RTM examples show source illumination |
| Configurable source type | Low | Devito supports Ricker, Gaussian, custom |
| SEG-Y strict compliance | Low | segyio/ObsPy patterns for production IO |

## 6) Immediate engineering moves (next cycles)

1. Introduce lightweight internal structs:
   - `ShotGeometry`, `ReceiverGeometry`, `Wavefields`.

2. Add static-analysis gate in routine:
   - keep `make static` (clang-tidy/cppcheck/fanalyzer fallback).

3. Expand multi-shot integration from generator into RTM runtime path.
