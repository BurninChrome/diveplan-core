---
type: Domain Model
title: Alveolar inert gas pressure
description: Why inspired inert gas pressure is not just ambient pressure times gas fraction, and what the respiratory quotient does.
tags: [alveolar, dalton, water-vapour, respiratory-quotient]
timestamp: '2026-07-25T09:30:00Z'
---

# Why a correction is needed

Gas in the tank is dry. Gas in the alveoli is saturated with water vapour at
body temperature and is carrying metabolic CO₂. Both displace inert gas, so the
inert gas partial pressure actually presented to the blood is **lower** than
`ambient × fraction`.

The Bühlmann/Schreiner alveolar equation:

```
P_alv = ( P_ambient − P_H₂O + ((1 − RQ)/RQ) · P_CO₂ ) · f_inert
```

Implemented as `ventilation()` in [`alveolar.c`](/components/alveolar.md).

# The constants

| Symbol | Macro | Value | Meaning |
|---|---|---|---|
| `P_H₂O` | `WATER_VAPOR_PRESSURE` | 0.0627 bar | Water vapour at 37 °C. |
| `P_CO₂` | `CO2_PRESSURE` | 0.0534 bar | Alveolar CO₂. |

0.0627 bar is the standard 47 mmHg saturated vapour pressure at body
temperature. Implementations differ slightly in the value they adopt; the choice
shifts every tissue pressure by a fraction of a percent and is not worth
changing without a sourced reason.

# The respiratory quotient

`RQ` is the ratio of CO₂ produced to O₂ consumed. It controls how much the CO₂
term contributes. Three values are defined in `buhlmann.h`:

| Macro | Value | Convention | Used where |
|---|---|---|---|
| `BUHLMANN_RQ` | 1.0 | Bühlmann | **Everywhere in production code.** |
| `SCHREINER_RQ` | 0.8 | Schreiner / physiological | One unit test only. |
| `USNAVY_RQ` | 0.9 | US Navy tables | Never referenced. |

At `RQ = 1.0` the term `((1 − RQ)/RQ) · P_CO₂` is exactly zero, so the equation
collapses to `(P_ambient − 0.0627) · f_inert`. This is the conservative choice:
lower RQ raises the computed inspired pressure, which raises tissue loading.

`RQ = 0` would divide by zero. Nothing guards against it, but nothing passes it
either.

# Worked value

Surface equilibrium on air, as [`dive.c`](/components/dive-cli.md) initialises
every compartment:

```
P_N₂ = (1.0 − 0.0627 + 0) × 0.78084 = 0.731881 bar
P_He = (1.0 − 0.0627 + 0) × 0.0      = 0.0 bar
```

`0.731881332` is therefore the starting nitrogen pressure of all 16
compartments in every run. It is a useful sentinel when debugging output.

Note the nitrogen fraction used is **0.78084** (dry atmospheric N₂), not the
0.79 commonly rounded to in dive tables, and not `1 − 0.20948 = 0.79052` which
would be consistent with the O₂ fraction [`gen_dive.py`](/tooling/gen-dive.md)
emits. The inconsistency is small (≈1.2%) and long-standing.

# Related

- Consumed by both [Haldane and Schreiner](/domain/inert-gas-loading.md).
- Not to be confused with oxygen partial pressure, which drives
  [oxygen toxicity](/domain/oxygen-toxicity.md) and is computed separately
  without the water-vapour correction.
