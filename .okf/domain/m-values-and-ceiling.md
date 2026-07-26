---
type: Domain Model
title: M-values and the decompression ceiling
description: How the a/b coefficients define maximum tolerated supersaturation, and the two different ways this codebase inverts them into a ceiling.
tags: [m-value, ceiling, supersaturation, safety-critical]
timestamp: '2026-07-25T09:30:00Z'
---

# The M-value line

Each compartment tolerates a maximum inert gas pressure that grows linearly
with ambient pressure:

```
M(P_ambient) = a + P_ambient / b
```

`a` has units of bar and is the tolerated supersaturation at zero ambient
pressure; `b` is dimensionless and is the slope's reciprocal. Both are per-gas
and per-compartment; see
[tissue constant tables](/components/tissue-constant-tables.md).

Fast compartments have large `a` and small `b` — they tolerate a lot of
supersaturation. Slow compartments have small `a` and `b` near 1 — they
tolerate very little. That ordering is asserted by the ZH-L16C unit tests.

# Inverting it: the ceiling

The quantity a dive planner actually wants is the inverse: given the current
tissue pressure, what is the shallowest ambient pressure that is still safe?

```
P_ceiling = (P_tissue − a) · b
```

A ceiling **≤ 1.0 bar** means the diver may surface directly. A ceiling above
1.0 bar is a decompression obligation, and

```
ceiling_depth_m = (P_ceiling_bar − 1.0) × 10
```

The dive's ceiling is the **maximum** across all compartments.

# Two gases, two formulations — and they disagree

With both nitrogen and helium dissolved, the `a` and `b` to use are not
obvious. This codebase contains **both** standard answers, in different
functions, and they do not agree:

### Weighted coefficients — the Bühlmann-standard answer

Blend `a` and `b` in proportion to each gas's share of the total inert load,
then apply the formula once to the combined pressure:

```
a = (P_N₂·a_N₂ + P_He·a_He) / (P_N₂ + P_He)
b = (P_N₂·b_N₂ + P_He·b_He) / (P_N₂ + P_He)
ceiling = (P_N₂ + P_He − a) · b
```

This is [`compartment_mvalue()`](/components/compartment.md). It is unit-tested
and **never called by production code**.

### Independent per gas — what the CLI actually uses

Compute a ceiling from each gas alone, ignoring the other, and take the larger:

```
ceiling = max( (P_N₂ − a_N₂)·b_N₂ , (P_He − a_He)·b_He )
```

This is [`getCeiling()`](/components/ceiling.md). It is what
[`dive.c`](/components/dive-cli.md) calls.

### The divergence

The two agree exactly whenever the gas actually present yields the larger of the
two per-gas terms, which covers every reachable air-diving state. On mixed loads
they do not, and the independent formulation reports the **shallower** — less
conservative — ceiling. Measured on ZH-L16C compartment 1:

| P_He (bar) | P_N₂ (bar) | `getCeiling` | `compartment_mvalue` | Difference |
|---:|---:|---:|---:|---:|
| 0.0 | 2.0 | 0.38781 | 0.38781 | 0 |
| 0.5 | 1.5 | 0.12581 | 0.30920 | **1.83 m shallower** |
| 1.0 | 1.0 | −0.13619 | 0.23658 | **3.73 m shallower** |
| 1.5 | 0.5 | −0.10290 | 0.16996 | **2.73 m shallower** |
| 2.0 | 0.0 | 0.10935 | 0.10935 | 0 |

Written up as [ceiling vs M-value divergence](/findings/ceiling-vs-mvalue-divergence.md).

# Gradient factors

Neither formulation applies conservatism. Modern practice interpolates between
ambient pressure and the M-value line using a gradient factor. The arithmetic
exists here but is not wired in — see [gradient factors](/domain/gradient-factors.md).

# From a ceiling to a plan

A ceiling is an instantaneous constraint, not a schedule. Turning the two
into a sequence of stop depths and durations is a separate algorithm, specified
in [ascent and stop scheduling](/domain/ascent-and-stop-scheduling.md) and not
implemented in this fork.

# Citations

[1] Erik C. Baker — *Understanding M-values*. In-repo: `doc/m-values_en.pdf`.
