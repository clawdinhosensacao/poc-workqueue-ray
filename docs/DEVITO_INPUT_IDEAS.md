# Devito Input Ideas for Synthetic Testing

Quick notes extracted from Devito seismic presets to inspire `rtm3d-cli` benchmark inputs.

Source file reviewed:
- `examples/seismic/preset_models.py`
- https://raw.githubusercontent.com/devitocodes/devito/master/examples/seismic/preset_models.py

## Useful preset families

- `constant-isotropic`
- `layers-isotropic`
- `circle-isotropic`
- `marmousi2d-isotropic` (requires external dataset)

## What to borrow for `rtm3d-cli`

1. **Preset naming + scenario catalog**
   - Keep explicit scenario names (`layered_fault`, `salt_dome`, future: `circle_lens`, `constant_bg`).

2. **Layered model parameterization**
   - Control number of layers and top/bottom velocity (`vp_top`, `vp_bottom`, `nlayers`).

3. **Simple anomaly scenario**
   - Circle/lens anomaly over background for fast algorithm sanity checks.

4. **Boundary damping mindset**
   - Devito uses damped boundaries (`bcs="damp"`), aligned with our PML/damping strategy.

5. **Path to realistic benchmark**
   - Add optional Marmousi-like import path for a harder reference scenario.

## Immediate actionable next steps

- Add `circle_lens` synthetic scenario.
- Add layer count / velocity range CLI knobs for `layered_fault`.
- Add a documented recipe for generating a small and a medium benchmark profile.
