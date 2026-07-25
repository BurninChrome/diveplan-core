---
type: Finding
title: CI has never run the C test suite
description: .travis.yml targets a service shut down in 2021, invokes python2-era commands, and even when it worked it ran only the 33-line Python test — never make check.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/.travis.yml
tags: [finding, ci, testing, process]
severity: medium
status: reported-awaiting-approval
timestamp: '2026-07-25T09:30:00Z'
---

# The configuration

```yaml
language: c
compiler: gcc
install: ./bootstrap.sh
script:
  - ./configure && make
  - python test/test_all_of_the_units.py -v
```

Three problems, compounding.

**travis-ci.org shut down in 2021.** Nothing has run here in years. The
[README](https://github.com/BurninChrome/diveplan-core/blob/main/README.md)
still renders a build badge pointing at `secure.travis-ci.org`, which now
reports nothing meaningful — a green-looking artefact that suggests verification
where there is none.

**`python` is the wrong interpreter.** The scripts were ported to Python 3 in
March 2026 with `env python3` shebangs
([ADR](/decisions/2026-03-09-modernize-build-and-python3.md)), but this
invocation was not updated. On any modern image `python` is either absent or
Python 3 by coincidence.

**`make check` is never invoked.** This is the substantive one. The script runs
`make`, which builds the library and `dive`, then runs the Python test. The
746-line `test_buhlmann.c`, with its 78 assertion call sites and 202 runtime
checks — registered as `TESTS` in `test/Makefile.am`, and the only real
verification the project has — has **never executed in CI**.

The only automated check that ever ran was
`test_all_of_the_units.py`: two tests, one of which asserts that
`'hello world'.split()` returns two words.

# Why this is the highest-priority item in the bundle

Every other finding here is a bug that CI could plausibly have caught, or could
catch in future:

- [The phantom compartment](/findings/zh-l16ab-phantom-compartment.md) — a
  three-line assertion would fail today.
- [The NDL over-report](/findings/nodecotime-overestimates-ndl.md) — catchable
  with a reference-value test.
- [The ceiling divergence](/findings/ceiling-vs-mvalue-divergence.md) — catchable
  with a cross-check between the two functions.

None of those tests are worth writing while nothing runs them. Fixing CI is the
precondition for every other quality improvement, and unlike the algorithm
findings it needs no approval under the
[integrity policy](/decisions/algorithm-integrity-policy.md) — it touches no
model code.

# Recommendation

Replace `.travis.yml` with a GitHub Actions workflow. The repository is already
on GitHub (`BurninChrome/diveplan-core`), so no new service is involved.

```yaml
name: build-and-test
on: [push, pull_request]

jobs:
  c:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install autotools
        run: sudo apt-get update && sudo apt-get install -y autoconf automake libtool
      - run: ./bootstrap.sh
      - run: ./configure
      - run: make
      - run: make check            # ← the missing step
      - name: Show test log on failure
        if: failure()
        run: cat test/test-suite.log || true

  python:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-python@v5
        with: { python-version: '3.x' }
      - run: python3 -m unittest discover -s test -v
```

Then update the README badge, and delete `.travis.yml` so nobody mistakes it for
live configuration.

Two follow-ons once it is green:

- Build with `-Werror` in CI only, which would surface the unused `stop`
  variable in [`dive.c`](/components/dive-cli.md) — note that requires fixing
  the [CFLAGS asymmetry](/tooling/build-system.md) first, since `dive` currently
  compiles with no warning flags at all.
- Add the golden-file regression over the 39 real dives in `test/xml/`. See
  [test suite](/tooling/test-suite.md) and
  [`parse_dive.py`](/tooling/parse-dive.md).
