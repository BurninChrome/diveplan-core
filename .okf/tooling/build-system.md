---
type: Build
title: Build system — autotools
description: How to bootstrap, configure and build; what each Makefile.am declares; and the compiler-flag asymmetry between the library and the CLI.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/configure.ac
tags: [build, autotools, automake, ci]
timestamp: '2026-07-25T09:30:00Z'
---

# Commands

```bash
./bootstrap.sh          # first time only — mkdir -p m4 && autoreconf -i
./configure
make
make check              # runs the C unit tests
```

Requires autoconf, automake, libtool and a C99 compiler. `bootstrap.sh` is two
lines; the generated `configure`, `Makefile.in` and friends are not committed,
so a fresh clone cannot be built without autotools installed.

# What configure checks

```
AC_CONFIG_SRCDIR([src/buhlmann.c])   # sanity anchor — see /components/buhlmann-empty-tu.md
AC_CONFIG_HEADERS([config.h])
AC_PROG_CC
AC_CHECK_LIB([m], [exp])             # libm
AC_CHECK_HEADERS([math.h])
AC_CHECK_FUNCS([getline])            # → HAVE_GETLINE, gates /components/getline-shim.md
```

Modernised in March 2026 — see
[the ADR](/decisions/2026-03-09-modernize-build-and-python3.md).

# Targets

| Target | Where | Kind |
|---|---|---|
| `libbuhlmann.la` | `src/` | `lib_LTLIBRARIES` — installed |
| `buhlmann.h` | `src/` | `include_HEADERS` — installed |
| `dive` | `src/` | `noinst_PROGRAMS` — **not** installed, links statically |
| `test_buhlmann` | `test/` | `check_PROGRAMS`, registered as `TESTS` |

`libbuhlmann_la_SOURCES` lists eleven `.c` files. `dive_SOURCES` is
`dive.c getline.c` — the shim belongs to the CLI, not the library.

# The compiler-flag asymmetry

Three different flag sets are in play:

```makefile
# src/Makefile.am
libbuhlmann_la_CFLAGS = -W -Wall -Wextra -O2 -std=c99 -D_GNU_SOURCE

# test/Makefile.am
AM_CFLAGS = -W -Wall -Wextra -std=c99 -D_GNU_SOURCE -I$(top_srcdir)/src -I$(top_builddir)
```

`dive` gets **neither**. There is no `dive_CFLAGS` and no `AM_CFLAGS` in
`src/Makefile.am`, so it compiles with whatever the environment's `CFLAGS`
holds — typically `-g -O2` with no warnings enabled and no `-std=c99`.

Consequences:

- The unused `double stop = 0.0;` in [`dive.c`](/components/dive-cli.md) is
  never diagnosed.
- `dive.c` compiles under whatever the compiler's default dialect is rather than
  c99 (recent GCC defaults have moved from gnu17 to gnu23). It works regardless
  because the file `#define _GNU_SOURCE` itself before any include, which is
  what `getline()` and `M_LN2` need.
- The library is `-O2` while `dive` and the tests inherit the environment's
  optimisation level, so a debug build mixes them.

Setting `AM_CFLAGS` in `src/Makefile.am` to match would close all three. It is a
build-file change, not an algorithm change, so it falls outside the
[integrity policy](/decisions/algorithm-integrity-policy.md).

Note also `-W` is the deprecated spelling of `-Wextra`, so it is specified
twice.

# CI

`.travis.yml` targets travis-ci.org, which has been shut down since 2021:

```yaml
language: c
compiler: gcc
install: ./bootstrap.sh
script:
  - ./configure && make
  - python test/test_all_of_the_units.py -v
```

Two problems beyond the dead service. It invokes `python`, which on any modern
image is either absent or Python 3 — the scripts were ported to Python 3 in
March 2026 and their shebangs updated, but this invocation was not. And it runs
only the Python test, never `make check`, so **the entire C test suite has never
run in CI.** The README still renders a Travis build badge.

Replacing this with a GitHub Actions workflow that runs `make check` is the
single highest-value tooling change available. See
[the test suite](/tooling/test-suite.md).

# Layout

```
configure.ac          Makefile.am           (SUBDIRS = src test)
bootstrap.sh          src/Makefile.am       (library + dive)
                      test/Makefile.am      (test_buhlmann)
```

`tools/` is not part of the build at all — it is a Python virtualenv, see
[`visoutput.py`](/tooling/visoutput.md).
