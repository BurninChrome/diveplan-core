---
type: Reference
title: Getting started — diveplan-core
description: Orientation map for the diveplan-core knowledge bundle — what the project is, how the bundle is organised, and where to start reading.
tags: [getting-started, orientation, buhlmann]
timestamp: '2026-07-25T09:30:00Z'
---

# What diveplan-core is

`diveplan-core` builds **`libbuhlmann`, a C library** implementing the
**Bühlmann ZH-L decompression algorithm and gradient factors**. It tracks the
partial pressures of two inert gases — nitrogen and helium — across a bank of
parallel tissue compartments as ambient pressure changes, and from that derives
a **decompression ceiling** and a **no-decompression limit**.

## Scope

**The library is the product.** `src/dive` is a **demonstration harness**, not a
deliverable — it is `noinst_PROGRAMS`, so `make install` ships the library and
`buhlmann.h` and nothing else. A dive planner built on this library is a
separate project.

That boundary matters when reading the rest of this bundle, because it decides
what counts as a defect:

- A wrong number in the model is a **library bug**.
- A hand-rolled loop in `dive.c` is usually a **missing library primitive** —
  the driver had nothing to call. See
  [the scope table](/domain/ascent-and-stop-scheduling.md#scope-what-belongs-in-the-library).
- The scheduling layer that turns ceilings into a stop plan is **out of scope
  here**, but the library must expose the primitives a planner needs. It is
  specified in full in
  [ascent and stop scheduling](/domain/ascent-and-stop-scheduling.md).

The demo's unit of work is one dive profile: `(time, pressure, fO2, fHe)`
samples on stdin, one line of model state per sample on stdout. See
[the stdio contract](/interfaces/dive-stdio-format.md) — that format is the
demo's, not the library's interface.

This is **safety-critical arithmetic**. A ceiling that reads too shallow, or a
no-decompression limit that reads too long, is the kind of error that gives a
diver decompression sickness. The repository encodes that seriousness as a
standing rule — see the [algorithm integrity policy](/decisions/algorithm-integrity-policy.md)
— and this bundle follows it: every defect below is written up, none is fixed
in passing.

# Reading order

If you are new, read in this order:

1. [Bühlmann ZH-L model](/domain/buhlmann-model.md) — the theory in one page.
2. [Units and conventions](/domain/units-and-conventions.md) — bar, minutes,
   fractions. Getting this wrong is the single most common integration error.
3. [Architecture](/components/architecture.md) — how the ~525 lines of C fit
   together.
4. [Findings](/findings/index.md) — the complete defect register, 15 entries. **Read before trusting any output.**

# Bundle layout

| Directory | Holds |
|---|---|
| [`domain/`](/domain/index.md) | The decompression theory being modelled, independent of this code. |
| [`components/`](/components/index.md) | One concept per source module: what it computes and how. |
| [`interfaces/`](/interfaces/index.md) | The contracts — C API surface and the stdin/stdout format. |
| [`tooling/`](/tooling/index.md) | Build system, profile generators, visualiser, test suite. |
| [`decisions/`](/decisions/index.md) | Architecture decision records and standing policy. |
| [`findings/`](/findings/index.md) | Verified defects and divergences, with reproductions. |
| [`playbooks/`](/playbooks/index.md) | Task recipes: run a dive, add a table, report a bug. |

# Status at a glance

Library first, then the demo.

| Aspect | Scope | State |
|---|---|---|
| Model equations | library | **Validated against the literature and numerical integration — correct to 6e-14.** See [the validation record](/decisions/2026-07-25-model-validation.md). |
| Constant tables | library | **ZH-L16 nitrogen `a` values do not match the published tables** ([finding](/findings/zh-l16-a-coefficients-nonstandard.md)); ZH-L16A/B are under-initialised ([finding](/findings/zh-l16ab-phantom-compartment.md)); ZH-L12 is [unverified](/findings/zh-l12-unverified.md). |
| Ceiling | library | Computed per gas independently, which diverges from Bühlmann on trimix ([finding](/findings/ceiling-vs-mvalue-divergence.md)). |
| **GF-adjusted ceiling** | library | ❌ **absent** — the central gap for a library whose remit includes gradient factors. |
| No-decompression limit | library | **Over-reports by 1.5–1.7×** ([finding](/findings/nodecotime-overestimates-ndl.md)). |
| Gradient factors | library | Line arithmetic implemented and unit-tested, [never called](/components/gradientfactor.md). |
| Oxygen toxicity | library | Implemented and unit-tested, [not declared in the header](/components/otu.md). |
| Public API for state setup, table selection, aggregation | library | ❌ absent — every caller hand-rolls it, and the demo got two of them wrong. |
| Stop scheduling | **planner** | Out of scope here; [specified](/domain/ascent-and-stop-scheduling.md) so the library can be judged against it. `stop.c` is misleadingly named and [holds only an NDL search](/components/stop.md). |
| Table the demo loads | demo | ZH-L12, *not* ZH-L16C ([finding](/findings/dive-uses-zh-l12-not-zh-l16c.md)) — a symptom of the missing selection API. |

# Provenance of the project

Written 2015–2017 under the AquaBSD banner (`LICENSE` is ISC, © 2015–2016
AquaBSD), originally as `libbuhlmann`. Five contributors across several git
identities, with the bulk of the work by Julien Stuyck and Julian Pidancet; the
French-language comments in
[`stop.c`](/components/stop.md) and the French M-value paper in `doc/` date from
that period. 78 of the 84 commits land in 2015–2017, after which the repository
was dormant until a modernisation pass in March 2026 — see
[the ADR](/decisions/2026-03-09-modernize-build-and-python3.md). The `AUTHORS`
file is empty.

This matters for reading the code: most of it is ten years old and was never
finished. The dead scaffolding in `dive.c` and `stop.c`, the unused `STOPINC`
and `USNAVY_RQ` macros, and the absent stop scheduler are all remnants of that
original push rather than deliberate omissions.

# Provenance of this bundle

Written 2026-07-25 by reading every source file in the repository and, where a
claim needed a number, reproducing the exact C formulas in a throwaway Python
port to compute it — no C toolchain was available at authoring time. The bundle
was then independently reviewed against the source, validated against the
decompression literature, and finally **re-checked against the compiled
library** once a toolchain was installed.

The build is clean and `make check` passes **202/202**. Every claim about the C
library was reproduced against the real binary, digit for digit. A fourth pass —
an independent merge review — then caught one genuine error the first three
missed: the nitrogen-fraction magnitudes, which had been derived from
`gen_dive.py`'s internal constant rather than from what it actually prints. See
[verification method](/decisions/2026-07-25-bundle-verification-method.md) for
the confirmation table and the corrections.
