---
type: C Module
title: buhlmann.c — the empty translation unit
description: A source file containing only includes, which nonetheless anchors the autoconf configuration.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/buhlmann.c
tags: [source, dead-code, build]
timestamp: '2026-07-25T09:30:00Z'
---

# The whole file

```c
#include <math.h>
#include "buhlmann.h"
#include <config.h>
```

No declarations, no definitions. It compiles to an object file with no symbols
and is linked into `libbuhlmann.la` for nothing.

# Why it cannot simply be deleted

`configure.ac` names it as the sanity-check file:

```
AC_CONFIG_SRCDIR([src/buhlmann.c])
```

`AC_CONFIG_SRCDIR` exists so `configure` can verify it is being run against the
right source tree. Deleting the file without changing that line makes
`./configure` fail with "cannot find sources". Point it at
`src/compartment.c` — or at `src/buhlmann.h`, which is what the library is
really named after — before removing it.

It is also listed in `libbuhlmann_la_SOURCES` in `src/Makefile.am`, which would
need the same edit. See [build system](/tooling/build-system.md).

# Is it strictly legal C?

Yes. A translation unit containing no external declarations violates the syntax
rule in C11 §6.9 and requires a diagnostic (§5.1.1.3) — it is ill-formed, not
undefined behaviour, and GCC reports it as "ISO C forbids an empty translation
unit" under `-Wpedantic`.

This file escapes even that: `<math.h>` and `buhlmann.h` both contribute
declarations, so the TU is not empty in the standard's sense. It compiles
cleanly under `-std=c99 -Wall -Wextra`.

# Judgement

Harmless, but it is the file a newcomer opens first when looking for "the
Bühlmann implementation", and it tells them nothing. If the module structure is
ever revisited, this is the natural home for a `buhlmann_step()` entry point
that owns the loop currently living in
[`dive.c`'s `main()`](/components/dive-cli.md) — the gap identified in
[architecture](/components/architecture.md).
