---
type: Bug Report
title: compartment_mvalue() returns NaN for an unloaded compartment
description: Both coefficient blends divide by the total inert pressure, which is zero for a zeroed compartment_state — a live blocker for the proposed getCeiling fix.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/compartment.c
tags: [bug, nan, robustness, unfixed]
severity: low
status: reported-awaiting-approval
timestamp: '2026-07-25T09:30:00Z'
---

> Filed under the [algorithm integrity policy](/decisions/algorithm-integrity-policy.md).
> **No code has been changed.**

# Affected code

[`compartment_mvalue()`](/components/compartment.md), `src/compartment.c` lines
7–18.

```c
a = ((compt->n2_p * constants->n2_a) + (compt->he_p * constants->he_a)) /
    (compt->n2_p + compt->he_p);
b = ((compt->n2_p * constants->n2_b) + (compt->he_p * constants->he_b)) /
    (compt->n2_p + compt->he_p);
return ((compt->n2_p + compt->he_p) - a) * b;
```

# Behaviour

With `n2_p == 0.0 && he_p == 0.0`, both blends evaluate `0.0 / 0.0`, which under
IEEE 754 is NaN. `a` and `b` are NaN, and the return value is NaN.

```c
struct compartment_state s = { .n2_p = 0.0, .he_p = 0.0 };
compartment_mvalue(&zh_l16C[0], &s);      /* NaN */
```

The state is easy to reach: a `struct compartment_state` from `calloc`, from a
`= {0}` initialiser, or from `memset` is exactly this. Nothing in the API
documents that a zeroed state is invalid, and there is no constructor that would
steer a caller away from it.

# Severity in isolation: low

`compartment_mvalue()` has **no production callers** — only the test suite,
which always supplies non-zero loading. Nothing in the shipped binary can reach
this today.

# Why it matters anyway

It is the blocking prerequisite for the recommended fix to
[the ceiling divergence](/findings/ceiling-vs-mvalue-divergence.md), which
proposes making `getCeiling()` delegate to `compartment_mvalue()`.
`getCeiling()` currently handles the zero state fine — it returns
`max(−a_N₂·b_N₂, −a_He·b_He)`, a well-defined negative number. Delegating
without a guard would **introduce** a NaN path into the production ceiling
calculation.

And NaN is the worst failure mode here, because `fmax(NaN, x)` returns `x`. A
NaN ceiling from one compartment is silently discarded by
[`dive.c`](/components/dive-cli.md)'s aggregation rather than propagating — the
compartment just stops contributing, and the reported ceiling is quietly
computed from a subset. Same swallowing behaviour as
[the phantom compartment](/findings/zh-l16ab-phantom-compartment.md).

# Proposed fix

**Not applied.** Guard the degenerate case:

```c
double total = compt->n2_p + compt->he_p;

if (total <= 0.0) {
    /* No dissolved inert gas: the compartment tolerates any ascent.
       Return the same sentinel getCeiling() produces for this state. */
    return fmax(-constants->n2_a * constants->n2_b,
                -constants->he_a * constants->he_b);
}

a = (compt->n2_p * constants->n2_a + compt->he_p * constants->he_a) / total;
b = (compt->n2_p * constants->n2_b + compt->he_p * constants->he_b) / total;
return (total - a) * b;
```

Using `<= 0.0` rather than `== 0.0` also catches negative pressures, which are
unphysical but not otherwise rejected anywhere.

Computing `total` once is a minor bonus — the current code evaluates the sum
three times.

There is a judgement call in what to return. Matching `getCeiling()`'s existing
value keeps the two functions consistent, which is the point of the delegation.
Returning `-INFINITY` or `0.0` would be defensible alternatives; consistency
seems more useful than either.

# Impact of fixing

None on current behaviour — the guarded branch is unreachable from every
existing call site. It is purely enabling work for the ceiling fix.

# Test to add

```c
struct compartment_state zero = { .n2_p = 0.0, .he_p = 0.0 };
ASSERT_TRUE(!isnan(compartment_mvalue(&zh_l16C[0], &zero)),
            "mvalue on zeroed state is not NaN");
ASSERT_NEAR(getCeiling(&zh_l16C[0], &zero),
            compartment_mvalue(&zh_l16C[0], &zero), EPSILON,
            "mvalue and getCeiling agree on the zero state");
```

The first assertion fails today.
