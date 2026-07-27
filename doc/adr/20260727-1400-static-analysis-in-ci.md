# ADR: Add static and dynamic analysis to CI

Date: 2026-07-27 14:00

## Decision

Add an `analysis` CI job running cppcheck, `gcc -fanalyzer`, and UBSan+ASan
over the test suites. All three are hard gates. A stricter warning set runs
alongside as **advisory** until the warnings it reports are cleared.

## Context

Power of Ten rule 10 requires compiling with all warnings enabled and using
static analysers. The project already builds with `-Wall -Wextra -Werror` in
CI; this is the analyser half.

Each tool was run against the codebase before being adopted, to know what it
would find and whether the gate would be green:

| Tool | Findings today | Verdict |
|---|---:|---|
| cppcheck (warning/performance/portability/unusedFunction) | 0 | gate |
| `gcc -fanalyzer` | 0 | gate |
| UBSan + ASan over `make check` and the pipeline | 0 | gate |
| UBSan over deliberately exercised edge paths | 4 | see below |
| Strict warning set | 7 | advisory |

## What the analysers found that nothing else did

Running UBSan against the documented edge cases — rather than the ordinary test
suite, which never reaches them — located three known defects at their exact
source lines:

```
src/alveolar.c:13     division by zero    ventilation() with rq = 0
src/compartment.c:12  division by zero    compartment_mvalue() on an empty compartment
src/compartment.c:14  division by zero
src/haldane.c:15      division by zero    half-time 0, the phantom ZH-L16A/B row
```

These are already in `.okf/findings/`, but no other tool in the project points
at them. The ordinary suites stay clean because they never exercise those
paths, so the gate is green while the capability is real: any *new* code
reaching a division by zero fails the build.

cppcheck and the strict warning set independently rediscovered further recorded
items — the missing prototypes on `otu_const`/`otu_descend`, the four
`compartment_state *` parameters that could be `const`, the `1.0f` literals in
`double` expressions. Useful corroboration that the findings register reflects
real defects rather than opinion.

## Why the strict set is advisory

Its seven warnings live in `haldane.c`, `schreiner.c`, `otu.c`,
`gradientfactor.c` and `dive.c` — four of them covered by the algorithm
integrity policy. Clearing them needs approval and belongs to the BARR-C /
Power of Ten pass, not to the commit that introduces the job. Making it a gate
now would mean either a red build or rushing protected-file edits to go green.

The three `-Wfloat-equal` cases in particular need judgement, not a blanket
fix: two are deliberate zero-guards and one is `if (dt)` in the driver.

Promote to a gate once that pass lands.

## Consequences

- Analysis runs on every push and pull request, in parallel with the build.
- cppcheck analyses the library **and** the test suites together. That is not
  only more coverage: it removes `unusedFunction` false positives, since
  several public API functions have no in-repo caller — which is what a library
  looks like — but are called by the tests.
- `-fno-sanitize-recover=all` makes any sanitizer report fatal rather than a
  log line nobody reads.
- `gcc -fanalyzer` finds nothing today and probably will for a while: the
  library never allocates and uses only single-level pointers. It is cheap
  insurance against that changing.
