---
type: Finding
title: doc/api.md documents OTU bugs that were fixed four months ago
description: The API reference still describes otu_const() and otu_descend() as broken by integer division, telling readers the functions always return wrong values.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/doc/api.md
tags: [finding, documentation, stale, low-risk]
severity: medium
status: reported-awaiting-approval
timestamp: '2026-07-25T09:30:00Z'
---

# The problem

`doc/api.md` §6.6 carries two "Known bug" callouts:

> **Known bug:** The exponent `-5/6` uses C integer division, evaluating to `0`
> (not `-0.833`). This means `pow(..., 0) = 1`, so `otu = time` regardless of
> O₂ fraction.

> **Known bugs:** Integer division errors in the formula: `3/11` → `0` …
> `11/6` → `1` … As a result, `otu_descend()` always returns `0.0`.

Both were accurate when written on 2026-03-09. Both were fixed the same day, in
commit `cb25705`, and the fix is recorded in
[the modernisation ADR](/decisions/2026-03-09-modernize-build-and-python3.md).
The current source reads `-5.0/6.0`, `3.0/11.0` and `11.0/6.0` — see
[`otu.c`](/components/otu.md) — and
[the test suite](/tooling/test-suite.md) asserts the exact values.

`otu_descend()` was then fixed a second time on 2026-03-10 for a NaN on
threshold crossing ([ADR](/decisions/2026-03-10-fix-otu-descend-nan.md)). The
document does not mention the clamping that fix introduced, so §6.6's formula
block is stale in a second way.

# Why a doc bug is worth a finding

The document tells a reader the functions are unusable. That is a stronger claim
than being merely out of date: it actively steers people away from working code,
and it invites a "fix" that would re-break arithmetic that is now correct. In a
repository whose standing rule is
[do not touch the algorithms without approval](/decisions/algorithm-integrity-policy.md),
a document asserting a phantom algorithm bug is a real hazard.

# Other stale or imprecise passages in the same file

While verifying the above, four more:

**§6.4 calls `nodecotime()` a binary search.**

> Uses a binary-search-style iterative approach … The algorithm converges in at
> most 100 iterations using ×0.5 / ×1.5 bisection.

It is a geometric probe, not a bisection — there is no maintained bracket. The
phrasing lends unearned confidence to a function that
[over-reports by 1.5–1.7×](/findings/nodecotime-overestimates-ndl.md).

**§6.4 describes the return value as "the additional time that can be spent at
`p_ambient` before a ceiling develops."** That is what it should return, not
what it does.

**§5 lists `zh_l16A` and `zh_l16B` as `[17]` without qualification.** They are
declared 17 and initialised 16 — see
[the phantom compartment](/findings/zh-l16ab-phantom-compartment.md). The table
in §5 is exactly where a reader would look before using them.

**§6.4's note on `getCeiling()` vs `compartment_mvalue()`** correctly observes
that they differ, but presents it as a neutral design detail rather than a
divergence with a right answer and a wrong one. See
[the divergence finding](/findings/ceiling-vs-mvalue-divergence.md).

# What is still accurate

Most of it, and it is a genuinely good reference: the unit table, the data
structures, the constant tables, the formula transcriptions for
`ventilation`/`haldane`/`schreiner`, the gradient-factor section, all the usage
examples, and the I/O format description are correct as of today.

# Recommendation

Edit `doc/api.md` — a documentation change, outside the integrity policy:

1. Delete both OTU "Known bug" callouts; replace with a note that the exponents
   are floating-point and that `otu_descend()` clamps to the 0.5 bar threshold.
2. Reword §6.4's "binary search" and its return-value description; link the NDL
   finding.
3. Annotate `zh_l16A`/`zh_l16B` in §5 as not fully initialised.
4. Strengthen §6.4's `getCeiling` note into a pointer at the divergence.

More durably: **`doc/api.md` and this bundle now overlap heavily.** Two
hand-maintained descriptions of the same twelve functions is how the first
staleness happened. Worth deciding that one of them is canonical — the bundle is
the better candidate, being cross-linked and carrying the findings — and
reducing `doc/api.md` to a pointer, or generating it.
