---
type: Finding
title: The CLI runs on ZH-L12, not the ZH-L16C the project designates as primary
description: dive.c hard-wires the 1983 ZH-L12 table while CLAUDE.md and the test suite treat ZH-L16C as the variant that matters.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/dive.c
tags: [finding, architecture, zh-l16c, mismatch]
severity: medium
status: reported-awaiting-approval
timestamp: '2026-07-25T09:30:00Z'
---

# The mismatch

`CLAUDE.md`, the repository's own guidance file, states:

> **ZH-L16C is the most important** — it is the variant validated for use in
> dive computers and is the standard used in commercial dive planning software.
> Tests must cover ZH-L16C first and foremost.

The test suite follows that instruction thoroughly — 147 of its 202 runtime
checks are ZH-L16C, across four dedicated test functions.

[`dive.c`](/components/dive-cli.md) uses `zh_l12`. Exclusively, and at compile
time:

```c
struct compartment_state s[ZH_L12_NR_COMPARTMENTS];        /* line 12 */
for (i = 0; i < ZH_L12_NR_COMPARTMENTS; i++) { … }         /* lines 21, 58 */
compartment_descend(&zh_l12[i], …);                        /* line 60 */
ceiling   = fmax(getCeiling(&zh_l12[i], &s[i]), ceiling);  /* line 68 */
nodectime = fmin(nodecotime(&zh_l12[i], …), nodectime);    /* line 69 */
```

Three live references to the `zh_l12` symbol itself, plus three to
`ZH_L12_NR_COMPARTMENTS`. (Two further `zh_l12` mentions sit in commented-out
code at lines 71–72.)

So the well-tested table is the one nothing runs, and the running table has no
structural test coverage at all. `doc/api.md` records the situation accurately
("`zh_l12` — used by `dive.c` (current)") without flagging it as a problem.

# Why this is a finding and not just a note

It has three concrete consequences.

**The tests do not test what ships.** Every ZH-L16C assertion — the monotonicity
invariants, the trimix loading, the deco-obligation checks — validates a table
the binary never loads. Conversely `zh_l12` gets no invariant checks, and would
in fact **fail** the strict-monotonicity ones the ZH-L16C tests use: its `n2_a`
column repeats (0.455 across rows 7–10, 0.255 across rows 12–15) and so does
`n2_b`. See [the tables](/components/tissue-constant-tables.md).

**The models differ materially**, and not in one direction. ZH-L16C is more
conservative than ZH-L12 in the shallow range (17.0 vs 19.3 min at 30 m) and
more permissive below about 35 m (4.2 vs 3.3 min at 60 m). Anyone assuming "the
newer table is safer" will be wrong for deep dives. Per-depth figures for both
tables: [no-decompression limit](/domain/no-decompression-limit.md).

**ZH-L12 lacks separate helium M-values.** For the first nine compartments its
helium `a` and `b` are verbatim copies of the nitrogen ones — the variant
predates distinct helium coefficients. Trimix modelling on ZH-L12 is therefore
weaker than on ZH-L16C regardless of the
[ceiling-formula issue](/findings/ceiling-vs-mvalue-divergence.md).

# Switching is not a one-line change

The table name appears only three times, but the real coupling is the
compartment count:

1. `ZH_L12_NR_COMPARTMENTS` (16) → `ZH_L16_NR_COMPARTMENTS` (17), which changes
   the state array size and every loop bound.
2. **The output format grows from 36 fields to 38** — a breaking change to
   [the stdio contract](/interfaces/dive-stdio-format.md).
3. [`visoutput.py`](/tooling/visoutput.md) hard-codes 16 in `compList` and
   `np.arange(16)`, and slices compartments by index. It breaks silently.
4. Surface initialisation and the `fmax`/`fmin` aggregations follow the loop
   bound and need no separate change.

# Industry context — is ZH-L12 still used?

Researched 2026-07-25, because it bears directly on whether switching is worth
the output-format break.

**ZH-L16C is the de facto standard.** It is the computational basis of
essentially every current dive computer: Shearwater (ZH-L16C with gradient
factors, alongside optional VPM-B), Garmin, Mares, and — as one option among
several proprietary defaults — Suunto and Scubapro. Where a manufacturer ships
something else as standard, it is a proprietary model (Suunto Fused RGBM,
Scubapro PMG/ZH-L8 ADT), not an older Bühlmann variant.

**ZH-L12 is a 1983 algorithm superseded in 1990.** Its coefficients were
determined empirically through decompression trials, where ZH-L16A's were
derived analytically and B and C then tuned. It is not the current
recommendation for anything.

**But "obsolete" overstates it.** ZH-L12 still appears as a selectable option in
at least one current technical computer — the Ratio iX3M2 offers `BUL`
(ZH-L12) alongside `BUL16B`, `BUL16C` and VPM-B. So a ZH-L12 implementation is a
legitimate thing to have; what is hard to defend is having it as the *only*
option, unlabelled, while shipping a well-tested ZH-L16C that nothing loads.

**The two models are not ordered by conservatism.** Computed from this
repository's own tables — air, surface-saturated start:

| Depth | ZH-L12 NDL | ZH-L16C NDL | ZH-L12 is |
|---:|---:|---:|---|
| 20 m | 55.4 min | 47.9 min | less conservative |
| 30 m | 19.3 min | 17.0 min | less conservative |
| 40 m | 7.5 min | 9.0 min | more conservative |
| 50 m | 4.5 min | 6.1 min | more conservative |
| 60 m | 3.3 min | 4.7 min | more conservative |

On trimix 21/35 this repository's ZH-L12 is **less conservative at every depth**
(19.1 vs 17.3 min at 60 m; ceiling 0.14 m vs 0.70 m after 20 min at 60 m). That
is because ZH-L12 predates separate helium coefficients and copies its nitrogen
`a`/`b` into the helium columns for the first nine compartments — and its
nitrogen `a` values are far larger than ZH-L16C's (2.200 vs 1.1696 in
compartment 1), so helium ends up tolerating *more* supersaturation, not less.

Some secondary sources describe ZH-L12 as "excessively conservative with regard
to helium." **That is the opposite of what this implementation does.** Either
those sources describe a different property, or this repository's ZH-L12 table
is wrong — which is not idle speculation given that it is
[unverified and one coefficient pair short of its own name](/findings/zh-l12-unverified.md).
Do not rely on either reading until the table is checked against Bühlmann 1983.

# Recommendation

Two things, in this order.

**Make the choice explicit and reversible.** Introduce a single
translation-unit-local alias in `dive.c`:

```c
static const struct compartment_constants *const MODEL = zh_l12;
#define MODEL_NR_COMPARTMENTS ZH_L12_NR_COMPARTMENTS
```

and use it throughout. That is a mechanical refactor with no behavioural change,
it documents the decision in one place, and it makes the eventual switch a
two-line edit. It touches `dive.c`, which is a driver rather than an algorithm
file — but given the subject matter, run it past the
[integrity policy](/decisions/algorithm-integrity-policy.md) anyway.

**Then decide, deliberately, which table ships.** That is a product decision, not
a cleanup. It needs an ADR recording the reasoning, the output-format break, and
the visualiser update. If the answer is ZH-L16C, note that
[`zh_l16A` and `zh_l16B` are unusable](/findings/zh-l16ab-phantom-compartment.md)
— the switch is exactly the context in which someone tries them.

Whichever way it goes, add the ZH-L12 invariant tests (non-strict monotonicity)
so the shipping table is covered.
