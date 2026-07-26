---
type: Domain Model
title: Inert gas loading — Haldane and Schreiner
description: The two exponential gas-loading equations, when each applies, and why Schreiner degenerates to Haldane at zero rate.
tags: [haldane, schreiner, gas-loading, equations]
timestamp: '2026-07-25T09:30:00Z'
---

# The differential equation

A perfusion-limited compartment approaches the inspired inert gas pressure at a
rate proportional to the gradient:

```
dP_tissue/dt = k · (P_inspired − P_tissue)
```

where `k = ln(2) / half_time`. Everything below is a closed-form solution of
this equation under different assumptions about `P_inspired`.

# Haldane — constant depth

When ambient pressure is constant, `P_inspired` is constant and the solution is
the familiar exponential approach:

```
P(t) = P₀ + (P_alv − P₀) · (1 − e^(−k·t))
```

Symmetric in direction: the same equation describes on-gassing (`P_alv > P₀`)
and off-gassing (`P_alv < P₀`). After one half-time the compartment has closed
50% of the gap, after two 75%, after three 87.5%.

Implemented in [`haldane.c`](/components/haldane.md).

# Schreiner — changing depth

When ambient pressure changes linearly at rate `R` (bar/min, positive
descending), the inspired pressure is a ramp and the solution gains a linear
term:

```
P(t) = P_alv + R·(t − 1/k) − (P_alv − P₀ − R/k) · e^(−k·t)
```

This is the **Schreiner equation**. It is exact, not an approximation — no
sub-stepping is needed to model a descent or ascent segment.

Implemented in [`schreiner.c`](/components/schreiner.md).

Note the rate `R` here is the rate of change of the *inert gas* partial
pressure, not of ambient pressure: the caller must scale by the gas fraction.
[`compartment_descend()`](/components/compartment.md) does that scaling.

# The two agree at R = 0

Setting `R = 0` in Schreiner:

```
P(t) = P_alv − (P_alv − P₀)·e^(−k·t)
     = P₀ + (P_alv − P₀)·(1 − e^(−k·t))     ← Haldane
```

This identity is exercised as a unit test (`test_schreiner`, "rate=0 → equals
haldane") and is why [`dive.c`](/components/dive-cli.md) can call the Schreiner
path unconditionally for every step, including flat ones.

# At t = 0

Both equations return `P₀` exactly at `t = 0`, so a zero-length step is a
no-op. For Schreiner this is not obvious; the `R/k` terms cancel:

```
P(0) = P_alv + R·(0 − 1/k) − (P_alv − P₀ − R/k)·1
     = P_alv − R/k − P_alv + P₀ + R/k = P₀
```

This cancellation depends on `k` being finite. It is exactly what breaks for
the zero-filled phantom compartment in ZH-L16A/B, where `half_time = 0` gives
`k = ∞` and the expression evaluates to NaN — see
[the finding](/findings/zh-l16ab-phantom-compartment.md).

# What feeds these equations

Neither takes ambient pressure directly. Both take an *alveolar* partial
pressure, which accounts for water vapour and CO₂ in the lung. See
[alveolar pressure](/domain/alveolar-pressure.md).

# Citations

[1] Schreiner, H.R. & Kelley, P.L. — *A pragmatic view of decompression*, in
    Underwater Physiology IV, 1971.
[2] Haldane, J.S., Boycott, A.E., Damant, G.C.C. — *The prevention of
    compressed-air illness*, Journal of Hygiene 8(3), 1908.
