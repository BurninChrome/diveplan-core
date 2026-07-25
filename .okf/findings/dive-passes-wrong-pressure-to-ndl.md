---
type: Bug Report
title: dive.c evaluates the NDL at the previous sample's depth
description: nodecotime() is called with lastp rather than p, so during descent the no-decompression limit is computed for a shallower depth than the diver has reached.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/dive.c
tags: [bug, ndl, dive-cli, unfixed]
severity: medium
status: reported-awaiting-approval
timestamp: '2026-07-25T14:30:00Z'
---

> The affected file, `dive.c`, is not on the
> [integrity policy](/decisions/algorithm-integrity-policy.md)'s protected list,
> but this change alters model output, so it should be treated as covered.
> **No code has been changed.**

# Affected code

[`src/dive.c`](/components/dive-cli.md) line 69, inside the per-compartment
loop:

```c
compartment_descend(&zh_l12[i], &s[i], &s[i],
                    lastp, dp / dt, dt, BUHLMANN_RQ, 1.0-o2-he, he);   /* line 60 */
...
nodectime = fmin(nodecotime(&zh_l12[i], &s[i], lastp, 1.0-o2-he, he),  /* line 69 */
                 nodectime);
```

# The defect

The two calls need *different* pressures, and both are given `lastp`.

`compartment_descend()` is correct: the
[Schreiner equation](/domain/inert-gas-loading.md) integrates a ramp from the
start of the interval, so it wants the start-of-interval pressure, which is
`lastp`.

[`nodecotime()`](/components/stop.md) asks "how much longer may the diver stay
**here**". Here is `p`, the depth the sample has just reached, not `lastp`, the
depth they left. Passing `lastp` computes the no-decompression limit for a depth
the diver is no longer at.

# Direction and magnitude

The bias follows the direction of travel:

- **Descending** — `lastp < p`, so the NDL is evaluated shallower than actual and
  the reported figure is **too long**. Optimistic.
- **Ascending** — `lastp > p`, evaluated deeper than actual, reported figure too
  short. Conservative.
- **Flat** — `lastp == p`, no difference.

Square profiles are mostly flat, which is why this has gone unnoticed. At the
0.1-minute sampling [`gen_dive.py`](/tooling/gen-dive.md) emits with a 20 m/min
descent, each descent step spans 2 m, so the error is bounded by roughly one
sample of depth — small per line, but systematic and always optimistic on the
way down.

The magnitude is minor next to
[the 1.5–1.7× scale error in `nodecotime()` itself](/findings/nodecotime-overestimates-ndl.md),
and the two compound in the same direction. Both should be fixed together;
fixing only this one would be immaterial.

# Proposed fix

**Not applied.** One token:

```c
nodectime = fmin(nodecotime(&zh_l12[i], &s[i], p, 1.0-o2-he, he), nodectime);
```

Leave line 60's `lastp` alone — it is correct.

# Impact of fixing

Field 35 of [the output](/interfaces/dive-stdio-format.md) changes on descent
and ascent lines only; flat segments are bit-identical. The change is small and
in the conservative direction during descent.

No test covers it: the C suite never exercises `main()`, so the argument list of
this call is unverified. See [test suite](/tooling/test-suite.md). A golden-file
regression over `test/xml/` would pin it.
