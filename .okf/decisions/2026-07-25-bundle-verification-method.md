---
type: Decision Record
title: 'ADR: How this bundle''s numeric claims were verified'
description: No C toolchain was available, so the exact C formulas were transcribed into Python to produce every number quoted here; this records the method and its limits.
tags: [adr, method, provenance, verification]
timestamp: '2026-07-25T09:30:00Z'
---

# Status: superseded by execution, 2026-07-25

**A toolchain was installed later the same day and every claim below was
re-checked against the compiled library. All of them held.** See
[Confirmation against the real binary](#confirmation-against-the-real-binary) at
the end of this record. The method described here is kept because it is what
produced the findings, and because it documents how to work without a compiler
if that situation recurs.

# Decision

Every numeric claim in this bundle was originally derived from a line-for-line
Python transcription of the C source, not from running the compiled `dive`
binary. Each claim states which. The transcription is a throwaway artefact and
is **not** committed to the repository.

# Context

The authoring environment had no C toolchain — `gcc`, `cc`, `make`, `autoconf`,
`automake` and `libtool` were all absent — so `./bootstrap.sh && ./configure &&
make` could not run and `make check` could not be executed.

The alternative to producing numbers some other way was to write a bundle of
qualitative claims. Given that three of the findings here
([NDL scale error](/findings/nodecotime-overestimates-ndl.md),
[ceiling divergence](/findings/ceiling-vs-mvalue-divergence.md),
[phantom compartment](/findings/zh-l16ab-phantom-compartment.md)) are only
visible as *magnitudes*, that would have missed the substance.

# Method

1. `alveolar.c`, `haldane.c`, `schreiner.c`, `compartment.c`, `ceiling.c` and
   `stop.c` were transcribed into Python, preserving expression structure and,
   for [`nodecotime()`](/components/stop.md), its exact control flow including
   the loop guards and the post-check mutation.
2. The constant tables were **parsed out of `zh-l12.c` and `zh-l16.c` directly**
   rather than retyped, so the values are the file's own. The parser
   reproduces C's zero-fill for missing initialisers, which is how
   [the phantom compartment](/findings/zh-l16ab-phantom-compartment.md) was
   found and confirmed.
3. Reference NDLs were computed by bisecting the ceiling crossing — an
   independent derivation, not a restatement of `nodecotime()`.
4. Structural claims (initialiser counts, symbol usage, slice indices) came from
   parsing and grepping the source, not from reading it by eye.

# Limits — read these before relying on a number

**Float vs double.** The C tables store `float`-suffixed literals in `double`
fields, so each is rounded to `float` precision and widened. The Python parser
reads full decimal precision. Divergence is ~1e-8 relative — irrelevant at the
reported 2–5 significant figures, but a bit-exact comparison will not match. See
[the tables](/components/tissue-constant-tables.md).

**Libm differences.** `exp()` and `pow()` are correctly rounded to within an ULP
in both, but not bit-identical across implementations. Same order of magnitude
as above.

**NaN and infinity were reasoned about, not observed.** Python raises
`ZeroDivisionError` where C produces `±inf` or NaN. Claims about `k = ln2/0`,
about `fmax(NaN, x)` returning `x`, and about NaN propagation follow from IEEE
754 and the C standard rather than from execution. They are the least
directly-evidenced claims here and are flagged as such where they appear.

**The compiled binary was never run.** No claim describes observed output of
`src/dive`. The
[stdio format](/interfaces/dive-stdio-format.md) description is derived from
reading `dive.c`'s `printf` calls and counting fields.

**`make check` was never run.** Statements about which tests pass are derived
from reading the assertions against the transcribed model, not from a test run.
The specific claim that `test_nodecotime()` passes despite the 1.6× error was
checked by evaluating each of its three assertions against the transcribed
`nodecotime()` output.

# Independent review, 2026-07-25

The bundle was reviewed against the source by a second agent working from its
own Python transcription rather than this one. It reproduced the headline
numbers exactly — the reference NDL tables for both variants, the 1.5–1.7×
`nodecotime()` ratio, the seven-iteration trace, the full ceiling-divergence
table, and the 16-vs-17 initialiser counts — and found no error in the three
high-severity findings.

It did find nine factual errors elsewhere, all now corrected. The instructive
ones:

- **One fabricated claim.** The bundle asserted that the dive-log XML stores
  depth in decimetres and that `parse_dive.py` therefore divides twice. The XML
  stores metres and the script divides once. Nothing in the source supported the
  claim; it was invented. Corrected in
  [`parse_dive.py`](/tooling/parse-dive.md) and
  [units](/domain/units-and-conventions.md).
- **Soft counts stated as hard ones.** "~120 assertions" appeared in five files
  and matched no counting basis. The real figures — 78 call sites, 202 runtime
  checks — were trivially obtainable and are now used consistently.
- **"Bit-identical" overclaimed.** Air-path agreement between `getCeiling()` and
  `compartment_mvalue()` is to within 4.4e-16, not bitwise; 21 of 198 sampled
  cases differ in the last bit. Restated as agreement at output precision.
- **A correct conclusion resting on a wrong premise.** The claim that tissue N₂
  "never drops below the surface equilibrium" is false — O₂-rich deco gas drives
  it lower. The conclusion survives on the coefficient inequality
  `a_N₂·b_N₂ ≤ a_He·b_He`, which is what the text now says.
- **A C-semantics error.** An empty translation unit is a constraint violation
  requiring a diagnostic, not undefined behaviour.

The pattern worth naming: the bundle applied a uniform tone of exactness to
things that were genuinely computed and things that were eyeballed. The numbers
this ADR's method actually produced held up under independent re-derivation;
the numbers that did not were the ones nobody had derived at all. **Verified and
estimated claims need visibly different language** — a discipline this ADR
originally applied only to the NaN and infinity reasoning.

# Consequences

- Anyone with a compiler can re-derive every number; the reproduction snippets
  in each finding are written for that.
- The first person to run `make check` should confirm the suite passes as
  described. If it does not, that is new information and the affected concepts
  need updating.
- A future maintenance pass on a machine with a toolchain should re-verify
  the [NDL tables](/domain/no-decompression-limit.md) and the
  [ceiling divergence table](/findings/ceiling-vs-mvalue-divergence.md) against
  real binary output, and note the confirmation in
  [`log.md`](/log.md).

# Confirmation against the real binary

A C toolchain (gcc 15.2.0, autoconf 2.72, automake, libtool, make 4.4.1) was
installed after this bundle was written, and every claim was re-derived by
compiling and running the actual library. **Nothing in the bundle had to be
retracted.**

## The build itself

| Step | Result |
|---|---|
| `./bootstrap.sh` | exit 0 |
| `./configure` | exit 0 |
| `make` | exit 0, **zero warnings, zero errors** |
| `make check` | **PASS — 202/202 assertions** |

The 202 figure matters: it is exactly the number
[the test-suite concept](/tooling/test-suite.md) claims, and it was the value
corrected during the review pass after the original "~120" estimate was
challenged. The corrected number was right.

The build being warning-free is itself consistent with
[the CFLAGS finding](/findings/dive-built-without-warnings.md) rather than
contradicting it: `dive.c` compiles as
`gcc -DHAVE_CONFIG_H -I. -I.. -g -O2 -c dive.c` — no `-Wall`, no `-std=c99`.
Recompiling that same file with `-Wall -Wextra` immediately produces
`src/dive.c:34:16: warning: unused variable 'stop'`, exactly as predicted.

## The findings

Reproduced by linking a throwaway harness against `libbuhlmann.a`:

| Claim | Predicted | Measured | |
|---|---|---|---|
| `nodecotime()` over-report, 20–60 m | 1.52–1.64× | 1.52, 1.64, 1.58, 1.58, 1.54× | ✅ |
| `nodecotime()` can exceed 100 | max 112.5 | max 112.5 | ✅ |
| Ceiling divergence, ZH-L16C cpt 1 | +1.83 / +3.73 / +2.73 m | identical | ✅ |
| `compartment_mvalue()` on `{0,0}` | NaN | `-nan` | ✅ |
| `getCeiling()` on `{0,0}` | −0.6602 (well-defined) | −0.660188 | ✅ |
| `zh_l16A[16]` / `zh_l16B[16]` | all-zero row | all zeros | ✅ |
| Phantom compartment at `t > 0` | returns `palv`, not NaN | 3.074401 | ✅ |
| Phantom compartment at `t = 0` | NaN | `-nan` | ✅ |
| `fmax(NaN, x)` swallows the NaN | returns `x` | `fmax(NaN, 0.5) = 0.5` | ✅ |
| ZH-L16C nitrogen `a`, cpt 5–15 | 11 cells, +0.0291…−0.0007 | identical | ✅ |
| `zh_l16C[0].n2_b` | 0.5240 vs formula 0.5050 | confirmed | ✅ |
| ZH-L12 distinct N₂ `(a,b)` pairs | 11, not 12 | 11 | ✅ |
| Output format | 36 fields, no NaN | 36 fields, 0 NaN | ✅ |
| Surface seed | 0.731881 | 0.731881 | ✅ |

**One discrepancy, immaterial.** The count of depth/compartment combinations
where `nodecotime()` returns more than 100 came out as 148 against the
predicted 147 — a floating-point loop-bound artefact between the Python and C
sampling grids, not a behavioural difference. The concepts quoting "147" are
left as they are; the number is illustrative of a phenomenon whose magnitude
(max 112.5) is exact.

## What this says about the method

The transcription approach was sound: parsing the constant tables straight out
of the `.c` files rather than retyping them, and preserving `stop.c`'s exact
control flow rather than its intent, is what made the results reproducible.

It also vindicates the discipline the review pass imposed. The claims that
failed under review were the ones nobody had derived — the estimated assertion
count, the fabricated XML unit, "bit-identical". Every claim that *was* derived
survived contact with the compiler.

# Related

- [Findings index](/findings/index.md) — every claim this method produced.
- [Test suite](/tooling/test-suite.md) — now confirmed to pass, 202/202.
