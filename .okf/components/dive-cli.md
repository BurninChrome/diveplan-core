---
type: CLI
title: src/dive — the simulation driver
description: The 95-line main loop that reads a dive profile from stdin, drives all 16 compartments, and writes model state per step.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/dive.c
tags: [cli, driver, main-loop, source, layer-4]
timestamp: '2026-07-25T09:30:00Z'
---

# What it is

`src/dive` is the only executable. It is `noinst_PROGRAMS`, so `make install`
does not install it — it is a harness for the library, not a product. It links
`libbuhlmann.la` statically.

No arguments, no options, no configuration. Everything is compiled in.

```bash
python3 test/gen_dive.py -d 20 -t 5 | src/dive
```

I/O contract: [stdio format](/interfaces/dive-stdio-format.md).

# Structure

```c
struct compartment_state s[ZH_L12_NR_COMPARTMENTS];   /* 16 */
lastt = 0.0; lastp = 1.0;

for each compartment:                                  /* surface init */
    s[i].he_p = ventilation(1.0, BUHLMANN_RQ, 0.0);        /* = 0.0      */
    s[i].n2_p = ventilation(1.0, BUHLMANN_RQ, 0.78084);    /* = 0.731881 */

while (getline(...)) {
    sscanf("%lf %lf %lf %lf", &t, &p, &o2, &he);
    dt = t - lastt;  dp = p - lastp;
    if (dt < 0) { warn; continue; }

    print t, p;
    for each compartment i:
        if (dt) compartment_descend(&zh_l12[i], &s[i], &s[i],
                                    lastp, dp/dt, dt,
                                    BUHLMANN_RQ, 1.0-o2-he, he);
        print s[i].n2_p, s[i].he_p;
        ceiling   = fmax(getCeiling(&zh_l12[i], &s[i]), ceiling);
        nodectime = fmin(nodecotime(&zh_l12[i], &s[i], lastp,
                                    1.0-o2-he, he), nodectime);
    print ceiling, nodectime;

    lastp = p; lastt = t;
}
```

# Design decisions baked in here

**Always Schreiner, never Haldane.** Every step goes through
[`compartment_descend()`](/components/compartment.md) even when `dp = 0`. Valid,
because Schreiner reduces to Haldane at zero rate — see
[inert gas loading](/domain/inert-gas-loading.md). It makes
[`haldane.c`](/components/haldane.md) unreachable from the CLI.

**ZH-L12 is hard-wired.** `zh_l12` is named directly three times and
`ZH_L12_NR_COMPARTMENTS` sizes the state array. Switching to ZH-L16C is a source
edit, not a flag. See
[the finding](/findings/dive-uses-zh-l12-not-zh-l16c.md).

**Aliased in-place update.** `&s[i]` is passed as both `cur` and `end`. Safe —
`compartment_descend()` reads both fields before writing either.

**Ceiling aggregates with `fmax`, NDL with `fmin`.** Correct: the most
restrictive compartment governs the ceiling, the first to saturate governs the
NDL. Both are seeded per line (`ceiling = 0.0`, `nodectime = 100.0`).

**The `fmin` seed is the only thing enforcing the 100-minute cap.**
[`nodecotime()`](/components/stop.md) can itself return more — its loop guard is
`Addtime < 101`, so a candidate of 75 that passes the ceiling check becomes
112.5 and is returned. That happens in 147 of the depth/compartment
combinations sampled between 5 m and 30 m. The seed clamps it before anyone
sees it, but a caller using `nodecotime()` directly must not assume the
documented 100-minute bound.

# Rough edges

**`nodecotime()` is called with `lastp`, not `p`.** The NDL is evaluated at the
depth of the *previous* sample. On flat segments they are equal; during a
descent it reports the NDL for a shallower depth than the diver is at. Compare
`compartment_descend()`, where `lastp` is genuinely correct because Schreiner
wants the start-of-interval pressure. Discussed in
[the nodecotime finding](/findings/nodecotime-overestimates-ndl.md).

**`double stop = 0.0;` is declared and never used**, a remnant of the
stop-scheduling that [`stop.c`](/components/stop.md) never grew. It survives
`-Wall -Wextra` only because `dive` is built without the library's CFLAGS — see
[build system](/tooling/build-system.md).

**`if (!l) { n++; continue; }` is dead.** `getline()` returns −1 at EOF (already
handled by the `while` condition) or at least 1 otherwise; it never returns 0.

**Malformed lines are skipped, not fatal.** A line failing the 4-field `sscanf`
prints to stderr and continues, leaving `lastt`/`lastp` untouched, so the next
good line integrates across the gap. Reasonable. Negative time intervals are
rejected the same way; note the check is `dt < 0`, so a repeated timestamp
(`dt == 0`) is accepted and the `if (dt)` guard makes it a no-op that still
emits an output line.

**`n` counts lines but starts at 0 and is only used in error messages**, which
therefore report 0-based line numbers.

**Output is `%lf`** — six decimal places, roughly 1e-6 bar ≈ 0.01 mm of water.
Ample, but it is fixed-point formatting: a tissue pressure of 1e-9 prints as
`0.000000`.

# Commented-out remnants

Three blocks of dead code survive: an alternative `nostoptime()` call, a
conditional NDL print, and a "Stop recommanded at" print that would have
converted a stop pressure to metres. They document intent that was never
finished — see [`stop.c`](/components/stop.md).

# Not covered by tests

The [C test suite](/tooling/test-suite.md) tests library functions, never
`main()`. There is no end-to-end test that feeds a profile in and checks the
output. The Python test tests only [`gen_dive.py`](/tooling/gen-dive.md). So the
loop assembly — argument order, the `lastp` choice, the aggregation seeds, the
output field order — is entirely unverified.
