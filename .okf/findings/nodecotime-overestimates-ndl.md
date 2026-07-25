---
type: Bug Report
title: nodecotime() over-reports the no-decompression limit by 1.5–1.7×
description: The NDL search returns the next untested candidate rather than the validated one, and accepts a 1 m ceiling as no-deco; both errors are optimistic.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/stop.c
tags: [bug, safety-critical, ndl, nodecotime, unfixed]
severity: high
status: reported-awaiting-approval
timestamp: '2026-07-25T09:30:00Z'
---

> Filed under the [algorithm integrity policy](/decisions/algorithm-integrity-policy.md).
> **No code has been changed.** This report exists so a human can decide.

# Affected code

`nodecotime()` in [`src/stop.c`](/components/stop.md), lines 18–66. Reached from
[`dive.c`](/components/dive-cli.md) line 69, once per compartment per input
line, and surfaced as field 35 of
[the output format](/interfaces/dive-stdio-format.md).

# Erroneous behaviour

The reported no-decompression limit is 1.5–1.7× longer than the model's own
exact NDL, at every depth, for both shipped constant tables. The error is
**optimistic** — it tells the diver they may stay longer than the algorithm
itself permits.

Minimum across all compartments, air, surface-saturated start:

| Depth | Exact model NDL | `nodecotime()` | Over-report |
|---:|---:|---:|---:|
| 20 m | 55.40 min | 84.38 min | +52% |
| 30 m | 19.30 min | 31.64 min | +64% |
| 40 m | 7.53 min | 11.87 min | +58% |
| 50 m | 4.46 min | 7.03 min | +58% |
| 60 m | 3.26 min | 5.01 min | +54% |

ZH-L16C behaves the same way (+57% to +66%).

At 30 m the function reports 31.6 minutes of no-decompression time where the
model permits 19.3. A diver following that figure surfaces with a real
decompression obligation and no stop having been made.

# Root cause

Two independent errors, both optimistic, compounding.

### 1. The return value is the next untested candidate

```c
while (iter < 100 && Addtime < 101 && (ceiling < 0.99 || ceiling > 1.1)) {
    compartment_stagnate(..., Addtime, ...);
    ceiling = getCeiling(constants, &end);

    if (ceiling > 1.1) Addtime = Addtime * .5;
    else               Addtime = Addtime * 1.5;   /* ← mutates after the check */
    iter++;
}
return Addtime;                                    /* ← returns the mutation   */
```

The loop condition is evaluated against `ceiling`, but `Addtime` has already
been multiplied past the value that produced it. When the loop exits *on the
ceiling band* — the normal case — the last branch taken was necessarily the
`else`, so the returned value is exactly **1.5× the accepted candidate**. (The
loop can also exit on its `iter` or `Addtime < 101` guards, in which case the
returned value is unrelated to any validated time at all; that path is how the
function comes to return 112.5 for some inputs.)

Traced, ZH-L12 compartment 1 at 50 m:

```
  it 1 tested=100.0000 ceiling=1.99759 -> next= 50.0000
  it 2 tested= 50.0000 ceiling=1.99758 -> next= 25.0000
  it 3 tested= 25.0000 ceiling=1.99296 -> next= 12.5000
  it 4 tested= 12.5000 ceiling=1.87586 -> next=  6.2500
  it 5 tested=  6.2500 ceiling=1.37332 -> next=  3.1250
  it 6 tested=  3.1250 ceiling=0.58389 -> next=  4.6875
  it 7 tested=  4.6875 ceiling=1.05816 -> next=  7.0312   ← exits here
  RETURNS 7.0312, having validated 4.6875
```

### 2. The acceptance band already contains a decompression obligation

The loop stops when `0.99 <= ceiling <= 1.1`. A ceiling of 1.1 bar is a
**1 metre decompression stop**. Accepting it as "no decompression required"
means the search targets the wrong crossing point before the 1.5× is even
applied. The correct target is `ceiling == 1.0`.

That is where the residual beyond 1.5× comes from — the measured factors run to
1.66×, not a flat 1.50×.

### Contributing: the wrong ambient pressure

Separately, [`dive.c`](/components/dive-cli.md) line 69 passes `lastp` — the
*previous* sample's pressure — rather than the current `p`:

```c
nodectime = fmin(nodecotime(&zh_l12[i], &s[i], lastp, 1.0-o2-he, he), nodectime);
```

During descent this evaluates the NDL for a shallower depth than the diver has
reached, adding a third optimistic bias. On flat segments `lastp == p`, which is
most of a square profile and is why it has gone unnoticed. Note the same `lastp`
is genuinely correct for `compartment_descend()` on the line above, which wants
the start-of-interval pressure — the two calls need different values.

# Why the tests pass

`test_nodecotime()` makes three assertions: surface ≈ 100, 50 m in `(0, 100)`,
and deeper < shallower. All three hold under a uniform 1.6× scale error. No test
compares the result to an independently computed NDL. See
[test suite](/tooling/test-suite.md).

# Reproduction

Originally derived by transcribing `alveolar.c`, `haldane.c`, `ceiling.c`,
`compartment.c` and `stop.c` into Python line for line — including `stop.c`'s
control flow — and bisecting the ceiling crossing for the reference, because no
C toolchain was available at authoring time.

**Since confirmed against the compiled library.** A harness linked against
`libbuhlmann.a` reproduces the ratios exactly: 1.52 / 1.64 / 1.58 / 1.58 / 1.54×
at 20–60 m, and the 112.5 maximum. Method and full confirmation table:
[verification method](/decisions/2026-07-25-bundle-verification-method.md).

Verify on a machine with a compiler by adding to `test_buhlmann.c`:

```c
/* Reference NDL by bisection on the ceiling crossing. */
static double ndl_reference(const struct compartment_constants *c,
                            struct compartment_state s0, double pamb)
{
    struct compartment_state e;
    double lo = 0.0, hi = 100.0;
    compartment_stagnate(c, &s0, &e, pamb, hi, BUHLMANN_RQ, 0.78084, 0.0);
    if (getCeiling(c, &e) <= 1.0) return hi;
    for (int k = 0; k < 60; k++) {
        double mid = 0.5 * (lo + hi);
        compartment_stagnate(c, &s0, &e, pamb, mid, BUHLMANN_RQ, 0.78084, 0.0);
        if (getCeiling(c, &e) > 1.0) hi = mid; else lo = mid;
    }
    return lo;
}
/* At 6.0 bar, zh_l12[0]: reference ≈ 4.46, nodecotime() ≈ 7.03. */
```

# Proposed fix

Two edits inside `nodecotime()`. **Not applied.**

1. Keep the last validated candidate and return that, not the probe:

```c
double accepted = 0.0;
while (...) {
    compartment_stagnate(constants, compt, &end, p_ambient, Addtime,
                         BUHLMANN_RQ, n2_ratio, he_ratio);
    ceiling = getCeiling(constants, &end);
    if (ceiling <= 1.0) accepted = Addtime;      /* provably safe so far */
    Addtime = (ceiling > 1.0) ? Addtime * 0.5 : Addtime * 1.5;
    iter++;
}
return accepted;
```

2. Target `ceiling == 1.0` rather than the `[0.99, 1.1]` band.

A cleaner alternative, and my recommendation: **replace the geometric probe with
a bracketed bisection** on `[0, 100]`, as in the reference above. It is
deterministic, converges to any requested tolerance in a fixed 40–60 iterations,
cannot return an unvalidated value by construction, and removes the `iter < 100`
and `Addtime < 101` guards entirely. It is also *cheaper* than the current
worst case.

Best of all is the closed form: the ceiling crossing can be solved analytically
by inverting the Haldane equation for `t`, giving an exact NDL in one step with
no iteration at all. That is a larger change and would want its own review.

# Impact of fixing

- **Field 35 of the output changes for essentially every dive**, dropping by
  ~35–40%. Any recorded baseline must be regenerated.
- [`visoutput.py`](/tooling/visoutput.md)'s NDL readout changes correspondingly.
  Its separate decimal-to-seconds bug is unaffected.
- No other function calls `nodecotime()`; the blast radius is field 35 and its
  consumers.
- `test_nodecotime()` continues to pass unchanged — which is itself the argument
  for adding the reference-based assertion at the same time.

# Recommendation

Fix, with the bisection rewrite, together with a reference-value test. Separately
change `dive.c` line 69 to pass `p`. Until then, treat field 35 as
**unsafe for dive planning** and document that in the README.
