# Tooling — build, generate, visualise, test

* [Build system — autotools](build-system.md) - bootstrap, configure, make; the compiler-flag asymmetry, and dead Travis CI.
* [Test suite — coverage and gaps](test-suite.md) - what the 202 C runtime checks verify, and the three structural gaps that let the known defects survive.

# Profile producers

* [test/gen_dive.py](gen-dive.md) - synthesises square profiles with gas and deco options; several flags do not do what they say.
* [test/parse_dive.py](parse-dive.md) - imports Subsurface-style XML; the gateway to the 39 real dive logs in `test/xml/`.

# Output consumer

* [tools/visoutput.py](visoutput.md) - matplotlib depth profile with ceiling overlay and a scrubbable per-compartment histogram.

# See also

* [stdio format](/interfaces/dive-stdio-format.md) - the contract all three tools depend on.
* [CI never runs the C tests](/findings/ci-never-runs-c-tests.md) - the highest-priority tooling defect.
