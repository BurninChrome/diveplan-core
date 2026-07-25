---
type: Domain Model
title: The Bühlmann ZH-L decompression model
description: How the Bühlmann algorithm models inert gas uptake and release across parallel tissue compartments, and what the ZH-L12 / ZH-L16 variants are.
tags: [buhlmann, decompression, theory, zh-l16, zh-l12]
timestamp: '2026-07-25T09:30:00Z'
---

# The idea

Breathing gas under pressure drives inert gas (nitrogen, and helium when
present) into body tissue. Ascending too fast lets that dissolved gas come out
of solution as bubbles — decompression sickness. A decompression model predicts
how much gas is dissolved where, and how shallow the diver can safely go.

Bühlmann's model — developed by Albert A. Bühlmann at the University Hospital
in Zürich from 1959, published 1983 — is a **perfusion-limited, parallel
compartment** model. It makes three simplifying assumptions:

1. The body is a bank of independent theoretical *compartments*, each
   characterised only by how fast it exchanges gas (its **half-time**).
   Compartments do not exchange gas with each other, only with the blood.
2. Each compartment approaches the inspired partial pressure **exponentially**.
   See [inert gas loading](/domain/inert-gas-loading.md).
3. Each compartment tolerates a maximum supersaturation — its **M-value** — that
   is a linear function of ambient pressure. See
   [M-values and the ceiling](/domain/m-values-and-ceiling.md).

The compartments are not anatomical. A "4-minute compartment" is a modelling
device, not a body part.

# Compartment banks

A compartment is defined by six numbers: a half-time and two M-value
coefficients (`a`, `b`), for each of N₂ and He. In code that is
`struct compartment_constants` — see [the C API](/interfaces/c-api.md).

| Variant | Compartments | Year | Status in this repo |
|---|---|---|---|
| **ZH-L12** | 16 | 1983 | Complete. **The one the CLI actually uses.** |
| **ZH-L16A** | 17 declared | 1990 | Under-initialised — [finding](/findings/zh-l16ab-phantom-compartment.md). |
| **ZH-L16B** | 17 declared | 1990 | Under-initialised — same finding. |
| **ZH-L16C** | 17 | 1990 | Complete and well-tested. |

The three ZH-L16 variants share half-times and differ only in the nitrogen `a`
coefficients:

- **A** is the theoretical set, derived directly from the half-times.
- **B** is A with the mid-range compartments made slightly more conservative;
  intended for table generation.
- **C** is B made more conservative still; **this is the variant validated for
  real-time use in dive computers**, and the de-facto standard in commercial
  dive planning software. The repository's `CLAUDE.md` names it the priority
  variant for testing.

ZH-L16 also adds a compartment ZH-L12 lacks: **compartment "1b"** (5.0 min N₂
half-time), sitting between compartments 1 and 2, which is why the tables are
17 rows for 16 numbered compartments.

See [tissue constant tables](/components/tissue-constant-tables.md) for the
actual numbers as they appear in this codebase.

# The per-step cycle

For each time step of a dive profile, for every compartment:

```
                 ambient pressure p, gas mix (fO2, fHe)
                                │
                                ▼
       ┌─────────────────────────────────────────────┐
       │ alveolar()   → inspired inert gas pressure  │  /components/alveolar.md
       │   subtract water vapour + CO2, scale by fN2 │
       └─────────────────────────────────────────────┘
                                │
                                ▼
       ┌─────────────────────────────────────────────┐
       │ Haldane (flat) or Schreiner (sloped)        │  /domain/inert-gas-loading.md
       │   → new tissue partial pressure             │
       └─────────────────────────────────────────────┘
                                │
                                ▼
       ┌─────────────────────────────────────────────┐
       │ M-value → ceiling for this compartment      │  /domain/m-values-and-ceiling.md
       └─────────────────────────────────────────────┘
                                │
                     max() across compartments
                                ▼
                    the dive's ceiling, in bar
```

The dive ceiling is the **maximum** across compartments (the most restrictive
compartment governs); the no-decompression limit is the **minimum** across
compartments (the first compartment to develop an obligation governs). See
[no-decompression limit](/domain/no-decompression-limit.md).

# What Bühlmann does not model

Deliberately out of scope for the classic algorithm, and absent here:

- Bubble mechanics (VPM, RGBM take this on).
- Deep stops. See `doc/deepstops.pdf` in the repository.
- Workload, temperature, thermal status, PFO, individual susceptibility.
- Oxygen toxicity — tracked separately, see [OTU](/domain/oxygen-toxicity.md).

Modern practice layers **gradient factors** on top to add conservatism; this
repository has the arithmetic but has not wired it in. See
[gradient factors](/domain/gradient-factors.md).

# Citations

[1] Bühlmann, A.A. — *Dekompression–Dekompressionskrankheit*, Springer, 1983.
[2] Erik C. Baker — *Understanding M-values*. Mirrored in the repository as
    `doc/m-values_en.pdf` (and `doc/m-values_fr.pdf`).
[3] Erik C. Baker — *Clearing Up The Confusion About "Deep Stops"*. Mirrored as
    `doc/deepstops.pdf`.
[4] `doc/mf2-elements-de-calcul-pour-l-elaboration-d-un-logiciel-de-decompression.pdf`
    — French-language derivation of the calculation elements, mirrored in-repo.
