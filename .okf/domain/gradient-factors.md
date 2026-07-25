---
type: Domain Model
title: Gradient factors
description: The GF-low/GF-high conservatism scheme, how this repository parameterises it, and why it is not yet part of the model.
tags: [gradient-factor, conservatism, deco, not-integrated]
timestamp: '2026-07-25T09:30:00Z'
---

# The idea

Raw Bühlmann lets a compartment sit exactly on its M-value line. Gradient
factors (Erik Baker, 1998) pull the allowed supersaturation back toward ambient
pressure by a fraction `GF`:

```
P_allowed = P_ambient + GF · (M(P_ambient) − P_ambient)
```

`GF = 1.0` is unmodified Bühlmann; `GF = 0` forbids any supersaturation.

Two values are chosen, and GF is interpolated linearly between them by depth:

- **GF-low** applies at the *first* (deepest) stop. Lower values force deeper
  first stops. Typical 0.30–0.50.
- **GF-high** applies at the *last* stop and at the surface. Lower values
  lengthen shallow stops. Typical 0.70–0.85.

A "30/85" plan means GF-low 0.30, GF-high 0.85.

# How this repository parameterises it

Two functions in [`gradientfactor.c`](/components/gradientfactor.md):

```c
double gradient_factor_slope(double gfhi, double gflow,
                             double final_stop_depth, double first_stop_depth);
double gradient_factor(double gfslope, double curr_stop_depth, double gfhi);
```

giving

```
slope = (GF_hi − GF_low) / (depth_final − depth_first)
GF(d) = slope · d + GF_hi
```

Two things to know before using them:

**The depth parameters are pressures, in bar, on the same absolute scale as
everything else** — surface is 1.0, not 0.0. Nothing in the signature says so;
the [unit tests](/tooling/test-suite.md) pass 3.0 and 30.0, which read as metres
and make the test self-consistent but not physically meaningful. Consistency is
all that matters arithmetically, but a caller mixing the two conventions gets
silently wrong conservatism.

**`gradient_factor()` intercepts at `d = 0`, not at the final stop.** The
formula returns exactly `GF_hi` when `curr_stop_depth` is zero. That is correct
only if `depth_final` is itself zero. With `depth_final = 0.3` bar (a 3 m last
stop, per `STOPINC`) the line is off by `slope × 0.3`. Callers should pass
`final_stop_depth = 0` — i.e. define the GF line to reach GF-high at the
surface — or correct the intercept themselves.

`gradient_factor_slope()` returns `0.0` when the two depths are equal, which
degenerates to a flat `GF = GF_hi` at all depths. That is a sane fallback for a
single-stop or no-stop ascent.

# Integration status

**Not integrated.** Neither function is called from
[`dive.c`](/components/dive-cli.md) or from
[`ceiling.c`](/components/ceiling.md). The ceilings the CLI reports are raw
Bühlmann with no conservatism whatsoever.

Wiring them in means changing [`getCeiling()`](/components/ceiling.md), which
falls squarely under the
[algorithm integrity policy](/decisions/algorithm-integrity-policy.md) — write a
report and get approval first.

It also requires something the codebase does not yet have: knowledge of where
the first stop *is*. GF-low applies at the first stop depth, which is only
known after computing an initial ceiling. That circularity is normally resolved
by computing the raw ceiling first, rounding it up to a stop increment
(`STOPINC`, 0.3 bar), and using that as `first_stop_depth`. See
[decompression stops](/components/stop.md) for why that machinery is absent.

# Citations

[1] Erik C. Baker — *Clearing Up The Confusion About "Deep Stops"*. In-repo:
    `doc/deepstops.pdf`.
[2] Erik C. Baker — *Understanding M-values*. In-repo: `doc/m-values_en.pdf`.
