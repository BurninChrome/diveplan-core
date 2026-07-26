---
type: Playbook
title: Report a suspected algorithm bug
description: The required procedure when you find a defect in decompression mathematics, with a template and a worked example.
tags: [playbook, howto, process, safety]
timestamp: '2026-07-25T09:30:00Z'
---

# Rule zero

**Do not change the code.** Not even if the fix is obvious, not even if it is one
character. The
[algorithm integrity policy](/decisions/algorithm-integrity-policy.md) requires
a written report and explicit human approval first, for every file in the
protected set.

The reason is not bureaucratic. A decompression bug does not crash — it returns
a plausible number that injures someone hours later. The review step exists
because the person best placed to catch a wrong "fix" is not the person who
wrote it.

# Is it covered?

**Protected:** `haldane.c`, `schreiner.c`, `compartment.c`, `ceiling.c`,
`stop.c`, `alveolar.c`, `gradientfactor.c`, `otu.c`, `zh-l12.c`, `zh-l16.c`, and
all constant tables.

**Not protected:** `dive.c`, `getline.c`, `buhlmann.h`, the build system, Python
tooling, documentation, tests.

Judgement applies at the boundary. `dive.c` is not on the list but it selects
the constant table, chooses which pressure to pass, and owns the `fmax`/`fmin`
aggregation. **If your change alters model output, treat it as protected**
regardless of which file it lives in.

# The report

Five required elements, plus a reproduction. Write it as a new concept in
[`findings/`](/findings/index.md).

```markdown
---
type: Bug Report
title: <one line, states the defect and its magnitude>
description: <one sentence>
resource: <github blob URL of the affected file>
tags: [bug, unfixed, …]
severity: high | medium | low
status: reported-awaiting-approval
timestamp: <ISO 8601>
---

> Filed under the algorithm integrity policy. **No code has been changed.**

# Affected code
Function, file, line numbers. Who calls it, and how the result reaches a user.

# Erroneous behaviour
Concrete inputs → actual outputs vs expected. Quantify. A table beats prose.
State the direction: optimistic errors (shallower ceiling, longer NDL) are
more serious than conservative ones.

# Root cause
Why, in the code. Quote the lines. If two errors compound, separate them.

# Why the tests pass
If there is coverage and it did not catch this, explain what the tests assert
instead. This is usually the most useful section for preventing a recurrence.

# Reproduction
Something the reader can run. A test-suite snippet is ideal.

# Proposed fix
The change, marked **not applied**. Note prerequisites and alternatives, and
say which you recommend.

# Impact of fixing
Which outputs move, by how much, for which inputs. Which tests break. Whether
any recorded baseline needs regenerating.

# Recommendation
Fix / do not fix / fix later, and why.
```

# Quantify before you file

"Looks wrong" is not a report. Get a number.

Without a C toolchain, transcribe the relevant functions into Python and compute
both the actual and an independently derived expected value — the method used
for every finding here, documented in
[verification method](/decisions/2026-07-25-bundle-verification-method.md). With
a toolchain, add a temporary test.

The independent derivation matters. Comparing a function to its own formula
restated proves nothing; that is exactly why
[the NDL error](/findings/nodecotime-overestimates-ndl.md) and
[the ceiling divergence](/findings/ceiling-vs-mvalue-divergence.md) survived a
suite of 202 runtime checks. See [test suite](/tooling/test-suite.md).

# Worked example

[nodecotime over-reports the NDL](/findings/nodecotime-overestimates-ndl.md) is
the fullest example in this bundle: a magnitude table across six depths and two
constant tables, an iteration trace pinning the exact line, two compounding root
causes separated, an explanation of why three passing tests miss it, a runnable
reproduction, three fix options with a recommendation, and a blast-radius
assessment.

[compartment_mvalue returns NaN](/findings/mvalue-division-by-zero.md) is the
short form — a low-severity defect with no production caller, filed anyway
because it blocks another fix.

# After approval

1. Make the change, and only that change.
2. Add a test that **fails before and passes after**, asserting against an
   independently derived value.
3. Write an ADR: `doc/adr/YYYYMMDD-HHMM-short-title.md`.
4. Regenerate any golden baseline the change moves, deliberately.
5. Update the finding: set `status: fixed`, add the commit, keep the analysis.
   Do not delete it — the record of why the code is the way it is outlives the
   bug.
6. Mirror the ADR into [`decisions/`](/decisions/index.md), update affected
   concepts, append to [`log.md`](/log.md).

# If approval is declined

Keep the finding. Set `status: wontfix` and record the reasoning. A documented
decision not to fix is worth as much as a fix — it stops the next reader
rediscovering it and re-litigating it.
