# ADR: Modernize build system and Python 3 compatibility

Date: 2026-03-09

## Decision
Update the codebase to compile cleanly on modern GCC with `-std=c99 -Wall -Wextra`, and update all Python scripts and tooling to work with Python 3 and current library versions.

## Context
The original code used Python 2-only imports (`StringIO` module), Python 2 shebangs (`/usr/bin/python`), mixed tabs/spaces (TabError in Python 3), and pinned ancient library versions (matplotlib 1.5, numpy 1.10) that no longer install on modern Python. On the C side, the `configure.ac` used deprecated Autoconf macros, `buhlmann.h` used `float` literals for `double` constants, and `otu.c` had integer division bugs (`-5/6 == 0`, `3/11 == 0`, `11/6 == 1`) that silently zeroed out OTU calculations. A dangling `if` in `stop.c` caused a GCC warning and logical dead code.

## Consequences
- Python files now require Python 3 (shebangs updated to `env python3`).
- `tools/requirements.txt` now specifies `matplotlib>=3.5.0` and `numpy>=1.21.0` without strict pinning.
- C library is compiled with `-std=c99 -D_GNU_SOURCE -Wall -Wextra -O2` (GNU source needed for `M_LN2`).
- OTU calculations now produce correct floating-point results.
- `configure.ac` uses modern Autoconf idioms (`AC_CONFIG_MACRO_DIRS`, `AC_CONFIG_FILES`).
- The `h2`/`he` variable bug in `gen_dive.py` ascent loop is fixed (deco gas helium fraction now actually applied).
