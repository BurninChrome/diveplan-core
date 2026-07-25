---
type: Bug Report
title: zh_l16A and zh_l16B declare 17 compartments but initialise 16
description: A zero-filled 17th row gives half-time 0, producing NaN tissue pressures for any caller that trusts ZH_L16_NR_COMPARTMENTS.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/zh-l16.c
tags: [bug, latent, tables, zh-l16, unfixed]
severity: high
status: reported-awaiting-approval
timestamp: '2026-07-25T09:30:00Z'
---

> Filed under the [algorithm integrity policy](/decisions/algorithm-integrity-policy.md).
> **No code has been changed.**

# Affected code

`zh_l16A` and `zh_l16B` in
[`src/zh-l16.c`](/components/tissue-constant-tables.md), lines 29 and 50.

# The defect

Both arrays are sized by `ZH_L16_NR_COMPARTMENTS`, which is **17**:

```c
const struct compartment_constants zh_l16A[ZH_L16_NR_COMPARTMENTS] = {
    {    4.0f, 1.2599f, 0.5240f,    1.51f,  1.6189f, 0.4245f }, /*  1  */
 // {    5.0f, 1.1696f, 0.5578f,    1.88f,  1.6189f, 0.4770f }, /*  1b */
    {    8.0f, 1.0000f, 0.6514f,    3.02f,  1.3830f, 0.5747f }, /*  2  */
    …
    {  635.0f, 0.2327f, 0.9653f,  240.03f,  0.5119f, 0.9267f }, /* 16  */
};
```

Compartment **1b is commented out** in both, leaving 16 initialisers for a
17-element array. C zero-fills the remainder, so `zh_l16A[16]` and
`zh_l16B[16]` are `{0, 0, 0, 0, 0, 0}`. `zh_l16C` has all 17 and is unaffected;
`zh_l12` is sized by its own correct 16-element macro.

Verified by parsing the initialiser lists out of the source:

```
zh_l16C initializers: 17
zh_l16B initializers: 16     ← declared 17
zh_l16A initializers: 16     ← declared 17
zh_l12  initializers: 16     ← declared 16, correct
```

The declaration in `buhlmann.h` is `extern const struct compartment_constants
zh_l16A[];` — an incomplete type, so no caller can detect the shortfall with
`sizeof`, and no compiler warning fires at any level.

# Consequences at runtime

A half-time of zero gives `k = M_LN2 / 0 = +∞` in both loading equations:

| Call | Result |
|---|---|
| `haldane(pt0, palv, t>0, 0)` | `exp(-∞)` = 0 → returns `palv`. Instant equilibration. |
| `haldane(pt0, palv, 0, 0)` | `-∞ × 0` = NaN → **NaN**. |
| `schreiner(..., t>0, 0)` | `r/k`=0, `1/k`=0 → returns `palv + r·t`. |
| `schreiner(..., t=0, 0)` | **NaN**. |

And with `a = b = 0`, [`getCeiling()`](/components/ceiling.md) returns
`(p − 0) × 0 = 0` — or NaN once the tissue pressure is NaN.

Either way the failure is silent. A NaN propagates through the `fmax` ceiling
aggregation in [`dive.c`](/components/dive-cli.md) — and `fmax(NaN, x)` returns
`x`, so **the NaN is silently swallowed and the phantom compartment simply
vanishes from the ceiling**. No crash, no warning, a plausible-looking answer
computed from 16 real compartments plus one that contributes nothing.

The `fmin` NDL aggregation behaves the same way.

# Why it has not bitten

Nothing calls these tables. `grep` finds `zh_l16A` and `zh_l16B` only at their
definitions — not in [`dive.c`](/components/dive-cli.md), not in
[the test suite](/tooling/test-suite.md), not anywhere. They are pure latent
hazard: correct-looking, exported in a public header, and waiting for the first
person who writes `for (i = 0; i < ZH_L16_NR_COMPARTMENTS; i++)` against them.

That person is quite likely to be someone acting on
[the ZH-L16C finding](/findings/dive-uses-zh-l12-not-zh-l16c.md), since
switching tables is exactly the context in which A and B get tried.

# Proposed fix

Three options. **None applied.**

**A — uncomment row 1b in both tables.** Two lines, no macro change, both arrays
become genuinely 17 long. But the commented rows are copies of ZH-L16C's 1b
values, and it is not established that ZH-L16A and ZH-L16B are *supposed* to
have a 1b compartment. Historically ZH-L16 defines 1b for all three variants, so
this is probably right — but it changes published constants and needs a source
check against Bühlmann's tables before anyone commits to it.

**B — give each table its own correctly sized macro.** `ZH_L16AB_NR_COMPARTMENTS
= 16`, `ZH_L16C_NR_COMPARTMENTS = 17`. Honest about what is in the file, no
invented data. Callers must then pick the right macro, which is easy to get
wrong.

**C — delete `zh_l16A` and `zh_l16B`.** They are unused, and ZH-L16C is the
variant `CLAUDE.md` designates for use. Least code, no risk, loses the ability
to compare variants.

I recommend **A**, verified against a published ZH-L16 table, with **B** as the
fallback if the 1b values for A and B cannot be sourced.

Regardless of which is chosen, add the cheap guard to
[the test suite](/tooling/test-suite.md):

```c
static void test_all_tables_fully_initialised(void)
{
    for (int i = 0; i < ZH_L12_NR_COMPARTMENTS; i++)
        ASSERT_TRUE(zh_l12[i].n2_h > 0.0 && zh_l12[i].he_h > 0.0, "zh_l12 row");
    for (int i = 0; i < ZH_L16_NR_COMPARTMENTS; i++) {
        ASSERT_TRUE(zh_l16A[i].n2_h > 0.0 && zh_l16A[i].he_h > 0.0, "zh_l16A row");
        ASSERT_TRUE(zh_l16B[i].n2_h > 0.0 && zh_l16B[i].he_h > 0.0, "zh_l16B row");
        ASSERT_TRUE(zh_l16C[i].n2_h > 0.0 && zh_l16C[i].he_h > 0.0, "zh_l16C row");
    }
}
```

Three lines of substance, and it fails today.

# Impact of fixing

Option A changes no existing output, because no code path reads these tables.
Options B and C are header/API changes with no behavioural effect either. This
is a rare case where the safety-critical fix is free.

# Also note

`zh_l16A[0]` and `zh_l16B[0]` carry `he_a = 1.6189` where `zh_l16C[0]` has
`1.7424`. That row is compartment 1's half-times paired with compartment 1b's
helium intercept — an artefact of deleting the 1b row rather than a real
variant difference. Whichever option is chosen, that value wants checking too.
