---
type: Test Suite
title: Test suite — coverage and gaps
description: What the C and Python tests actually verify, where the coverage is strong, and the three structural gaps that let the known defects survive.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/test/test_buhlmann.c
tags: [testing, coverage, quality]
timestamp: '2026-07-25T09:30:00Z'
---

# The two suites

**`test/test_buhlmann.c`** — 746 lines, no framework. Two macros (`ASSERT_NEAR`
with a 1e-6 default epsilon, `ASSERT_TRUE`), a pass/run counter, exit 0 iff all
pass. Registered as `TESTS` in `test/Makefile.am`, so `make check` runs it.

Two counts are worth keeping distinct, because several assertions sit inside
loops over compartments: **78 assertion call sites in the source**, expanding to
**202 runtime checks** — which is the number `tests_run` reports. Of those, 23
call sites / 147 runtime checks are ZH-L16C.

**`test/test_all_of_the_units.py`** — 33 lines, `unittest`. Two tests, one of
which asserts that `'hello world'.split()` works. The real one checks the first
line [`gen_dive.py`](/tooling/gen-dive.md) emits.

Neither runs in CI. `.travis.yml` targets a service shut down in 2021 and, even
if it ran, invokes only the Python file — **`make check` has never run in CI**.
See [build system](/tooling/build-system.md).

# C coverage by function

Assertion call sites; runtime checks in brackets where loops expand them.

| Function | Assertions | Quality |
|---|---|---|
| `ventilation` | 5 | Strong — both RQs, proportionality. |
| `haldane` | 7 | Strong — 1/2/3 half-times, saturation, off-gassing. |
| `schreiner` | 3 | Adequate — the `r=0`↔Haldane identity is the valuable one. |
| `getCeiling` | 5 | **Misleading — see below.** |
| `compartment_stagnate` | 4 | Good, incl. cross-check against `haldane` directly. |
| `compartment_descend` | 3 | Good, incl. the `rate=0`↔`stagnate` identity. |
| `compartment_mvalue` | 4 | Good — pure N₂, pure He, mixed, monotonic. |
| `nodecotime` | 3 | **Weak — see below.** |
| `otu_const` | 5 | Strong, exact values. |
| `otu_descend` | 7 | Strong — all four threshold-crossing combinations. |
| `gradient_factor*` | 4 | Complete for the arithmetic in isolation. |
| ZH-L16C constants | 16 [124] | Excellent — named values plus structural invariants. |
| ZH-L16C behaviour | 7 [23] | Equilibrium, loading order, deco obligation, trimix. |
| Integration (ZH-L12) | 5 | Surface equilibrium, loading order, deco obligation. |

The ZH-L16C block is the strongest part of the suite — nearly three quarters of
all runtime checks — and directly reflects `CLAUDE.md`'s instruction to
prioritise that variant. It checks named
coefficients *and* invariants: monotonic half-times, He faster than N₂, all `b`
in (0,1), `a` decreasing and `b` increasing with index.

# The three structural gaps

**1. Nothing checks a value against an independent reference.**
`test_nodecotime()` asserts the surface case is ≈100, that 50 m is somewhere in
`(0, 100)`, and that deeper is shorter. All three pass while the function
returns a number **1.5–1.7× too large**. Sign-and-ordering tests pin behaviour;
they cannot detect a scale error. Contrast the OTU tests, which compute the
expected value from the formula and compare — and which are the tests that
exist *because* two silent-wrong-answer bugs shipped.

See [nodecotime over-reports NDL](/findings/nodecotime-overestimates-ndl.md).

**2. Nothing checks that two implementations of the same concept agree.**
[`getCeiling()`](/components/ceiling.md) and
[`compartment_mvalue()`](/components/compartment.md) both answer "what is this
compartment's ceiling", by different rules. Each is tested against its own
formula, so both pass, and the divergence on mixed gas — up to 3.7 m — is
invisible.

See [ceiling vs M-value divergence](/findings/ceiling-vs-mvalue-divergence.md).

**3. Nothing tests the assembled program.** Every C test calls library
functions. No test feeds a profile into `dive` and inspects the output, so the
main loop's argument order, its choice of `lastp` over `p`, its aggregation
seeds and its 36-field output layout are all unverified. The 39 real dive logs
in `test/xml/` — see [`parse_dive.py`](/tooling/parse-dive.md) — are sitting
right there and unused.

# Other untested surface

- `zh_l12` structural invariants. Only ZH-L16C gets them, and ZH-L12 would fail
  the strict-monotonicity versions — its `a` and `b` columns repeat. See
  [the tables](/components/tissue-constant-tables.md).
- `zh_l16A` / `zh_l16B` — never referenced by any test, which is why the
  [phantom 17th row](/findings/zh-l16ab-phantom-compartment.md) went unnoticed.
  A single `n2_h > 0` loop over all four tables would have caught it.
- `compartment_mvalue({0,0})` → NaN. See
  [the finding](/findings/mvalue-division-by-zero.md).
- Aliased `cur == end` calls, despite production code relying on it.
- `he_ratio > 0` through `compartment_descend()` — trimix is only tested via
  `compartment_stagnate()`.
- [`parse_dive.py`](/tooling/parse-dive.md) and
  [`visoutput.py`](/tooling/visoutput.md): zero coverage.

# Highest-value additions, in order

1. **Wire `make check` into GitHub Actions.** Everything else is moot while the
   suite does not run.
2. **Golden-file regression over `test/xml/`.** Record today's 36-field output
   for all 39 dives; diff on every change. This catches scale errors, layout
   changes and table swaps at once, and needs no reference implementation.
3. **Assert `nodecotime()` against a bisection reference** computed in the test
   itself. This is the test that would have caught the live defect.
4. **A `n2_h > 0 && he_h > 0` loop over all four tables.** Three lines; catches
   the phantom compartment.

Note that (2) and (3) interact with the
[algorithm integrity policy](/decisions/algorithm-integrity-policy.md): a golden
file records current behaviour including known defects, so it must be
regenerated — deliberately, with review — when a defect is fixed.
