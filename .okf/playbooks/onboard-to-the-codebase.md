---
type: Playbook
title: Onboard to the codebase
description: A two-hour path from nothing to productive, with the specific misconceptions this codebase invites and how to avoid them.
tags: [playbook, onboarding, howto]
timestamp: '2026-07-25T09:30:00Z'
---

# Hour one — orient

Read, in order:

1. [Bühlmann ZH-L model](/domain/buhlmann-model.md) — the theory. If you have
   never met a perfusion-limited compartment model, this is the page that makes
   the rest legible.
2. [Units and conventions](/domain/units-and-conventions.md) — bar absolute,
   minutes, fractions. Ten minutes here saves hours later.
3. [Architecture](/components/architecture.md) — the layering and the call
   graph of one step.
4. [stdio format](/interfaces/dive-stdio-format.md) — the contract joining the
   three processes.

Then run one: [run a dive simulation](/playbooks/run-a-dive-simulation.md).

# Hour two — read the code

It is ~525 lines of C. Read it all, bottom-up, in this order:

| Order | File | Why |
|---|---|---|
| 1 | `alveolar.c` | 19 lines. One formula. Start here. |
| 2 | `haldane.c` | The core exponential. |
| 3 | `schreiner.c` | Same equation, sloped input. |
| 4 | `compartment.c` | Where a gas mix becomes two single-gas problems. |
| 5 | `ceiling.c` | The inversion to a safe depth. |
| 6 | `dive.c` | The loop that drives all of it. |
| 7 | `stop.c` | Last. It is the messiest and the most wrong. |

Skip `buhlmann.c` — it is [empty](/components/buhlmann-empty-tu.md). Skip
`getline.c` — it is a [portability shim](/components/getline-shim.md) that
compiles to nothing on your machine.

# Six things that will mislead you

These are the specific traps, in the order you are likely to hit them.

**`stop.c` has nothing to do with decompression stops.** It contains one NDL
search. There is no stop scheduler in this codebase, no runtime generator, and
`STOPINC` is defined but never used. [concept](/components/stop.md)

**`compartment_descend()` handles ascent too.** And flat segments. It is the
only integrator the CLI uses; a negative rate is an ascent and a zero rate
reduces to Haldane. Read the name as "integrate over a linear ramp".
[concept](/components/compartment.md)

**Pressure is absolute.** Surface is 1.0 bar, not 0. A ceiling of 1.0 means *no*
obligation. Ceilings can legitimately be negative.
[concept](/domain/units-and-conventions.md)

**The well-tested table is not the one that runs.** 147 of the suite's 202
checks cover ZH-L16C; `dive.c` uses `zh_l12` exclusively.
[finding](/findings/dive-uses-zh-l12-not-zh-l16c.md)

**Two functions answer "what is the ceiling", differently.**
[`getCeiling()`](/components/ceiling.md) is what runs;
[`compartment_mvalue()`](/components/compartment.md) is the Bühlmann-correct one
and is never called. [finding](/findings/ceiling-vs-mvalue-divergence.md)

**`struct compartment_state` orders helium first**; every function signature and
the output format order nitrogen first. Use designated initialisers.
[concept](/interfaces/c-api.md)

# Before you change anything

Read the [algorithm integrity policy](/decisions/algorithm-integrity-policy.md).
Ten of the thirteen `.c` files in `src/` are protected — everything except
`dive.c`, `getline.c` and `buhlmann.c`. You may not modify a protected file
without a written report and explicit approval, however obvious the fix. The procedure is
[report an algorithm bug](/playbooks/report-an-algorithm-bug.md).

This is not theatre. The
[NDL over-report](/findings/nodecotime-overestimates-ndl.md) is a two-line fix
that changes safety output by 40% — exactly the kind of change that needs a
second pair of eyes.

# Good first tasks

None of these touch protected files, so none needs approval.

1. **[Fix CI.](/findings/ci-never-runs-c-tests.md)** The C test suite has never
   run automatically. Highest value in the repository, and it is one YAML file.
2. **[Fix `doc/api.md`.](/findings/stale-api-doc.md)** It documents two bugs
   that were fixed four months ago.
3. **[Fix `visoutput.py`.](/findings/visoutput-he-column.md)** A dropped column
   and a minutes-to-seconds conversion, four lines.
4. **Add the golden-file regression** over the 39 real dives in `test/xml/`.
   See [test suite](/tooling/test-suite.md) and
   [`parse_dive.py`](/tooling/parse-dive.md). This is the most valuable *new*
   thing you could build, and it needs no reference implementation — just record
   today's output and diff.
5. **Fix the [CFLAGS asymmetry](/tooling/build-system.md)** so `dive` compiles
   with warnings enabled. It will immediately surface an unused variable.

Once you want to go further, the
[integrity policy](/decisions/algorithm-integrity-policy.md) carries a suggested
ordering for the algorithm work.

# Keep this bundle honest

If you learn something durable — a constraint, a gotcha, why something is the way
it is — write it back. That is the OKF maintain workflow: update the affected
concepts, refresh the relevant `index.md`, append to [`log.md`](/log.md). This
bundle decays exactly as fast as
[`doc/api.md`](/decisions/2026-03-09-add-api-documentation.md) did if nobody
does.
