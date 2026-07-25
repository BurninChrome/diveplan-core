---
type: Domain Model
title: Oxygen toxicity and OTU
description: Pulmonary oxygen toxicity, the NOAA OTU unit, and the constant-depth and changing-depth integrals implemented here.
tags: [otu, oxygen-toxicity, noaa, not-integrated]
timestamp: '2026-07-25T09:30:00Z'
---

# Two kinds of oxygen toxicity

- **CNS toxicity** — acute, driven by high ppO₂ (above roughly 1.4–1.6 bar),
  risk of convulsion underwater. Tracked as "CNS %" against exposure-time
  limits. **Not modelled in this repository.**
- **Pulmonary toxicity** — cumulative lung irritation from prolonged moderate
  ppO₂. Tracked in **Oxygen Toxicity Units (OTU)**, also called Units of
  Pulmonary Toxic Dose. This is what [`otu.c`](/components/otu.md) computes.

A commonly cited operational budget is ~850 OTU in a single day and ~300/day
averaged over a multi-day series.

# The NOAA OTU formula

OTU accumulates only above a ppO₂ of **0.5 bar**. At constant ppO₂:

```
OTU = t · ( 0.5 / (ppO₂ − 0.5) ) ^ (−5/6)
```

which is `t` exactly when `ppO₂ = 1.0` bar, rises above `t` for higher ppO₂ and
falls below for lower. Implemented as `otu_const()`.

When ppO₂ changes linearly over the interval — a descent, an ascent, or a gas
switch ramp — the integral of that expression is:

```
OTU = (3/11)·t / (ppO₂_f − ppO₂_i)
      · [ ((ppO₂_f − 0.5)/0.5)^(11/6) − ((ppO₂_i − 0.5)/0.5)^(11/6) ]
```

Implemented as `otu_descend()`.

# The 0.5 bar threshold is a branch point, not just a floor

The `^(11/6)` term takes a **negative base** whenever ppO₂ < 0.5, and `pow()`
with a negative base and non-integer exponent returns NaN. A single NaN
propagating into an OTU accumulator poisons the whole dive's total.

The original guard — `if (ppO₂_i > 0.5 || ppO₂_f > 0.5)` — caught the case where
*both* endpoints are sub-threshold but let both mixed cases through, so any
descent that crossed 0.5 bar produced NaN. Fixed on 2026-03-10 by clamping each
endpoint to 0.5 before exponentiation while keeping the *unclamped* values in
the divisor, so the OTU is scaled against the full excursion. See
[the ADR](/decisions/2026-03-10-fix-otu-descend-nan.md).

# The units trap: ratio vs partial pressure

Both functions name their parameters `o2_ratio`, and the whole repository
otherwise uses `o2` to mean a **gas fraction** (0.21 for air). But the physics
is about **partial pressure in bar**. A gas fraction never exceeds 1.0, so
feeding fractions in means the 0.5 threshold fires only for mixes richer than
50% O₂ regardless of depth — 32% nitrox at 40 m has a ppO₂ of 1.6 bar and would
score zero OTU.

The correct call is `otu_const(t, fO2 × P_ambient)`. Nothing in the code or its
tests enforces this: the unit tests pass values like 0.6, 0.8 and 1.0 that read
naturally as either. **Any future caller must pass partial pressure, in bar.**

# Integration status

**Not integrated.** Neither function is declared in `buhlmann.h`, neither is
called from [`dive.c`](/components/dive-cli.md), and the
[output format](/interfaces/dive-stdio-format.md) has no OTU field. They are
compiled into the library and reachable only by an `extern` declaration, which
is how [the test suite](/tooling/test-suite.md) gets at them.

# Citations

[1] NOAA Diving Manual, oxygen exposure limits — the source of the OTU
    formulation and the 0.5 bar threshold.
[2] `doc/decolessons.pdf`, mirrored in the repository.
