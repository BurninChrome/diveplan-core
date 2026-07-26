# Components — the C source

One concept per module. Start with [the architecture overview](architecture.md)
for the layering and the call graph.

* [Architecture](architecture.md) - module map, dependency layering, call graph of one step, and design observations.

# Layer 1 — equations

* [alveolar.c — ventilation()](alveolar.md) - ambient pressure and gas fraction to alveolar inert gas pressure.
* [haldane.c](haldane.md) - constant-pressure exponential gas loading.
* [schreiner.c](schreiner.md) - linear-ramp gas loading; the only integrator the CLI uses.

# Layer 2 — per compartment

* [compartment.c](compartment.md) - dispatches a gas mix into two single-gas problems; also holds the weighted M-value.

# Layer 3 — derived quantities

* [ceiling.c — getCeiling()](ceiling.md) - the per-compartment ceiling the CLI reports.
* [stop.c — nodecotime()](stop.md) - the NDL search; not the stop scheduler its filename suggests.
* [gradientfactor.c](gradientfactor.md) - GF line arithmetic; complete, tested, unconnected.
* [otu.c](otu.md) - oxygen toxicity units; correct since March 2026, undeclared and unused.

# Layer 0 — data

* [Tissue constant tables](tissue-constant-tables.md) - ZH-L12 and ZH-L16A/B/C with full values, invariants, and which are safe to use.

# Layer 4 — driver

* [src/dive — the simulation driver](dive-cli.md) - the 95-line main loop.

# Incidental

* [buhlmann.c — the empty translation unit](buhlmann-empty-tu.md) - contains only includes, yet anchors the autoconf configuration.
* [getline.c — portability shim](getline-shim.md) - fallback getline() for platforms lacking POSIX 2008.
