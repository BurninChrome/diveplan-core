---
type: Decision Record
title: 'ADR: Fix otu_descend() NaN when ppO₂ crosses 0.5 bar'
description: Clamped both endpoints to the toxicity threshold before exponentiation, eliminating NaN on any depth change that crossed ppO2 = 0.5 bar.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/doc/adr/20260310-1200-fix-otu-descend-nan.md
tags: [adr, otu, nan, bugfix, historical]
timestamp: '2026-07-25T09:30:00Z'
---

> Mirrors `doc/adr/20260310-1200-fix-otu-descend-nan.md`. Commit `767112a`.

# Decision

Clamp both `o2_ratio_i` and `o2_ratio_f` to a minimum of 0.5 before computing
the power terms in [`otu_descend()`](/components/otu.md). Guard against division
by zero when both clamp to the same value.

# Context

The formula contains `pow((x − 0.5)/0.5, 11.0/6.0)`. When `x < 0.5` the base is
negative and a non-integer exponent makes `pow()` return NaN.

The existing guard `if (o2_ratio_i > 0.5 || o2_ratio_f > 0.5)` correctly skipped
the both-below case but passed both mixed cases straight through:

- `o2_i < 0.5, o2_f > 0.5` — descent into hyperoxia → NaN from the `i` term.
- `o2_i > 0.5, o2_f < 0.5` — ascent out of hyperoxia → NaN from the `f` term.

Physically, oxygen toxicity accumulates only above ppO₂ 0.5 bar, so the integral
should start or end at the threshold rather than at the out-of-range endpoint.

# Consequences

- Both power terms receive non-negative bases; NaN eliminated.
- The divisor `(o2_ratio_f − o2_ratio_i)` keeps the **unclamped** values, so the
  OTU is scaled against the full pressure excursion rather than only its
  hyperoxic portion.
- An `o2_f != o2_i` guard prevents division by zero when both endpoints clamp to
  0.5, returning 0.0.
- No change for cases where both endpoints were already above 0.5.
- Three new unit tests cover the previously broken cases.

# Why this one is worth studying

It is the cleanest example in the repository of the failure mode the
[integrity policy](/decisions/algorithm-integrity-policy.md) exists to catch.
The guard was not absent — someone had thought about the threshold and written a
condition. It was just the wrong condition: `||` where the domain needed
per-endpoint handling. The code read as if it were safe.

Note also the deliberate asymmetry in the fix: clamp the `pow()` bases, do
**not** clamp the divisor. Clamping both would have been the obvious symmetric
edit and would have silently changed the answer for every mixed case. The ADR
records that reasoning, which is exactly what makes it useful four months later.

# Still open

The parameter is named `o2_ratio` but must be a **partial pressure in bar**, not
a gas fraction. Neither ADR addresses it, and the unit tests use values readable
either way. Any future integration must pass `fO2 × P_ambient`. See
[oxygen toxicity](/domain/oxygen-toxicity.md) and [`otu.c`](/components/otu.md).

[`otu_const()`](/components/otu.md) has the same threshold structure but no
crossing problem, since it takes a single value and guards with `>` — a clean
`if (o2_ratio > 0.5)`.
