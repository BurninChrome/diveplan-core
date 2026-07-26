---
type: C Module
title: alveolar.c — ventilation()
description: Converts ambient pressure and a gas fraction into the alveolar inert gas partial pressure that drives tissue loading.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/alveolar.c
tags: [alveolar, source, layer-1]
timestamp: '2026-07-25T09:30:00Z'
---

# Signature

```c
double ventilation(double pamb, double rq, double ig_ratio);
```

| Parameter | Unit | Meaning |
|---|---|---|
| `pamb` | bar absolute | Ambient pressure. |
| `rq` | — | Respiratory quotient. Pass `BUHLMANN_RQ` (1.0). |
| `ig_ratio` | 0.0–1.0 | Inert gas fraction of the breathing mix. |

Returns the alveolar partial pressure of that inert gas, in bar.

# Implementation

```c
palv = (pamb - WATER_VAPOR_PRESSURE + ((1 - rq) / rq) * CO2_PRESSURE) * ig_ratio;
```

The whole module. Pure, stateless, 19 lines including includes.

Theory and the meaning of the constants: [alveolar pressure](/domain/alveolar-pressure.md).

# Behaviour notes

- **`rq = 0` divides by zero**, yielding ±∞ and then NaN. Unguarded. No caller
  passes it.
- With `BUHLMANN_RQ = 1.0` the CO₂ term vanishes exactly and the function
  reduces to `(pamb − 0.0627) × ig_ratio`. This is what happens in every
  production call.
- **The result goes negative for `pamb < 0.0627`** — i.e. above roughly 9 km
  altitude. Not a scenario this codebase supports, but worth knowing before
  anyone adds altitude diving.
- No clamping of `ig_ratio`. Passing a fraction > 1.0, or the sum of N₂ + He +
  O₂ exceeding 1.0, is silently accepted.

# Callers

Only [`compartment.c`](/components/compartment.md), twice per operation (once
per gas), plus surface initialisation in [`dive.c`](/components/dive-cli.md) and
the [test suite](/tooling/test-suite.md).

# Naming

The function is `ventilation()` but the file is `alveolar.c`, and the header
groups it with the pressure constants rather than the gas-loading declarations.
"Alveolar" is the accurate term for what it returns; "ventilation" describes the
process. Neither name is wrong, but grep for both.

# Test coverage

`test_ventilation()` in [the test suite](/tooling/test-suite.md) covers surface
and 20 m on air, the He = 0 case, `SCHREINER_RQ`, and pressure proportionality.
This is the best-covered function in the library.
