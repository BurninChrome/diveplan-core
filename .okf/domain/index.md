# Domain — decompression theory

The physics and physiology the code models, described independently of this
implementation. Read [the model overview](buhlmann-model.md) first.

Pages typed **Specification** are *normative*: they describe what a correct
implementation must do, and may cover ground this fork does not implement.
Pages typed **Domain Model** describe theory the code does implement. When
reimplementing, work from the specifications and treat
[`components/`](/components/index.md) as a description of *this* fork, bugs
included.

# Specifications (normative)

* [Ascent and decompression stop scheduling](ascent-and-stop-scheduling.md) - how per-compartment ceilings become an actual stop schedule: segmentation, the ascent permission test, stop durations, gradient-factor integration and runtime accounting. **Not implemented in this fork.**

* [Bühlmann ZH-L model](buhlmann-model.md) - parallel compartments, the ZH-L12/ZH-L16 variants, and what the model deliberately omits.
* [Inert gas loading — Haldane and Schreiner](inert-gas-loading.md) - the two exponential loading equations and why Schreiner degenerates to Haldane at zero rate.
* [Alveolar inert gas pressure](alveolar-pressure.md) - water vapour, CO₂ and the respiratory quotient.
* [M-values and the decompression ceiling](m-values-and-ceiling.md) - the a/b coefficients, the ceiling inversion, and the two conflicting combined-gas rules in this codebase.
* [No-decompression limit](no-decompression-limit.md) - what the NDL means and the reference values this model produces.
* [Gradient factors](gradient-factors.md) - the GF-low/GF-high conservatism scheme, implemented here but not wired in. Its role in an ascent is specified in [stop scheduling](ascent-and-stop-scheduling.md).
* [Oxygen toxicity and OTU](oxygen-toxicity.md) - pulmonary toxicity, the NOAA unit, and the 0.5 bar threshold.

# Cross-cutting

* [Units and conventions](units-and-conventions.md) - every unit, sign convention and scale in the codebase.
