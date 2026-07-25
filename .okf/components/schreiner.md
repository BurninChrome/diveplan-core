---
type: C Module
title: schreiner.c — variable-pressure gas loading
description: The exact closed-form solution for gas loading during a linear ascent or descent; the only integrator the main dive loop uses.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/schreiner.c
tags: [schreiner, gas-loading, source, layer-1]
timestamp: '2026-07-25T09:30:00Z'
---

# Signature

```c
double schreiner(double pt0, double palv0, double r, double t, double half_val);
```

| Parameter | Unit | Meaning |
|---|---|---|
| `pt0` | bar | Tissue partial pressure at start of interval. |
| `palv0` | bar | Alveolar partial pressure **at the start** of the interval. |
| `r` | bar/min | Rate of change of the *alveolar inert gas* pressure. Positive descending. |
| `t` | min | Interval length. |
| `half_val` | min | Compartment half-time for this gas. |

**`r` is already scaled by the gas fraction.** The caller multiplies the ambient
pressure rate by `fN₂` or `fHe` — [`compartment_descend()`](/components/compartment.md)
does this. Passing a raw ambient rate over-loads the tissue by roughly 1/0.78.

# Implementation

```c
k  = M_LN2 / half_val;
pt = palv0 + r * (t - 1.0f / k) - (palv0 - pt0 - r / k) * exp(-k * t);
```

Correct transcription of the Schreiner equation. Theory:
[inert gas loading](/domain/inert-gas-loading.md).

# Why this is the only integrator that runs

[`dive.c`](/components/dive-cli.md) calls
[`compartment_descend()`](/components/compartment.md) for **every** step,
including flat bottom time where `r = dp/dt = 0`. That is legitimate: at `r = 0`
the equation reduces algebraically to Haldane, and the test suite asserts the
identity numerically to 1e-6. So [`haldane.c`](/components/haldane.md) is dead
code from the CLI's perspective, reached only through
[`nodecotime()`](/components/stop.md).

# Behaviour at the edges

| Input | Result |
|---|---|
| `t = 0` | Returns `pt0` exactly — the `r/k` terms cancel. |
| `r = 0` | Identical to `haldane(pt0, palv0, t, half_val)`. |
| `t → ∞` | Diverges linearly, tracking the ramp with a fixed lag of `r/k`. Correct behaviour for a sustained descent. |
| `half_val = 0` | `k = ∞`, `r/k = 0`, `1/k = 0`. For `t > 0` returns `palv0 + r·t`; **NaN at `t = 0`**. See [phantom compartment](/findings/zh-l16ab-phantom-compartment.md). |

# A subtlety about `palv0`

`palv0` is the alveolar pressure at the **start** of the interval, and the ramp
`r·t` carries it forward. [`compartment_descend()`](/components/compartment.md)
honours this by computing `ventilation()` from `p_ambient`, and
[`dive.c`](/components/dive-cli.md) passes `lastp` — the previous line's
pressure — for that argument. That pairing is correct.

The same `lastp` is *also* passed to [`nodecotime()`](/components/stop.md),
where it is wrong: an NDL should be evaluated at the depth you are at, not the
one you just left. Discussed in
[the nodecotime finding](/findings/nodecotime-overestimates-ndl.md).

# Test coverage

`test_schreiner()` covers the `r = 0` ↔ Haldane identity, `t = 0`, and one
realistic descent step. Adequate. Not covered: sustained descent over many
half-times, negative `r` (ascent), or the interaction with a gas switch
mid-ramp.
