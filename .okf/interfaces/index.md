# Interfaces — the contracts

Two boundaries: the linker boundary (the installed header) and the process
boundary (the stdio format).

* [C API — buhlmann.h](c-api.md) - two structs, ten declared functions, eight macros, four tables; plus what is deliberately or accidentally missing.
* [dive stdin/stdout format](dive-stdio-format.md) - the 4-field input and 36-field output contract joining the generators, the simulator and the visualiser.

# See also

* [Units and conventions](/domain/units-and-conventions.md) - what the numbers on both sides of these boundaries mean.
* [Architecture](/components/architecture.md) - why there is no "simulate a dive" entry point.
