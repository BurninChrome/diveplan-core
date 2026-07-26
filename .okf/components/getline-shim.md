---
type: C Module
title: getline.c — portability shim
description: A fallback getline() for platforms lacking the POSIX 2008 function, compiled only when autoconf does not find one.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/getline.c
tags: [portability, shim, source]
timestamp: '2026-07-25T09:30:00Z'
---

# Purpose

`getline()` is POSIX.1-2008 and a GNU extension before that. `configure.ac`
probes for it:

```
AC_CHECK_FUNCS([getline])
```

and `getline.c` wraps its whole body in `#ifndef HAVE_GETLINE`, so on Linux and
any modern BSD or macOS the file compiles to nothing. Given the project's
AquaBSD origin, this was aimed at older BSD userlands.

It is a source of `dive` only (`dive_SOURCES = dive.c getline.c`), not of the
library.

# Semantics of the fallback

Grows a buffer geometrically from 16 bytes, reads to `\n` inclusive or EOF,
NUL-terminates, and returns the character count excluding the terminator.
Returns −1 at EOF-with-nothing-read or on allocation failure. That matches the
POSIX contract closely enough for [`dive.c`](/components/dive-cli.md)'s use,
which is a `while (getline(...) != -1)` loop feeding `sscanf`.

# Where it diverges from POSIX

Worth knowing if this ever runs somewhere real:

- **`*lineptr` and `*n` are only written on success.** POSIX updates them
  whenever the buffer is reallocated, including on the failure path. A caller
  that frees `*lineptr` after a −1 return can double-free the buffer this
  function already freed. `dive.c` is safe: it frees once, after the loop, and
  the shim leaves `*lineptr` untouched on the alloc-failure path where it freed
  its own local copy — but that local copy came from `realloc(*lineptr, ...)`,
  so `*lineptr` is left dangling. Only reachable under memory exhaustion.
- **No `NULL`/zero-size validation** of the arguments.
- **Does not distinguish EOF from error**; POSIX asks for `errno` to be set.
- The growth check `(l + 2) > buflen` reserves room for the character plus the
  NUL, which is correct, though it reallocates one iteration earlier than
  strictly necessary.

# Practical status

Dead code on every platform this is likely to be built on today. Verifying it
would require deliberately defining `HAVE_GETLINE` to 0, which nothing does. It
carries no test coverage.
