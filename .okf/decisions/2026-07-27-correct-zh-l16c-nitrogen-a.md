---
type: Decision Record
title: 'ADR: Correct the ZH-L16C nitrogen a coefficients'
description: Replaced eleven nitrogen a values with the published ZH-L16C figures; ten move the ceiling deeper. One unsourced b value deliberately left alone.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/doc/adr/20260727-1200-correct-zh-l16c-nitrogen-a.md
tags: [adr, zh-l16c, constants, safety-critical, fix]
timestamp: '2026-07-27T12:00:00Z'
---

> Mirrors `doc/adr/20260727-1200-correct-zh-l16c-nitrogen-a.md`.
> **The first algorithm-level fix in this project**, made under the
> [algorithm integrity policy](/decisions/algorithm-integrity-policy.md) with
> explicit approval.

# Decision

Eleven nitrogen `a` values in `zh_l16C` replaced with the published ZH-L16C
figures, for half-times 27.0 through 498.0 min. The unsourced
`zh_l16C[0].n2_b` was deliberately left alone.

# Why it was safe to make

**No current output changes.** `src/dive` loads `zh_l12`; nothing reads this
table. [`test_profiles`](/tooling/reference-vectors.md) confirmed it
empirically: after the change all `zh_l12` rows were byte-identical and only
four `zh_l16C` rows moved.

# What the safety net did

This was the first real exercise of the machinery built for it, and both halves
behaved as designed:

- **The ratchet fired.** `test_vectors` went from 12 allowed deviations to 1
  and reported `FIXED - update expected-failures.txt`, refusing to pass until
  the allowance was corrected in the same change.
- **The baseline caught the movement** and localised it to `zh_l16C` only,
  proving the change did not leak into the table the demo uses.
- **A stale-data interaction surfaced** that had not been anticipated:
  `ceiling.tsv` and `gradient-tolerance.tsv` hold values computed from the
  source tables, so a legitimate table change made them stale and they failed.
  Regenerating was correct — those vectors test the ceiling *rule* given
  whatever constants exist, while the constants themselves are checked against
  an independent transcription. Worth remembering for the next table change.

# The `b` value, and a lesson about evidence

`b` for the 4-minute compartment was 0.5240 — matching neither the derivation
(0.5050) nor any published table. Initially held back: the published C table
starts at 5.0 min and has no 4-minute compartment, so nothing states the value,
and correcting it moves the ceiling *less* conservative.

That was over-cautious. Unlike `a`, **`b` is never hand-modified between
variants**, and `b = 1.005 − 1/√t½` reproduces the published table to four
decimals in **15 of 17** compartments, with one rounding-level case and one
documented departure at t = 18.5. A relation validated on 16 of 17 cells is not
weak inference. 0.5240 corresponds to nothing at all.

Corrected in all three tables. ZH-L16C now matches the published reference in
**102 of 102 cells**.

# What the old test suite revealed

`test_buhlmann.c` asserted `zh_l16C[0].n2_b == 0.5240`. **The suite was pinning
the defect** — 202 assertions passed while the constant was wrong, because one
of them required it to be wrong. That is the sharpest possible argument for
[the conformance vectors](/tooling/reference-vectors.md): a test written from
the implementation cannot find a wrong constant, because it was written from
the wrong constant.

# Related

- [The finding](/findings/zh-l16-a-coefficients-nonstandard.md)
- [Tissue constant tables](/components/tissue-constant-tables.md)
- [Reference vectors](/tooling/reference-vectors.md)
