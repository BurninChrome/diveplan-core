---
type: Bug Report
title: getCeiling() and compartment_mvalue() disagree on trimix by up to 3.7 m
description: Two implementations of the combined-gas ceiling coexist; the one the CLI uses evaluates each gas independently and reports a shallower, less conservative ceiling.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/ceiling.c
tags: [bug, safety-critical, ceiling, trimix, unfixed]
severity: high
status: reported-awaiting-approval
timestamp: '2026-07-25T09:30:00Z'
---

> Filed under the [algorithm integrity policy](/decisions/algorithm-integrity-policy.md).
> **No code has been changed.**

# Affected code

- [`getCeiling()`](/components/ceiling.md) — `src/ceiling.c` lines 10–25. **This
  is what the CLI calls.**
- [`compartment_mvalue()`](/components/compartment.md) — `src/compartment.c`
  lines 7–18. Never called outside tests.

# The two formulations

`compartment_mvalue()` blends the coefficients by each gas's share of the total
inert load, then applies the M-value formula once:

```c
a = (n2_p·n2_a + he_p·he_a) / (n2_p + he_p);
b = (n2_p·n2_b + he_p·he_b) / (n2_p + he_p);
return ((n2_p + he_p) - a) * b;
```

`getCeiling()` computes a ceiling from each gas as if the other were absent and
returns the larger:

```c
return fmax((n2_p - n2_a) * n2_b, (he_p - he_a) * he_b);
```

The first is the standard Bühlmann rule for mixed inert gas, and is what Baker's
M-value treatment and mainstream implementations use. The second is not a
recognised variant.

# Erroneous behaviour

They agree exactly whenever the gas that is actually present produces the larger
of the two per-gas terms — which covers every air dive. On mixed loads they
diverge, and `getCeiling()` — the one in the production path — always reports
the **shallower** ceiling. Shallower means the diver is told they may ascend
higher than the Bühlmann rule permits.

ZH-L16C compartment 1, total inert load held at 2.0 bar:

| P_He | P_N₂ | `getCeiling` (bar) | `compartment_mvalue` (bar) | Difference |
|---:|---:|---:|---:|---:|
| 0.0 | 2.0 | 0.38781 | 0.38781 | — |
| 0.5 | 1.5 | 0.12581 | 0.30920 | **1.83 m shallower** |
| 1.0 | 1.0 | −0.13619 | 0.23658 | **3.73 m shallower** |
| 1.5 | 0.5 | −0.10290 | 0.16996 | **2.73 m shallower** |
| 2.0 | 0.0 | 0.10935 | 0.10935 | — |

The divergence peaks near a 50/50 split and vanishes at both ends. It is
structural, not a rounding artefact: taking the max of two independent ceilings
throws away the fact that both gases are supersaturating the *same* tissue
simultaneously.

The absolute values above are below 1.0 bar, so at this loading no stop is
required under either rule. The error matters at loadings where the ceiling
straddles a stop increment — which is precisely the decision boundary a
decompression algorithm exists to resolve.

# Which one is right?

`compartment_mvalue()`. The physical claim behind the M-value is that a tissue
tolerates a total dissolved gas pressure; with two inert gases present the
tolerance is the load-weighted blend of their individual tolerances. Treating
them independently asks "would nitrogen alone be a problem? would helium alone
be a problem?" and answers no to both while their sum is a problem.

This also matches the repository's own documentation: `doc/api.md` describes
`compartment_mvalue()` as computing "the M-value ... using a weighted average of
the N₂ and He Bühlmann coefficients", and flags the `getCeiling()` difference in
a note without resolving it.

# Why the tests pass

`test_getCeiling()` has a mixed-gas case, but it asserts the result equals
`fmax(n2_ceil, he_ceil)` — the function's own formula restated. It verifies the
code does what the code does. `test_compartment_mvalue()` does the same for the
other formula. Neither compares the two, and no test states what a trimix
ceiling *should* be. See [test suite](/tooling/test-suite.md).

# Reproduction

Transcribed both formulas into Python and evaluated across the mixing range.
Method: [verification method](/decisions/2026-07-25-bundle-verification-method.md).

To verify with a compiler:

```c
struct compartment_state s = { .he_p = 1.0, .n2_p = 1.0 };
printf("getCeiling         = %.5f\n", getCeiling(&zh_l16C[0], &s));
printf("compartment_mvalue = %.5f\n", compartment_mvalue(&zh_l16C[0], &s));
/* expect -0.13619 and 0.23658 */
```

# Scope of the impact today

**Air-only dives are unaffected.** With `he_p = 0` the helium term is the fixed
negative constant `−a_He·b_He`, and the nitrogen term `(n2_p − a_N₂)·b_N₂` is at
its smallest when `n2_p = 0`, where it equals `−a_N₂·b_N₂`. Since
`a_N₂·b_N₂ ≤ a_He·b_He` in **every row of both usable tables** (checked; no
violations), the nitrogen term wins for all `n2_p ≥ 0` — so `getCeiling()`
reduces to the correct single-gas formula at any nitrogen loading whatsoever,
not merely realistic ones.

Agreement is numerical, not bitwise. `compartment_mvalue()` computes
`a = (n2_p·a_N₂)/n2_p`, which is not guaranteed to round back to `a_N₂`.
Evaluated over all 33 compartments of both tables at six loadings — 198 cases —
the two agree to within **4.4e-16 absolute**, with 21 of the 198 differing in
the last bit. At the `%lf` six-decimal output precision the printed values are
identical, but a bit-exact comparison of `double`s is not guaranteed.

Every profile [`gen_dive.py`](/tooling/gen-dive.md) produces by default, and
every dive in `test/xml/`, is air.

(The symmetric case does *not* hold: with nitrogen at exactly zero and helium
below the crossover `a_He − (a_N₂·b_N₂)/b_He` — 0.17 to 0.27 bar across ZH-L16C,
and exactly 0 for ZH-L12 rows 0–8, where the helium and nitrogen coefficients
are identical — the absent nitrogen's sentinel term is the larger and
`getCeiling()` returns it instead of the helium ceiling. Both values are deeply
negative and unreachable in practice, but it means "they agree when one gas is
absent" is a fact about these particular coefficient tables, not a theorem.)

The divergence is live only for trimix, which the library fully supports, the
[input format](/interfaces/dive-stdio-format.md) carries a helium field for, and
`test_zh_l16c_trimix()` exercises.

# Proposed fix

**Not applied.** Make `getCeiling()` delegate:

```c
double getCeiling(const struct compartment_constants *constants,
                  struct compartment_state *compt)
{
    return compartment_mvalue(constants, compt);
}
```

This is a one-line change with two prerequisites:

1. **`compartment_mvalue()` returns NaN when both pressures are zero** —
   0/0 in both blends. `getCeiling()` has no such problem today. That must be
   fixed first or the delegation introduces a new failure mode. See
   [the NaN finding](/findings/mvalue-division-by-zero.md).
2. `test_getCeiling()`'s mixed-gas assertion encodes the old formula and will
   fail. It should be replaced with the weighted expectation, not deleted.

Alternatively keep both functions and have the caller choose — but two live
answers to one question is how this arose.

# Impact of fixing

- Trimix dives: ceiling deepens by up to ~4 m at peak divergence, which is the
  conservative direction.
- Air dives: **unchanged at output precision.** The nitrogen-only path is
  algebraically the same expression; it can differ in the last bit of the
  `double` (see above) but not in the six decimals `dive` prints.
- Field 34 of [the output format](/interfaces/dive-stdio-format.md) changes for
  helium-bearing profiles only.
- [`nodecotime()`](/components/stop.md) calls `getCeiling()` internally, so
  trimix NDLs shorten too — on top of
  [their own 1.5× error](/findings/nodecotime-overestimates-ndl.md).

# Recommendation

Fix, after the NaN guard. Air output is unchanged at printed precision, so the
risk is confined to the trimix path that is currently wrong. Add a test asserting the
two functions agree, so they cannot drift apart again.
