---
type: Reference
title: Units and conventions
description: Every unit, sign convention and scale used across the codebase — the single most common source of integration errors.
tags: [units, conventions, pressure, integration]
timestamp: '2026-07-25T09:30:00Z'
---

# The table

| Quantity | Unit | Convention |
|---|---|---|
| Pressure | bar, **absolute** | Surface = 1.0, not 0.0. |
| Depth | metres seawater | Only in tooling and human-facing text. The C library never sees metres. |
| Time | minutes | Including half-times and NDL. |
| Gas fraction | dimensionless 0.0–1.0 | `0.21`, not `21`. |
| Rate | bar per minute | **Positive = descending**, negative = ascending. |
| Half-time | minutes | |
| M-value `a` | bar | |
| M-value `b` | dimensionless | Always in (0, 1). |
| Gradient factor | dimensionless 0.0–1.0 | `0.85`, not `85`. |

# Depth ↔ pressure

```
P_bar   = depth_m / 10 + 1.0
depth_m = (P_bar − 1.0) × 10
```

A flat 10 metres-per-bar. No salinity, altitude or atmospheric-pressure
correction anywhere in the codebase. **Every pressure is absolute** — a "20 m
dive" is 3.0 bar.

# Where the conversion lives

Only at the edges, and each edge does it by hand:

- [`gen_dive.py`](/tooling/gen-dive.md) — `depth/10 + 1` when writing profiles.
- [`parse_dive.py`](/tooling/parse-dive.md) — `depth/10 + 1`; its XML source
  stores depth in metres, so this is the same conversion.
- [`visoutput.py`](/tooling/visoutput.md) — `(p − 1) × 10` when plotting.

The C library has no conversion function and no metres anywhere. Adding one
would be the natural home for altitude support.

# Two exceptions to watch

**`gradient_factor*()` depth parameters.** Named `..._stop_depth` but consumed
as bar on the absolute scale like everything else. The unit tests pass 3.0 and
30.0, which read as metres. Arithmetically consistent either way, silently
wrong if a caller mixes conventions. See [gradient factors](/domain/gradient-factors.md).

**`otu_*()` `o2_ratio` parameters.** Named as fractions but physically partial
pressures in bar. See [oxygen toxicity](/domain/oxygen-toxicity.md).

Both are naming problems, not arithmetic ones, but both are the kind that
produce plausible-looking wrong numbers rather than crashes.

# Nitrogen fraction

Two values appear, and they disagree by 1.2%:

- **0.78084** — dry atmospheric N₂. Hard-coded in `dive.c`'s surface
  initialisation and throughout the test suite.
- **0.79052** — implied by `gen_dive.py` emitting `O2 = 0.20948` and `dive.c`
  computing `n2_ratio = 1.0 − fO2 − fHe`.

So a run initialises compartments at the 0.78084 equilibrium and then
immediately starts loading them against 0.79052. On a surface-interval-only
profile this shows as a slow drift upward in every compartment. It is
long-standing and harmless at dive-planning resolution, but it means
"equilibrated at the surface" is not a fixed point of the simulation.

# Sentinel values

| Value | Field | Means |
|---|---|---|
| `100.0` | `nodectime` | Capped — "no obligation within 100 min", not a measurement. See [NDL](/domain/no-decompression-limit.md). |
| `≤ 1.0` | `ceiling` | No decompression obligation. Can be negative; [`visoutput.py`](/tooling/visoutput.md) clamps to 0 for display. |

# Related

- [stdio format](/interfaces/dive-stdio-format.md) — where these units cross the
  process boundary.
- [C API](/interfaces/c-api.md) — where they cross the linker boundary.
