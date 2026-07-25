---
type: Reference
title: Getting started — diveplan-core
description: Orientation map for the diveplan-core knowledge bundle — what the project is, how the bundle is organised, and where to start reading.
tags: [getting-started, orientation, buhlmann]
timestamp: '2026-07-25T09:30:00Z'
---

# What diveplan-core is

`diveplan-core` (built as `libbuhlmann`) is a small C library and companion CLI
that implement the **Bühlmann ZH-L decompression algorithm**. It tracks the
partial pressures of two inert gases — nitrogen and helium — across a bank of
parallel tissue compartments as ambient pressure changes, and from that derives
a **decompression ceiling** and a **no-decompression limit**.

The unit of work is one dive profile: a sequence of `(time, pressure, fO2, fHe)`
samples read on stdin, one line of model state written on stdout per sample.
See [the stdio contract](/interfaces/dive-stdio-format.md).

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

| Aspect | State |
|---|---|
| Model equations | **Validated against the literature and numerical integration — correct to 6e-14.** See [the validation record](/decisions/2026-07-25-model-validation.md). |
| Constant tables | **ZH-L16 nitrogen `a` values do not match the published tables** ([finding](/findings/zh-l16-a-coefficients-nonstandard.md)); ZH-L16A/B are also under-initialised ([finding](/findings/zh-l16ab-phantom-compartment.md)). ZH-L12 is unverified. |
| Table used by the CLI | ZH-L12 (1983 vintage), *not* ZH-L16C ([finding](/findings/dive-uses-zh-l12-not-zh-l16c.md)). |
| Ceiling | Computed per gas independently, which diverges from Bühlmann on trimix ([finding](/findings/ceiling-vs-mvalue-divergence.md)). |
| No-decompression limit | **Over-reports by 1.5–1.7×** ([finding](/findings/nodecotime-overestimates-ndl.md)). |
| Gradient factors | Implemented and unit-tested, but not wired into the model ([concept](/components/gradientfactor.md)). |
| Oxygen toxicity | Implemented and unit-tested, but not wired into the model ([concept](/components/otu.md)). |
| Decompression stop scheduling | **Not implemented** despite the `stop.c` filename ([concept](/components/stop.md)). |

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
port to compute it. No C toolchain was available in the authoring environment,
so no claim here is sourced from running the compiled `dive` binary; numeric
claims are labelled with how they were derived. See
[verification method](/decisions/2026-07-25-bundle-verification-method.md).
