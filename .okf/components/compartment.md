---
type: C Module
title: compartment.c — per-compartment operations
description: Dispatches gas fractions and ambient pressure into the loading equations for both gases, and computes the weighted M-value.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/compartment.c
tags: [compartment, dispatch, m-value, source, layer-2]
timestamp: '2026-07-25T09:30:00Z'
---

# What this layer does

Everything below it works on one gas at a time and knows nothing about gas
mixes. This module is where "a compartment breathing a mix at a pressure"
becomes two independent single-gas problems. Three functions.

# `compartment_stagnate()` — constant depth

```c
void compartment_stagnate(const struct compartment_constants *constants,
                          struct compartment_state *cur,
                          struct compartment_state *end,
                          double p_ambient, double time, double rq,
                          double n2_ratio, double he_ratio);
```

Computes `ventilation()` for each gas at `p_ambient`, then calls
[`haldane()`](/components/haldane.md) twice. `cur` and `end` may alias — the
function reads both fields of `cur` before writing either field of `end`, so
in-place update is safe.

Called by [`nodecotime()`](/components/stop.md) and the test suite. **Not**
called by the main dive loop.

# `compartment_descend()` — changing depth

```c
void compartment_descend(const struct compartment_constants *constants,
                         struct compartment_state *cur,
                         struct compartment_state *end,
                         double p_ambient, double rate, double time, double rq,
                         double n2_ratio, double he_ratio);
```

Same, but scales `rate` by each gas fraction and calls
[`schreiner()`](/components/schreiner.md):

```c
n2_rate = rate * n2_ratio;
he_rate = rate * he_ratio;
```

**"Descend" is a misnomer** — a negative `rate` models ascent perfectly well,
and `rate = 0` reduces to `compartment_stagnate()`. This is the only integrator
the CLI uses; it handles all three cases. Read the name as "integrate over a
linear pressure ramp".

Aliasing is safe here too, and [`dive.c`](/components/dive-cli.md) relies on it,
passing `&s[i]` for both `cur` and `end`.

# `compartment_mvalue()` — weighted M-value

```c
double compartment_mvalue(const struct compartment_constants *constants,
                          struct compartment_state *compt);
```

```c
a = (n2_p·n2_a + he_p·he_a) / (n2_p + he_p);
b = (n2_p·n2_b + he_p·he_b) / (n2_p + he_p);
return ((n2_p + he_p) - a) * b;
```

The Bühlmann-standard combined-gas ceiling: blend the coefficients by each
gas's share of the total inert load. Theory:
[M-values and ceiling](/domain/m-values-and-ceiling.md).

Two things to know:

- **It is never called by production code.** The CLI uses
  [`getCeiling()`](/components/ceiling.md), which uses a different and less
  conservative formulation. See
  [the divergence finding](/findings/ceiling-vs-mvalue-divergence.md).
- **It returns NaN when both pressures are zero** — 0/0 in both coefficient
  blends. Reachable, since a freshly zeroed `compartment_state` is exactly that.
  See [the finding](/findings/mvalue-division-by-zero.md).

The `compt` parameter is not `const` although the function does not write
through it. Cosmetic; changing it is an API-compatible improvement but touches a
policy-protected file.

# The n2_ratio convention

Callers pass `n2_ratio` explicitly rather than having it derived. In
[`dive.c`](/components/dive-cli.md) it is computed as `1.0 - o2 - he`, so any
trace gas in the mix is silently accounted as nitrogen. For air that means
0.79 rather than the 0.78084 used to initialise the compartments — see
[units and conventions](/domain/units-and-conventions.md).

# Test coverage

`test_compartment_stagnate()`, `test_compartment_descend()` and
`test_compartment_mvalue()` cover equilibrium, on-gassing, the
`descend(rate=0) == stagnate` identity, pure-N₂, pure-He and mixed M-values, and
monotonicity in loading. Not covered: aliasing safety, the NaN case, or
`he_ratio > 0` through `compartment_descend()`.
