---
type: C Module
title: haldane.c — constant-pressure gas loading
description: The exponential wash-in/wash-out equation used for flat (constant depth) profile segments.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/haldane.c
tags: [haldane, gas-loading, source, layer-1]
timestamp: '2026-07-25T09:30:00Z'
---

# Signature

```c
double haldane(double pt0, double palv0, double t, double half_val);
```

| Parameter | Unit | Meaning |
|---|---|---|
| `pt0` | bar | Tissue partial pressure at start of interval. |
| `palv0` | bar | Alveolar partial pressure, constant over the interval. |
| `t` | min | Interval length. |
| `half_val` | min | Compartment half-time **for this gas**. |

# Implementation

```c
k  = M_LN2 / half_val;
pt = pt0 + (palv0 - pt0) * (1.0f - exp(-k * t));
```

Correct. Symmetric for on- and off-gassing. Theory:
[inert gas loading](/domain/inert-gas-loading.md).

`M_LN2` comes from `<math.h>` and requires `_GNU_SOURCE` or a non-strict `-std`;
the library sets `-std=c99 -D_GNU_SOURCE` for exactly this reason. See
[the ADR](/decisions/2026-03-09-modernize-build-and-python3.md).

# The `1.0f` literal

`(1.0f - exp(...))` mixes a `float` literal into a `double` expression. C's
usual arithmetic conversions promote it back to `double` before the subtraction,
so **there is no precision loss** — `1.0f` is exactly representable. It is a
cosmetic leftover from an era when the constants were `float`, and it appears in
[`schreiner.c`](/components/schreiner.md) and
[`gradientfactor.c`](/components/gradientfactor.md) too. Harmless; do not
"fix" it under the [algorithm integrity policy](/decisions/algorithm-integrity-policy.md)
without a reason, since touching these files requires approval regardless.

# Behaviour at the edges

| Input | Result |
|---|---|
| `t = 0` | Returns `pt0` exactly. |
| `t → ∞` | Converges to `palv0`. |
| `t = half_val` | Exactly halfway between `pt0` and `palv0`. |
| `half_val = 0` | `k = +∞`; `exp(-∞·t)` = 0 for `t > 0` (returns `palv0`), but **NaN at `t = 0`** since `∞ × 0` is NaN. Reachable via [the phantom compartment](/findings/zh-l16ab-phantom-compartment.md). |
| `half_val < 0` | Diverges exponentially. Unguarded, unreachable with shipped tables. |

# Callers

[`compartment_stagnate()`](/components/compartment.md) only. That in turn is
called by [`nodecotime()`](/components/stop.md) and the test suite — **not** by
the main dive loop, which routes every step through
[`schreiner()`](/components/schreiner.md) instead.

# Test coverage

`test_haldane()` covers `t = 0`, one/two/three half-times, saturation,
off-gassing, and a realistic compartment-1 step. Strong.
