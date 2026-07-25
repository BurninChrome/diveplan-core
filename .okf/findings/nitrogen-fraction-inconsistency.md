---
type: Bug Report
title: Compartments are initialised at a different air composition than the simulation uses
description: dive.c seeds tissues with fN2 = 0.78084 but then loads them against 1 − 0.20948 = 0.79052, so a diver sitting at the surface slowly on-gasses.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/dive.c
tags: [bug, initialisation, consistency, unfixed]
severity: low
status: reported-awaiting-approval
timestamp: '2026-07-25T14:30:00Z'
---

> Affects model output, so treat as covered by the
> [integrity policy](/decisions/algorithm-integrity-policy.md).
> **No code has been changed.**

# The inconsistency

Two different values for the nitrogen fraction of air are in play.

**Initialisation** — [`dive.c`](/components/dive-cli.md) line 23 hard-codes dry
atmospheric N₂:

```c
s[i].n2_p = ventilation(lastp, BUHLMANN_RQ, 0.78084);   /* = 0.731881 bar */
```

**Simulation** — the per-step nitrogen fraction is derived from the input:

```c
compartment_descend(..., 1.0 - o2 - he, he);
```

and [`gen_dive.py`](/tooling/gen-dive.md) emits `O2 = 0.20948`, giving
`fN₂ = 0.79052`.

The two disagree by 1.24%.

# Consequence

**Surface equilibrium is not a fixed point of the simulation.** A profile that
starts at the surface begins 0.009073 bar below the equilibrium the model is
integrating toward, so every compartment on-gasses from the first step even at
zero depth:

```
initial tissue N2        0.731881 bar   (from 0.78084)
simulated surface equil  0.740954 bar   (from 0.79052)
drift                   +0.009073 bar   (+1.240%)
```

Left at the surface for six hours, compartment 1's ceiling rises by 0.074 m
purely from this mismatch. That is negligible for dive planning, but it means:

- A "surface interval" segment produces a slow upward creep in every
  compartment rather than a flat line, which is confusing when reading output or
  debugging.
- Off-gassing computations start from a slightly wrong baseline.
- The 0.731881 sentinel documented in
  [alveolar pressure](/domain/alveolar-pressure.md) is only valid on line one.

# Which value is right?

Both are defensible; using both is not.

- **0.78084** is dry atmospheric N₂ and is what the test suite uses throughout.
- **0.79052** is what you get from `1 − fO₂` with `fO₂ = 0.20948`, and treats
  argon and trace gases as nitrogen — which is what the model does anyway, since
  it has no third inert gas.

`1 − o2 − he` is the more principled rule for the *simulation*, because it
correctly handles nitrox and trimix where there is no "atmospheric" answer. The
cleanest resolution is therefore to make **initialisation** consistent with it
rather than the reverse: seed from the first input line's gas mix instead of a
hard-coded constant.

# Proposed fix

**Not applied.** Two options.

**A — seed from the first sample** (preferred). Read the first line before the
loop and initialise with its `1.0 - o2 - he` at its pressure. Correct for
altitude starts and for a diver already on nitrox, both of which the current
code gets wrong regardless of which constant is chosen.

**B — align the constants.** Change the initialisation to `1.0 - 0.20948`, or
change [`gen_dive.py`](/tooling/gen-dive.md) to emit `O2 = 0.21916` so that
`1 − fO₂ = 0.78084`. One line, no structural change, but it leaves the
hard-coded assumption that every dive starts at sea level breathing air.

Option A also makes the [test suite](/tooling/test-suite.md)'s pervasive
`0.78084` a test-only convention rather than a load-bearing constant.

# Impact of fixing

Every compartment's starting pressure shifts by ~0.009 bar, so **every output
line of every profile changes slightly**. Any recorded baseline must be
regenerated. Ceilings move by well under a tenth of a metre; no
decompression decision realistically flips.

# Related

- [Units and conventions](/domain/units-and-conventions.md) — where both values
  are catalogued.
- [Alveolar pressure](/domain/alveolar-pressure.md) — derivation of the
  0.731881 seed.
