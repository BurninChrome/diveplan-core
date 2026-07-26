---
type: Decision Record
title: 'ADR: Modernize build system and Python 3 compatibility'
description: Brought the C build to c99 with warnings enabled and ported all Python to 3, incidentally fixing three integer-division bugs that had silently zeroed OTU output.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/doc/adr/20260309-modernize-build-and-python3.md
tags: [adr, build, python3, otu, historical]
timestamp: '2026-07-25T09:30:00Z'
---

> Mirrors `doc/adr/20260309-modernize-build-and-python3.md`. Commit `cb25705`.

# Decision

Compile cleanly on modern GCC with `-std=c99 -Wall -Wextra`, and update all
Python scripts and tooling to Python 3 with current library versions.

# Context

The tree had accumulated a decade of rot:

- Python 2 imports (`StringIO`), `/usr/bin/python` shebangs, mixed tabs and
  spaces causing `TabError` under Python 3.
- `tools/requirements.txt` pinned matplotlib 1.5 and numpy 1.10, neither of
  which installs on a current Python.
- `configure.ac` used deprecated Autoconf macros.
- `buhlmann.h` used `float` literals for `double` constants.
- **`otu.c` contained three integer-division bugs** — `-5/6` → `0`,
  `3/11` → `0`, `11/6` → `1` — which silently zeroed the OTU calculations.
- A dangling `if` in `stop.c` produced a GCC warning and dead code.

# Consequences

- Python files require Python 3 (`env python3` shebangs).
- `tools/requirements.txt` moved to `matplotlib>=3.5.0`, `numpy>=1.21.0`,
  unpinned.
- The library builds with `-std=c99 -D_GNU_SOURCE -Wall -Wextra -O2` —
  `_GNU_SOURCE` is needed for `M_LN2`.
- **OTU calculations produce correct floating-point results.**
- `configure.ac` uses `AC_CONFIG_MACRO_DIRS` and `AC_CONFIG_FILES`.
- A `h2`/`he` variable-name bug in [`gen_dive.py`](/tooling/gen-dive.md)'s
  ascent loop was fixed, so the deco helium fraction is now actually applied.

# What this ADR is really about

The framing is housekeeping; the substance is that three silent-wrong-answer
bugs in safety-critical arithmetic were found by turning on compiler warnings
and reading the code. `pow(x, 0)` is 1, so
[`otu_const()`](/components/otu.md) returned `time` for every input, and
`otu_descend()` returned `0.0` for every input. No crash, no warning, no test
failure — the functions had no tests at all at the time.

That is the strongest available argument for the
[algorithm integrity policy](/decisions/algorithm-integrity-policy.md)'s
insistence on review, and for
[the test suite](/tooling/test-suite.md)'s later practice of asserting exact
computed values rather than signs.

# Follow-ups still open

- The `f` literal suffixes were removed from `buhlmann.h`'s macros but **not**
  from the constant tables in `zh-l12.c` and `zh-l16.c`, which still store
  `float`-parsed values in `double` fields. See
  [the tables](/components/tissue-constant-tables.md).
- The `-Wall -Wextra` flags reach the library and the tests but **not `dive`**,
  which has no `CFLAGS` of its own. See [build system](/tooling/build-system.md).
- `.travis.yml` was not updated and still invokes `python`. See
  [the CI finding](/findings/ci-never-runs-c-tests.md).
- `otu_descend()` was fixed again the following day for a NaN — see
  [the next ADR](/decisions/2026-03-10-fix-otu-descend-nan.md).
