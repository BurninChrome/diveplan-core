---
type: Domain Model
title: No-decompression limit (NDL)
description: What the no-decompression limit means, how it is defined against the ceiling, and the reference values this model produces.
tags: [ndl, no-deco, safety-critical]
timestamp: '2026-07-25T09:30:00Z'
---

# Definition

The **no-decompression limit** at a given depth is the longest time a diver can
remain there and still ascend directly to the surface without a mandatory stop.

In Bühlmann terms it is the largest `t` such that, after holding the current
depth for `t` more minutes, **every** compartment's
[ceiling](/domain/m-values-and-ceiling.md) is still at or below 1.0 bar.

Per compartment, the NDL is a closed-form inversion of the Haldane equation:
solve `(P(t) − a)·b = 1.0` for `t`. The dive's NDL is the **minimum** across
compartments. In this codebase it is instead found by iterative search — see
[`stop.c` / `nodecotime()`](/components/stop.md).

# Reference values for this model

Computed by reproducing the exact formulas of `alveolar.c`, `haldane.c` and
`ceiling.c` and bisecting for the crossing point. Air, surface-saturated
start, no ascent time credited, no gradient factor.

| Depth | ZH-L12 | ZH-L16C |
|---:|---:|---:|
| 10 m | > 100 min | > 100 min |
| 20 m | 55.4 min | 47.9 min |
| 30 m | 19.3 min | 17.0 min |
| 40 m | 7.5 min | 8.7 min |
| 50 m | 4.5 min | 5.6 min |
| 60 m | 3.3 min | 4.2 min |

These are the *model's own* limits, and they are not agency tables — recreational
tables build in extra conservatism, typically via gradient factors and a slower
ascent credit, and land near 20 min at 30 m rather than 19.3.

Note the tables cross over: ZH-L16C is more conservative than ZH-L12 in the
shallow range but more permissive below about 35 m. Do not assume "newer is
always more conservative" when comparing outputs across tables.

# What the shipped implementation returns

**Not these numbers.** [`nodecotime()`](/components/stop.md) over-reports by a
factor of 1.5–1.7 at every depth on both tables — at 30 m it reports 31.6
minutes against the 19.3 above. The per-depth comparison, the root cause and a
reproduction are in
[nodecotime over-reports NDL](/findings/nodecotime-overestimates-ndl.md); this
page is the canonical source for the reference column that finding compares
against.

# The 100-minute cap

Both the implementation and the table above cap at 100 minutes. `nodecotime()`
starts its search at `Addtime = 100.0` and returns that immediately if no
ceiling develops. So a reported **100.0 means "no decompression obligation
within 100 minutes"**, not "exactly 100 minutes". Consumers of the
[stdio output](/interfaces/dive-stdio-format.md) must treat 100.0 as a
saturating sentinel, not a measurement.

# Related

- The ceiling this is defined against: [M-values and ceiling](/domain/m-values-and-ceiling.md).
- The gas loading that drives it: [inert gas loading](/domain/inert-gas-loading.md).
- Conservatism that would shorten it: [gradient factors](/domain/gradient-factors.md).
