---
type: C Module
title: stop.c — nodecotime()
description: The iterative no-decompression-limit search; the most expensive and least correct function in the library, and not the stop scheduler its filename suggests.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/stop.c
tags: [ndl, nodecotime, safety-critical, source, layer-3]
timestamp: '2026-07-25T09:30:00Z'
---

> **Descriptive, not normative.** This page documents what `stop.c` does,
> including behaviour that is wrong. Do not reimplement from it. The
> specification for what stop scheduling *should* do is
> [ascent and stop scheduling](/domain/ascent-and-stop-scheduling.md).

# The filename is misleading

`stop.c` contains no decompression-stop scheduling. There is no function that
computes stop depths, stop durations, total ascent time or a runtime. The
repository's own `CLAUDE.md` describes this file as "decompression stop depth
and duration"; that describes an intent, not the code.

What it actually contains is one function: an NDL search.

# Signature

```c
double nodecotime(const struct compartment_constants *constants,
                  struct compartment_state *compt,
                  double p_ambient,
                  double n2_ratio, double he_ratio);
```

Returns, in minutes, an estimate of how much longer this compartment can stay at
`p_ambient` before developing a ceiling. Not declared in a header comment
anywhere; the semantics above are inferred from the body and confirmed
numerically.

# How it searches

```c
Addtime = 100; iter = 0;
end = stagnate(compt, Addtime); ceiling = getCeiling(end);
if (ceiling <= 1.0) return Addtime;                    // never deco → cap

while (iter < 100 && Addtime < 101 && (ceiling < 0.99 || ceiling > 1.1)) {
    end = stagnate(compt, Addtime); ceiling = getCeiling(end);
    Addtime = (ceiling > 1.1) ? Addtime * 0.5 : Addtime * 1.5;
    iter++;
}
return Addtime;
```

A geometric probe, not a bisection: ×0.5 when overshooting, ×1.5 when
undershooting. It always re-integrates from the *original* `compt`, so there is
no accumulation error, and it does converge — the ×0.5/×1.5 product of 0.75 per
overshoot-undershoot pair contracts.

# It returns the wrong value

**The returned `Addtime` is the next untested candidate, not the one that
satisfied the loop condition.** The multiplication happens after the ceiling
check and before the loop re-tests, so the function exits carrying a value 1.5×
larger than the time it actually validated.

Traced on ZH-L12 compartment 1 at 50 m:

```
  it 1 tested=100.0000 ceiling=1.99759 -> next= 50.0000
  it 2 tested= 50.0000 ceiling=1.99758 -> next= 25.0000
  it 3 tested= 25.0000 ceiling=1.99296 -> next= 12.5000
  it 4 tested= 12.5000 ceiling=1.87586 -> next=  6.2500
  it 5 tested=  6.2500 ceiling=1.37332 -> next=  3.1250
  it 6 tested=  3.1250 ceiling=0.58389 -> next=  4.6875
  it 7 tested=  4.6875 ceiling=1.05816 -> next=  7.0312   ← loop exits
  RETURNS 7.0312   (the accepted candidate was 4.6875)
```

Compounding it, the acceptance band `[0.99, 1.1]` treats a ceiling of 1.1 bar —
a **1 metre decompression obligation** — as "no decompression required". So the
function accepts a time that is already slightly into deco, then multiplies it
by 1.5.

Net effect against the model's own exact NDL: **1.5–1.7× too long, at every
depth.** Full write-up, reproduction and proposed fix:
[nodecotime over-reports NDL](/findings/nodecotime-overestimates-ndl.md).

# Other properties worth knowing

**The 100-minute cap is a sentinel, and it is not enforced here.** The early
return fires whenever 100 minutes at this depth produces no ceiling, so `100.0`
means "no obligation within 100 min", not a measurement. Consumers must not
average or plot it as a quantity. See [NDL](/domain/no-decompression-limit.md).

But the function can also return *more* than 100: the loop guard is
`Addtime < 101`, so a candidate of 75 that clears the ceiling check is
multiplied to 112.5 and returned. That occurs in 147 of the depth/compartment
combinations sampled between 5 m and 30 m. Only
[`dive.c`](/components/dive-cli.md)'s `fmin(…, 100.0)` seed keeps the published
output bounded.

**A wasted first integration.** The pre-loop `stagnate` at `Addtime = 100` is
immediately repeated as the loop's first iteration with the identical argument.
One redundant pair of `exp()` calls per compartment per step.

**Called with the wrong pressure.** [`dive.c`](/components/dive-cli.md) passes
`lastp`, the *previous* line's ambient pressure, not the current `p`. During
descent that evaluates the NDL at a shallower depth than the diver is at, so the
reported figure is optimistic on top of the 1.5× error. During ascent it errs
conservative. On flat segments `lastp == p` and it makes no difference — which
is most of a square profile, and is why this has gone unnoticed.

**It dominates runtime.** Up to 101 iterations × 2 `exp()` calls × 16
compartments, per input line. See [architecture](/components/architecture.md).

**Dead scaffolding.** The file opens with the full `compartment_stagnate()`
prototype commented out, carries `#include <stdio.h>` solely for a commented-out
`printf`, and holds French-language comments (`// Surface donc 99`, `// trop de
celing ou trop en surface`) from the original author.

# Test coverage

`test_nodecotime()` asserts three things: the surface case returns ≈100, 50 m
returns something in `(0, 100)`, and deeper is shorter. All three pass with the
1.5× error present, because none of them checks the value against an
independently computed NDL. This is the clearest example in the repository of
tests that pin behaviour rather than correctness.
