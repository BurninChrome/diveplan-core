---
type: Decision Record
title: 'ADR: How this bundle''s numeric claims were verified'
description: No C toolchain was available, so the exact C formulas were transcribed into Python to produce every number quoted here; this records the method and its limits.
tags: [adr, method, provenance, verification]
timestamp: '2026-07-25T09:30:00Z'
---

# Decision

Every numeric claim in this bundle is derived from a line-for-line Python
transcription of the C source, not from running the compiled `dive` binary. Each
claim states which. The transcription is a throwaway artefact and is **not**
committed to the repository.

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

# Related

- [Findings index](/findings/index.md) — every claim this method produced.
- [Test suite](/tooling/test-suite.md) — what would run, given a compiler.
