---
type: Finding
title: zh_l12 is unverified — and it is the table the CLI actually runs
description: ZH-L12's coefficients are empirical, so no formula or mirrored table exists to check them against; the one set of constants that reaches production output has never been validated.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/zh-l12.c
tags: [finding, verification-gap, zh-l12, safety-critical, open]
severity: high
status: open-verification-gap
timestamp: '2026-07-25T14:30:00Z'
---

# The gap

[The literature validation](/decisions/2026-07-25-model-validation.md) checked
every equation and every ZH-L16 constant against an independent reference. It
could not do the same for `zh_l12`, and `zh_l12` is
[the only table the CLI loads](/findings/dive-uses-zh-l12-not-zh-l16c.md).

So the validated tables are the ones nothing runs, and the running table is
unvalidated. That is the same inversion as
[the test-coverage finding](/findings/dive-uses-zh-l12-not-zh-l16c.md), and it
compounds it: ZH-L16C has both test coverage and literature validation, ZH-L12
has neither.

# Why it could not be checked

ZH-L16A's coefficients are *derived*: `a = 2/∛t½`, `b = 1.005 − 1/√t½`. That
gives an exact, independent relation to test against, which is how the
[ZH-L16 discrepancy](/findings/zh-l16-a-coefficients-nonstandard.md) was found.

ZH-L12 has no such relation. As Baker puts it, its M-values *"were determined
empirically (i.e. with actual decompression trials)"* — twelve pairs of
coefficients spread across sixteen half-time compartments. There is no formula,
and unlike ZH-L16 the table is not widely mirrored in machine-readable form, so
there is nothing to diff against.

# A structural handle: "twelve pairs" — but the table has eleven

There is one independent check after all, and it is suggestive.

The name encodes the table's shape. Wikipedia's variant list gives ZH-L12 as
*"the set of parameters published in 1983 with 'Twelve Pairs of Coefficients for
Sixteen Half-Value Times'"*, and Baker independently says *"the ZH-L12 set has
twelve pairs of coefficients for sixteen half-time compartments."* So the
nitrogen `(a, b)` column should contain exactly **12 distinct pairs** spread
across 16 compartments, with repeats where compartments share coefficients.

Counting the distinct nitrogen pairs in `src/zh-l12.c` gives **11**:

| `a` | `b` | used by compartments |
|---:|---:|---|
| 2.200 | 0.820 | 1 |
| 1.500 | 0.820 | 2 |
| 1.080 | 0.825 | 3 |
| 0.900 | 0.835 | 4 |
| 0.750 | 0.845 | 5 |
| 0.580 | 0.860 | 6 |
| 0.470 | 0.870 | 7 |
| 0.455 | 0.890 | 8, 9 |
| 0.455 | 0.934 | 10, 11 |
| 0.380 | 0.944 | 12 |
| 0.255 | 0.962 | 13, 14, 15, 16 |

One pair short of the number in the algorithm's own name.

**This is a signal, not a proof.** The "twelve pairs" phrasing could plausibly
count differently — the helium column yields 10 distinct pairs and the two gases
together yield 13, so some counting conventions do reach 12. But the natural
reading is the nitrogen column, and on that reading a pair that should be
distinct has been collapsed into a neighbour. The candidates are the runs where
`a` repeats across differing `b` (compartments 8–11, all at `a = 0.455`) and the
four-compartment run at `a = 0.255`.

Given that the ZH-L16 tables in the same repository turned out to have
[eleven wrong cells](/findings/zh-l16-a-coefficients-nonstandard.md), this is
worth resolving rather than explaining away.

# What was checked, and what it is worth

Structural plausibility only:

- Nitrogen half-times increase monotonically, 2.65 → 635 min.
- All `b` coefficients lie in (0, 1).
- Helium half-times are shorter than nitrogen throughout.
- Helium `a`/`b` are verbatim copies of the nitrogen values for the first nine
  compartments, consistent with ZH-L12 predating separate helium coefficients.

None of that would catch a transcription error in a single digit. Given that
the ZH-L16 tables in the same repository turned out to have
[eleven wrong cells](/findings/zh-l16-a-coefficients-nonstandard.md), a
transcription error here is not hypothetical — it is the base rate.

Note also that ZH-L12 fails the strict-monotonicity invariants the ZH-L16C tests
assert (its `n2_a` repeats at rows 7–10 and 12–15, its `n2_b` likewise), so even
the structural checks that exist for ZH-L16C cannot be reused unmodified. See
[the tables](/components/tissue-constant-tables.md).

# What would close it

**Check `src/zh-l12.c` against Bühlmann's 1983 *Dekompression–
Dekompressionskrankheit*** (or the 1984 English translation,
*Decompression–Decompression Sickness*), which is where the ZH-L12 set was
published. This needs a person with the book; it is not resolvable from the
open web at the precision required.

Failing that, two weaker options:

1. **Cross-check against another implementation** that cites its source — weaker,
   because transcription chains propagate errors, which is plausibly how the
   ZH-L16 values in this repository went wrong.
2. **Switch the CLI to ZH-L16C** once
   [its coefficients are corrected](/findings/zh-l16-a-coefficients-nonstandard.md),
   which retires the question rather than answering it. ZH-L16C is the variant
   `CLAUDE.md` designates as primary and the one validated for dive computers,
   so this is likely the right destination regardless — see
   [the finding](/findings/dive-uses-zh-l12-not-zh-l16c.md).

# Priority

Highest-value outstanding verification in the repository. Every other finding
concerns code whose behaviour is at least *known*; this one is an unquantified
risk sitting directly in the production path. Until it is closed, the honest
statement about the shipped model is that its tissue constants have not been
checked against any source.

# Citations

[1] Erik C. Baker, *Understanding M-values* — in-repo `doc/m-values_en.pdf`;
    source of the "determined empirically" characterisation.
[2] Bühlmann, A.A., *Dekompression–Dekompressionskrankheit*, Springer 1983 —
    the primary source for ZH-L12. **Not consulted.**
