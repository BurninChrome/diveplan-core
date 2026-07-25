# Findings — the defect register

Every known defect in the repository, in one place. **None has been fixed.** The
[algorithm integrity policy](/decisions/algorithm-integrity-policy.md) requires
a written report and explicit approval before any change to decompression
mathematics; the findings outside that scope were left alone too, to keep the
analysis passes read-only.

Every numeric claim is reproducible. Methods:
[bundle verification](/decisions/2026-07-25-bundle-verification-method.md) and
[model validation against the literature](/decisions/2026-07-25-model-validation.md).

# High severity — wrong or unverified safety output

* [ZH-L16 nitrogen 'a' coefficients do not match the published tables](zh-l16-a-coefficients-nonstandard.md) - 11 of 96 cells differ from published ZH-L16C, 10 less conservative; the tables labelled A and B are not those variants.
* [zh_l12 is unverified — and it is the table the CLI actually runs](zh-l12-unverified.md) - empirical coefficients with no formula or mirrored table to check against. The largest open risk.
* [nodecotime() over-reports the NDL by 1.5–1.7×](nodecotime-overestimates-ndl.md) - the search returns the next untested candidate, and accepts a 1 m ceiling as no-deco. Both errors optimistic.
* [getCeiling() and compartment_mvalue() disagree on trimix by up to 3.7 m](ceiling-vs-mvalue-divergence.md) - two combined-gas rules coexist; the CLI uses the less conservative one. Air output unaffected.
* [zh_l16A and zh_l16B declare 17 compartments but initialise 16](zh-l16ab-phantom-compartment.md) - a zero-filled row gives half-time 0 and NaN tissue pressures. Latent: nothing calls these tables.

# Medium severity — process, consistency, and input generation

* [CI has never run the C test suite](ci-never-runs-c-tests.md) - Travis is dead, and even when it worked it ran only the 33-line Python test. **The precondition for everything else.**
* [The CLI runs ZH-L12, not the ZH-L16C the project designates as primary](dive-uses-zh-l12-not-zh-l16c.md) - the well-tested, literature-validated table is the one nothing loads.
* [gen_dive.py ignores the bottom gas and stalls on zero-duration deco stops](gen-dive-gas-handling.md) - `-g` has almost no effect and `-o …,0` silently disables every later deco gas.
* [parse_dive.py mis-handles gas switches and can crash on a malformed first sample](parse-dive-robustness.md) - only the first DiveMixture is read; blocks use of the 39-dive corpus as a regression fixture.
* [dive.c evaluates the NDL at the previous sample's depth](dive-passes-wrong-pressure-to-ndl.md) - `lastp` where `p` belongs; optimistic during descent.
* [doc/api.md documents OTU bugs that were fixed four months ago](stale-api-doc.md) - tells readers working code is broken, and invites a re-break.

# Low severity

* [Compartments are initialised at a different air composition than the simulation uses](nitrogen-fraction-inconsistency.md) - 0.78084 vs 0.79052, so a diver at the surface slowly on-gasses.
* [compartment_mvalue() returns NaN for an unloaded compartment](mvalue-division-by-zero.md) - no production caller today, but it blocks the ceiling fix.
* [visoutput.py drops the last helium column and mis-renders the NDL](visoutput-he-column.md) - display-only; four lines to fix.
* [The dive binary compiles with no warning flags](dive-built-without-warnings.md) - `-Wall -Wextra` never sees the driver, hiding at least one diagnostic.

# What is validated and correct

Worth stating alongside the defects, because it narrows where to look:
**the mathematics is right.** Haldane and Schreiner reproduce numerical
integration of the perfusion ODE to 6e-14 for both descent and ascent, the
alveolar constants match the physiological references, the M-value inversion
matches Baker's slope-intercept definition, and the helium half-time ratios
match Graham's law. Every defect above sits in the data, the search procedure
around the equations, or the plumbing — not in the equations.
See [the validation record](/decisions/2026-07-25-model-validation.md).

# Suggested order of work

**First, needing no approval:** [fix CI](ci-never-runs-c-tests.md). Every other
change wants a test, and tests are worthless unrun. Then
[turn on warnings for `dive`](dive-built-without-warnings.md), since `-Werror`
in CI depends on it.

**Then, in dependency order:**

1. [`compartment_mvalue` NaN guard](mvalue-division-by-zero.md) — no behavioural change, unblocks (2).
2. [`getCeiling` delegation](ceiling-vs-mvalue-divergence.md) — air output unchanged at printed precision.
3. [`nodecotime` rewrite](nodecotime-overestimates-ndl.md) + [the `lastp` fix](dive-passes-wrong-pressure-to-ndl.md) — same output field, do together.
4. [ZH-L16 `a` coefficients](zh-l16-a-coefficients-nonstandard.md) + [the phantom row](zh-l16ab-phantom-compartment.md) — same rows of the same file, do together. Prerequisite for adopting ZH-L16C.
5. [Verify `zh_l12`](zh-l12-unverified.md) against Bühlmann 1983, **or** switch to the corrected ZH-L16C and retire the question.

**Independently, no approval needed:** [`doc/api.md`](stale-api-doc.md),
[`visoutput.py`](visoutput-he-column.md),
[`gen_dive.py`](gen-dive-gas-handling.md),
[`parse_dive.py`](parse-dive-robustness.md).

# Reporting a new one

Follow [report an algorithm bug](/playbooks/report-an-algorithm-bug.md) — it
carries the template and a worked example.
