---
type: Bug Report
title: Compartments are initialised at a different air composition than the simulation uses
description: dive.c seeds tissues with fN2 = 0.78084 but then loads them against 1 − 0.21 = 0.79, so a diver sitting at the surface slowly on-gasses.
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

[`gen_dive.py`](/tooling/gen-dive.md) holds `O2 = .20948` internally, but every
`out.write()` formats it with **`"%.2f"`**, so what actually crosses the process
boundary is `0.21` — giving `fN₂ = 0.79`.

The two disagree by 1.17%.

That rounding is easy to miss and worth stating explicitly: the generator's
internal constant is *not* the value the model sees. Reading `gen_dive.py`'s
source and reasoning from `0.20948` gives 0.79052 and a drift of 0.009073 bar;
both are wrong. The
[stdio format](/interfaces/dive-stdio-format.md) is the contract, and it carries
two decimals.

# Consequence

**Surface equilibrium is not a fixed point of the simulation.** A profile that
starts at the surface begins 0.008586 bar below the equilibrium the model is
integrating toward, so every compartment on-gasses from the first step even at
zero depth:

```
initial tissue N2        0.731881 bar   (from 0.78084)
simulated surface equil  0.740467 bar   (from 0.79)
drift                   +0.008586 bar   (+1.173%)
```

Confirmed against the compiled binary — feeding `dive` two surface samples six
hours apart yields `n2_c0 = 0.740467`, not the 0.740954 that `0.20948` would
imply.

Left at the surface for six hours, compartment 1's ceiling rises by 0.070 m
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
- **0.79** is what you get from `1 − fO₂` with the `fO₂ = 0.21` that actually
  reaches the model, and treats argon and trace gases as nitrogen — which is
  what the model does anyway, since it has no third inert gas.

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

**B — align the constants.** Change the initialisation to `1.0 - 0.21`, or widen
[`gen_dive.py`](/tooling/gen-dive.md)'s output format past `%.2f` and emit an
`fO₂` whose complement is 0.78084. One line, no structural change, but it leaves
the hard-coded assumption that every dive starts at sea level breathing air —
and note that changing the generator's format is itself a change to
[the stdio contract's](/interfaces/dive-stdio-format.md) effective precision.

Option A also makes the [test suite](/tooling/test-suite.md)'s pervasive
`0.78084` a test-only convention rather than a load-bearing constant.

# Impact of fixing

Every compartment's starting pressure shifts by ~0.0086 bar, so **every output
line of every profile changes slightly**. Any recorded baseline must be
regenerated. Ceilings move by well under a tenth of a metre; no
decompression decision realistically flips.

# Related

- [Units and conventions](/domain/units-and-conventions.md) — where both values
  are catalogued.
- [Alveolar pressure](/domain/alveolar-pressure.md) — derivation of the
  0.731881 seed.
