---
type: C Module
title: gradientfactor.c — GF line arithmetic
description: Two small functions defining the linear gradient-factor line; complete, tested, and not connected to anything.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/gradientfactor.c
tags: [gradient-factor, conservatism, not-integrated, source, layer-3]
timestamp: '2026-07-25T09:30:00Z'
---

# Signatures

```c
double gradient_factor_slope(double gfhi, double gflow,
                             double final_stop_depth, double first_stop_depth);
double gradient_factor(double gfslope, double curr_stop_depth, double gfhi);
```

# Implementation

```c
/* slope */
if (final_stop_depth - first_stop_depth != 0.0)
    gfslope = (gfhi - gflow) / (final_stop_depth - first_stop_depth);
return gfslope;                       /* 0.0 otherwise */

/* value at a depth */
return (gfslope * curr_stop_depth) + gfhi;
```

Together they define the straight line through `(first_stop, GF_low)` and
`(final_stop, GF_high)` — the standard Baker construction. Theory:
[gradient factors](/domain/gradient-factors.md).

# Three caveats for a future caller

**The depths are pressures in bar**, on the absolute scale used everywhere else
(surface = 1.0). Nothing in the names says so. The unit tests pass 3.0 and 30.0,
which read naturally as metres — arithmetically self-consistent, physically
ambiguous. See [units and conventions](/domain/units-and-conventions.md).

**`gradient_factor()` intercepts at depth 0, not at the final stop.** It returns
exactly `GF_hi` when `curr_stop_depth == 0`. That is only the intended line if
`final_stop_depth` is itself 0. Pass `final_stop_depth = 0` — defining the GF
line to reach GF-high at the surface — or the line is displaced by
`slope × final_stop_depth`.

**The zero-slope fallback is deliberate.** Equal first and final depths give
slope 0 and hence a flat `GF = GF_hi` everywhere, which is the right degenerate
answer for a single-stop ascent.

Note the guard uses exact float comparison (`!= 0.0`). Depths differing by less
than an ULP produce a huge slope rather than the fallback. Not reachable with
realistic stop depths.

# Integration status

**Zero production callers.** `grep` finds these symbols only in
`gradientfactor.c` itself and in [the test suite](/tooling/test-suite.md).
Neither [`ceiling.c`](/components/ceiling.md) nor
[`dive.c`](/components/dive-cli.md) references them, so every ceiling the CLI
reports is raw Bühlmann at GF = 100.

Integrating them requires modifying `getCeiling()`, which is covered by the
[algorithm integrity policy](/decisions/algorithm-integrity-policy.md), and
requires first knowing the first-stop depth — machinery
[`stop.c`](/components/stop.md) does not provide.

# Test coverage

`test_gradient_factor()` covers a 30/85 slope, the equal-depths fallback, a
mid-depth value, and the depth-0 intercept. Complete for what the functions do
in isolation. Nothing tests them against a reference GF ascent profile, because
there is nothing to test that against yet.
