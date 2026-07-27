# ADR: Correct the ZH-L16C nitrogen `a` coefficients

Date: 2026-07-27 12:00

## Decision

Replace eleven nitrogen `a` values in `zh_l16C` (`src/zh-l16.c`) with the
published ZH-L16C values, for the compartments with half-times 27.0 through
498.0 minutes.

Deliberately **not** changed in this ADR: `zh_l16C[0].n2_b`, which is 0.5240
where the derivation gives 0.5050. See "What is left" below.

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

## What is left

`zh_l16C[0].n2_b = 0.5240` still deviates; the formula gives 0.5050. It was not
changed because **no source was found that states it**. The published ZH-L16C
table has 16 compartments beginning at 5.0 minutes and contains no 4-minute
compartment at all; this library's table has 17 and does.

0.5050 is a *derivation*: variants differ only in `a`, so compartment 1 is
identical across A/B/C, and A is defined by `b = 1.005 − 1/√t½`. That reasoning
is sound but it is not a transcription, and the change would move the ceiling
in the **less** conservative direction. Given that the defect being fixed here
was itself a case of an unsourced value in the unsafe direction, this one waits
for a source.

Tracked in `.okf/findings/zh-l16-a-coefficients-nonstandard.md` and in the
allowance line for `zh-l16c-published.tsv`.
