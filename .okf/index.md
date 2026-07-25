---
okf_version: '0.1'
---

# diveplan-core

Knowledge bundle for `diveplan-core` / `libbuhlmann` — a C implementation of the
Bühlmann ZH-L decompression algorithm.

* [Getting started](getting-started.md) - what the project is, how this bundle is organised, and where to begin.
* [Update log](log.md) - chronological history of this bundle.

# Read first

* [Bühlmann ZH-L model](domain/buhlmann-model.md) - the decompression theory in one page.
* [Units and conventions](domain/units-and-conventions.md) - bar absolute, minutes, fractions; the most common source of integration errors.
* [Architecture](components/architecture.md) - module map, layering and the call graph of one simulation step.
* [Findings](findings/index.md) - **the complete defect register: 15 findings. Read before trusting any output.**
* [Model validation](decisions/2026-07-25-model-validation.md) - the equations checked against the decompression literature and numerical integration.

# Directories

* [domain/](domain/index.md) - the decompression theory being modelled, independent of this code.
* [components/](components/index.md) - one concept per source module.
* [interfaces/](interfaces/index.md) - the C API and the stdin/stdout data contract.
* [tooling/](tooling/index.md) - build system, profile generators, visualiser, test suite.
* [decisions/](decisions/index.md) - architecture decision records and the algorithm integrity policy.
* [findings/](findings/index.md) - verified defects and divergences, with reproductions.
* [playbooks/](playbooks/index.md) - task recipes.

# Safety notice

Two of the model's outputs are known to be wrong in the optimistic direction:
the no-decompression limit is
[1.5–1.7× too long](findings/nodecotime-overestimates-ndl.md), and the trimix
ceiling reads [up to 3.7 m shallow](findings/ceiling-vs-mvalue-divergence.md).
**This software is not fit for planning a real dive.** Every defect is reported
under the [algorithm integrity policy](decisions/algorithm-integrity-policy.md)
and none has been fixed.
