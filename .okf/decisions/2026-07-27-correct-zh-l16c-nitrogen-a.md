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

# The one left behind

`zh_l16C[0].n2_b = 0.5240` against a derived 0.5050. Not changed: the published
C table has 16 compartments starting at 5.0 min and contains no 4-minute
compartment, so nothing states this value. 0.5050 follows from variants
differing only in `a`, but that is inference, and the change would move the
ceiling **less** conservative. Since the defect just fixed was itself an
unsourced value in the unsafe direction, this one waits for a source.

# Related

- [The finding](/findings/zh-l16-a-coefficients-nonstandard.md)
- [Tissue constant tables](/components/tissue-constant-tables.md)
- [Reference vectors](/tooling/reference-vectors.md)
