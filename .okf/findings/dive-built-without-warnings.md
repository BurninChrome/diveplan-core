---
type: Finding
title: The dive binary compiles with no warning flags
description: src/Makefile.am sets CFLAGS on the library and test/Makefile.am on the tests, but the dive program gets neither — so -Wall -Wextra never sees the driver.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/Makefile.am
tags: [finding, build, quality, unfixed]
severity: low
status: reported-awaiting-approval
timestamp: '2026-07-25T14:30:00Z'
---

Build configuration only — outside the
[integrity policy](/decisions/algorithm-integrity-policy.md).

# The gap

Three flag sets are in play, and one target has none:

```makefile
# src/Makefile.am
libbuhlmann_la_CFLAGS = -W -Wall -Wextra -O2 -std=c99 -D_GNU_SOURCE

# test/Makefile.am
AM_CFLAGS = -W -Wall -Wextra -std=c99 -D_GNU_SOURCE -I$(top_srcdir)/src -I$(top_builddir)
```

`src/Makefile.am` declares no `AM_CFLAGS` and no `dive_CFLAGS`, so `dive`
compiles with whatever the environment's `CFLAGS` holds — typically `-g -O2`,
with **no warnings enabled and no `-std=c99`**.

Automake's per-target `_CFLAGS` does not cascade to sibling targets, so adding
the library's flags did not cover the program built beside it.

# Why it matters

`dive.c` is where the model is assembled — the table selection, the argument
order, the `fmax`/`fmin` aggregation. It is the least tested file in the
repository ([no C test exercises `main()`](/tooling/test-suite.md)) and the only
one the compiler is not checking either.

It already hides at least one diagnostic: `double stop = 0.0;` is declared and
never used, which `-Wunused-variable` would flag. That variable is a remnant of
the stop scheduling that [`stop.c`](/components/stop.md) never grew, so the
warning would have been a useful signal rather than noise.

The `if (!l)` branch at line 37 is likewise unreachable — `getline()` returns
−1 or ≥ 1, never 0 — though most compilers would not diagnose that.

# Secondary effects

**Dialect.** `dive.c` builds under the compiler's default dialect rather than
c99. It works anyway because the file `#define _GNU_SOURCE` before any include,
which is what `getline()` and `M_LN2` need — but the library and the program are
compiled under different language rules, which is a latent portability trap.

**Optimisation.** The library is pinned at `-O2` while `dive` and the tests
inherit the environment, so a debug build mixes optimisation levels across the
link.

**`-W` is redundant.** It is the deprecated spelling of `-Wextra`, so both flag
sets specify it twice.

# Proposed fix

**Not applied.** Add to `src/Makefile.am`:

```makefile
AM_CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE
```

which covers `dive` (and anything added later) while the existing
`libbuhlmann_la_CFLAGS` continues to override for the library. Drop the
redundant `-W` from both files while there.

Then, in CI only, add `-Werror` — see
[the CI finding](/findings/ci-never-runs-c-tests.md). Do it in that order: turning
on warnings first surfaces the existing ones, and `-Werror` before they are
cleared would break the build.

# Impact of fixing

No behavioural change — these are diagnostics, not semantics. Expect at least
one new warning (`stop`), possibly more once `-Wextra` sees the driver for the
first time.

# Related

- [Build system](/tooling/build-system.md) — full description.
- [`dive` CLI](/components/dive-cli.md) — the unused variable and dead branch.
- [CI never runs the C tests](/findings/ci-never-runs-c-tests.md) — the
  precondition for `-Werror` being meaningful.
