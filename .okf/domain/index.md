# Domain — decompression theory

The physics and physiology the code models, described independently of this
implementation. Read [the model overview](buhlmann-model.md) first.

* [Bühlmann ZH-L model](buhlmann-model.md) - parallel compartments, the ZH-L12/ZH-L16 variants, and what the model deliberately omits.
* [Inert gas loading — Haldane and Schreiner](inert-gas-loading.md) - the two exponential loading equations and why Schreiner degenerates to Haldane at zero rate.
* [Alveolar inert gas pressure](alveolar-pressure.md) - water vapour, CO₂ and the respiratory quotient.
* [M-values and the decompression ceiling](m-values-and-ceiling.md) - the a/b coefficients, the ceiling inversion, and the two conflicting combined-gas rules in this codebase.
* [No-decompression limit](no-decompression-limit.md) - what the NDL means and the reference values this model produces.
* [Gradient factors](gradient-factors.md) - the GF-low/GF-high conservatism scheme, implemented here but not wired in.
* [Oxygen toxicity and OTU](oxygen-toxicity.md) - pulmonary toxicity, the NOAA unit, and the 0.5 bar threshold.

# Cross-cutting

* [Units and conventions](units-and-conventions.md) - every unit, sign convention and scale in the codebase.
