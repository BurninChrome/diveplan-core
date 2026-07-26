---
type: Policy
title: Algorithm integrity policy
description: The standing rule that decompression mathematics must not be modified without explicit human approval, and the report format that rule requires.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/CLAUDE.md
tags: [policy, safety, process, governance]
timestamp: '2026-07-25T09:30:00Z'
---

# The rule

From `CLAUDE.md`, verbatim in force:

> **The decompression algorithms must never be modified without explicit user
> approval.**

The rationale given: the Bühlmann equations and the tissue constant tables
implement safety-critical mathematics, and incorrect changes produce dive plans
that cause decompression sickness. Unlike most software defects, the failure
mode is a plausible-looking number that injures someone hours later.

# Files under the policy

`haldane.c`, `schreiner.c`, `compartment.c`, `ceiling.c`, `stop.c`,
`alveolar.c`, `gradientfactor.c`, `otu.c`, `zh-l12.c`, `zh-l16.c`, and all
tissue constant tables.

Concept pages: [haldane](/components/haldane.md),
[schreiner](/components/schreiner.md), [compartment](/components/compartment.md),
[ceiling](/components/ceiling.md), [stop](/components/stop.md),
[alveolar](/components/alveolar.md),
[gradientfactor](/components/gradientfactor.md), [otu](/components/otu.md),
[tables](/components/tissue-constant-tables.md).

# Files not under the policy

`dive.c`, `getline.c`, `buhlmann.h`, the build system, all Python tooling, all
documentation, all tests.

The boundary deserves care rather than literal reading. `dive.c` is not listed,
but it chooses which table to load, passes `lastp` where `p` belongs, and owns
the `fmax`/`fmin` aggregation — decisions with the same consequences as the
equations themselves. Treat driver changes that alter model output as if they
were covered. Conversely, `otu.c` *is* listed but is not wired into anything, so
changes there cannot affect any current output.

# Required procedure

On finding a suspected defect:

1. **Do not change the code.**
2. Write a report containing:
   - which function and file
   - the exact erroneous behaviour, with input/output examples
   - the root cause
   - the proposed fix and its scope
   - the impact on other functions and outputs
3. Present it and **wait for explicit confirmation**.

# How this bundle complies

Fifteen defects have been found across three passes — authoring, independent
review, and
[validation against the literature](/decisions/2026-07-25-model-validation.md).
**None was fixed.** Each became a report in
[`findings/`](/findings/index.md) carrying the five required elements plus a
reproduction:

| Finding | Severity | Policy-covered file? |
|---|---|---|
| [ZH-L16 nitrogen `a` coefficients are non-standard](/findings/zh-l16-a-coefficients-nonstandard.md) | high | yes — `zh-l16.c` |
| [`zh_l12` is unverified](/findings/zh-l12-unverified.md) | high | yes — `zh-l12.c` |
| [nodecotime over-reports NDL by 1.5–1.7×](/findings/nodecotime-overestimates-ndl.md) | high | yes — `stop.c` |
| [getCeiling vs compartment_mvalue divergence](/findings/ceiling-vs-mvalue-divergence.md) | high | yes — `ceiling.c` |
| [ZH-L16A/B phantom compartment](/findings/zh-l16ab-phantom-compartment.md) | high | yes — `zh-l16.c` |
| [CI never runs the C tests](/findings/ci-never-runs-c-tests.md) | medium | no |
| [CLI runs ZH-L12, not ZH-L16C](/findings/dive-uses-zh-l12-not-zh-l16c.md) | medium | boundary — `dive.c` |
| [gen_dive.py gas handling](/findings/gen-dive-gas-handling.md) | medium | no |
| [parse_dive.py robustness](/findings/parse-dive-robustness.md) | medium | no |
| [dive.c passes the wrong pressure to the NDL](/findings/dive-passes-wrong-pressure-to-ndl.md) | medium | boundary — `dive.c` |
| [doc/api.md documents fixed bugs](/findings/stale-api-doc.md) | medium | no |
| [Nitrogen fraction inconsistency](/findings/nitrogen-fraction-inconsistency.md) | low | boundary — `dive.c` |
| [compartment_mvalue NaN on zero state](/findings/mvalue-division-by-zero.md) | low | yes — `compartment.c` |
| [visoutput.py column and NDL bugs](/findings/visoutput-he-column.md) | low | no |
| [`dive` compiles with no warning flags](/findings/dive-built-without-warnings.md) | low | no |

The six unrestricted ones — CI, `doc/api.md`, `visoutput.py`, `gen_dive.py`,
`parse_dive.py` and the build flags — were left unfixed as well, to keep the
analysis passes read-only. They can be actioned without approval.

Note the three `dive.c` findings marked *boundary*. That file is not on the
protected list, but each of those changes alters model output, which is the test
that actually matters. Treat them as covered.

# Suggested ordering, if approval is given

**First, and needing no approval:**
[fix CI](/findings/ci-never-runs-c-tests.md), then
[turn on warnings for `dive`](/findings/dive-built-without-warnings.md). Every
other change wants a test, and tests are worthless unrun.

**Then, in dependency order:**

1. [`compartment_mvalue` NaN guard](/findings/mvalue-division-by-zero.md) — no
   behavioural change, unblocks (2).
2. [`getCeiling` delegation](/findings/ceiling-vs-mvalue-divergence.md) — air
   output unchanged at printed precision, trimix becomes correct.
3. [`nodecotime` rewrite](/findings/nodecotime-overestimates-ndl.md) together
   with [the `lastp` fix](/findings/dive-passes-wrong-pressure-to-ndl.md) — same
   output field, so land them as one change. Largest output shift; do it after
   (2) so the two ceiling-related movements are not entangled.
4. [ZH-L16 `a` coefficients](/findings/zh-l16-a-coefficients-nonstandard.md)
   together with
   [the phantom row](/findings/zh-l16ab-phantom-compartment.md) — same rows of
   the same file, no current output change. Prerequisite for adopting ZH-L16C.
5. [Verify `zh_l12`](/findings/zh-l12-unverified.md) against Bühlmann 1983, or
   switch the CLI to the corrected ZH-L16C and retire the question.
6. [Nitrogen fraction consistency](/findings/nitrogen-fraction-inconsistency.md)
   — touches every output line by ~0.009 bar, so do it last and regenerate
   baselines once.

Each wants its own ADR under `doc/adr/`, per the repository's convention, and a
regenerated golden baseline where output moves.

# Observation on the policy itself

It is working. The two OTU bugs it did not prevent — see
[the](/decisions/2026-03-09-modernize-build-and-python3.md)
[ADRs](/decisions/2026-03-10-fix-otu-descend-nan.md) — were both caught by
review and both produced an ADR and hardened tests. The gap the policy does not
close is *detection*: it governs what happens after a defect is found, and says
nothing about finding them. Three of the four high/medium algorithm findings
here were invisible to the existing tests because those tests assert signs and
orderings rather than values. See [test suite](/tooling/test-suite.md).

A natural complement: require that any fix to a policy-covered file ship with a
test that compares against an independently derived value, not against the
implementation's own formula restated.

# Related

- [Repository ADRs](/decisions/index.md)
- [How to report an algorithm bug](/playbooks/report-an-algorithm-bug.md)
