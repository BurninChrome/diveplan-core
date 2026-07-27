# ADR: Correct the ZH-L16C nitrogen `a` coefficients

Date: 2026-07-27 12:00

## Decision

Replace eleven nitrogen `a` values in `zh_l16C` (`src/zh-l16.c`) with the
published ZH-L16C values, for the compartments with half-times 27.0 through
498.0 minutes.

Also correct `n2_b` for the 4-minute compartment from 0.5240 to 0.5050, in
**all three** tables. After this the ZH-L16C table matches the published
reference in 102 of 102 cells.

## Context

The table did not match published ZH-L16C. Found by
`.okf/findings/zh-l16-a-coefficients-nonstandard.md` and confirmed by
`test/vectors/zh-l16c-published.tsv`, which compares the table against a
retyped published reference held independently of `src/`.

Every value in the table was too high, meaning the model tolerated more
supersaturation than the standard permits:

| t½ (min) | was | now | ceiling at 3 bar |
|---:|---:|---:|---:|
| 27.0 | 0.6491 | 0.6200 | +0.236 m deeper |
| 38.3 | 0.5316 | 0.5043 | +0.230 m |
| 54.3 | 0.4681 | 0.4410 | +0.236 m |
| 77.0 | 0.4301 | 0.4000 | +0.268 m |
| 109.0 | 0.4049 | 0.3750 | +0.272 m |
| 146.0 | 0.3719 | 0.3500 | +0.202 m |
| 187.0 | 0.3447 | 0.3295 | +0.142 m |
| 239.0 | 0.3176 | 0.3065 | +0.104 m |
| 305.0 | 0.2828 | 0.2835 | −0.007 m |
| 390.0 | 0.2716 | 0.2610 | +0.101 m |
| 498.0 | 0.2523 | 0.2480 | +0.041 m |

Ten of eleven move the ceiling deeper, i.e. **toward** the published standard
and toward conservatism. The 305-minute compartment moves 7 mm the other way;
it is the published value regardless.

## Sourcing

The values were taken from the published ZH-L16C parameter table (bar/minute
units) and checked twice against its raw source, then corroborated
structurally: the specification states that variant C carries *"more
conservative `a` values for tissue compartments #5 to 15"*, which predicts
exactly the eleven rows that differed, independently of the numbers themselves.

Note what this correction is **not** based on. An earlier version of the test
vectors checked `zh_l16C` against `a = 2/∛t½`, which is the formula that
*defines* variant **A**. Since B and C are A with `a` hand-lowered, that check
demanded C hold A's values — up to 0.089 bar *less* conservative than
published. That was caught in review before it could influence any change to
the source. Variant A is checked against the formula; C is checked against a
transcription.

## Consequences

- **No change to any current output.** `src/dive` is hard-wired to `zh_l12`, so
  nothing in the shipped demo loads this table. `test_profiles` confirms: after
  the change, all `zh_l12` rows are byte-identical and only the four `zh_l16C`
  rows moved.
- `test/vectors/expected-failures.txt` drops from 12 allowed deviations to 1.
- `test/vectors/ceiling.tsv` and `gradient-tolerance.tsv` were regenerated:
  their expected values are a function of the rule *and* the constants, so a
  legitimate table change makes them stale. The table itself is checked against
  the independent transcription, not against these, so there is no circularity.
- `test/profiles/expected.tsv` was re-recorded in the same commit.
- The library is now materially closer to being usable as ZH-L16C, which is the
  variant `CLAUDE.md` designates and the one current dive computers implement.

## The `b` value, and why it was initially held back

`b` for the 4-minute compartment was 0.5240, which matches neither the
derivation (0.5050) nor any published table. It was initially left alone
because the published ZH-L16C table has 16 compartments beginning at 5.0
minutes and contains no 4-minute compartment, so no source states the value
directly — and correcting it moves the ceiling in the *less* conservative
direction.

That caution was excessive, on evidence gathered afterwards. Unlike `a`, the
`b` column is **never hand-modified between variants**, and
`b = 1.005 − 1/√t½` reproduces the published table almost perfectly:

| Outcome | Compartments |
|---|---|
| exact to four decimals | 15 of 17 |
| rounding-level (Δ = 0.0001) | 1 (t = 27.0) |
| genuine published departure | 1 (t = 18.5, 0.7825 vs 0.7725, documented) |

A relation validated on 16 of 17 cells of the same table is not inference in
any weak sense. And 0.5240 corresponds to nothing at all: no formula, no
variant, no published figure.

Applied to `zh_l16A` and `zh_l16B` as well, which carried the same value.
Leaving one table corrected and its siblings wrong would be worse than either.

## What the existing test suite revealed

`test_buhlmann.c` asserted `zh_l16C[0].n2_b == 0.5240`. **The suite was pinning
the defect**, which is a large part of why it survived — 202 assertions passed
while the constant was wrong, because one of them required it to be wrong.
Updated to 0.5050.

That is the strongest available argument for the conformance vectors: a test
written from the implementation cannot find a wrong constant, because it was
written from the wrong constant.

## What is left in this table

`zh_l16A` still carries what appear to be ZH-L16B `a` values at four
compartments (t = 38.3, 54.3, 77.0, 305.0), and `zh_l16B` cannot be verified at
all. Both tracked in `.okf/findings/zh-l16-a-coefficients-nonstandard.md`;
neither is loaded by anything.
