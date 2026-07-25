---
type: C Module
title: otu.c — oxygen toxicity units
description: NOAA OTU accumulation for constant and changing ppO2; correct since the 2026-03 fixes, but undeclared and unused.
resource: https://github.com/BurninChrome/diveplan-core/blob/main/src/otu.c
tags: [otu, oxygen-toxicity, not-integrated, source, layer-3]
timestamp: '2026-07-25T09:30:00Z'
---

# Signatures

```c
double otu_const(double time, double o2_ratio);
double otu_descend(double time, double o2_ratio_i, double o2_ratio_f);
```

**Neither is declared in `buhlmann.h`.** They have external linkage and are
linked into the library, but a caller must declare them itself. That is how
[the test suite](/tooling/test-suite.md) reaches them:

```c
extern double otu_const(double time, double o2_ratio);
extern double otu_descend(double time, double o2_ratio_i, double o2_ratio_f);
```

`otu.c` is also the only module that includes neither `buhlmann.h` nor
`config.h` — just `<math.h>`. It is effectively a separate mini-library that
happens to be in the same archive.

# `otu_const()` — constant ppO₂

```c
if (o2_ratio > 0.5)
    otu = time * pow((0.5 / (o2_ratio - 0.5)), (-5.0/6.0));
```

Correct. Returns 0 at or below the 0.5 bar threshold, and exactly `time` at
1.0 bar. Theory: [oxygen toxicity](/domain/oxygen-toxicity.md).

# `otu_descend()` — linearly changing ppO₂

```c
if (o2_ratio_i > 0.5 || o2_ratio_f > 0.5) {
    double o2_i = fmax(o2_ratio_i, 0.5);
    double o2_f = fmax(o2_ratio_f, 0.5);
    if (o2_f != o2_i)
        otu = ((3.0/11.0)*time)/(o2_ratio_f-o2_ratio_i)
              * (pow((o2_f-0.5)/0.5, 11.0/6.0) - pow((o2_i-0.5)/0.5, 11.0/6.0));
}
```

The `fmax` clamps keep both `pow()` bases non-negative, which is what stops a
threshold crossing producing NaN. Note the divisor deliberately uses the
**unclamped** values so the result is scaled against the full pressure
excursion. See [the ADR](/decisions/2026-03-10-fix-otu-descend-nan.md).

The `o2_f != o2_i` guard catches the case where both endpoints clamp to 0.5,
which would be 0/0.

# Repair history

This file has been fixed twice and both fixes are worth remembering:

1. **Integer division, 2026-03-09.** The exponents were written `-5/6`, `3/11`
   and `11/6` — integer expressions evaluating to `0`, `0` and `1`. The result
   was that `otu_const()` returned `time` regardless of ppO₂ and
   `otu_descend()` returned `0.0` always, silently, with no warning at any
   optimisation level. See
   [the modernisation ADR](/decisions/2026-03-09-modernize-build-and-python3.md).
2. **NaN on threshold crossing, 2026-03-10.** Described above.

Both were silent-wrong-answer bugs in safety-critical arithmetic that survived
years in the tree. Both are the reason
[the test suite](/tooling/test-suite.md) now pins the exact formulas rather than
just their signs.

# The parameter name is wrong

`o2_ratio` suggests a gas fraction (0.21 for air). The physics requires
**partial pressure in bar**. A caller passing fractions gets zero OTU for any
mix under 50% O₂ no matter how deep — 32% nitrox at 40 m has a ppO₂ of 1.6 bar
and would score nothing.

The unit tests pass 0.6, 0.8 and 1.0, which are readable either way, so they do
not disambiguate. Any integration must pass `fO2 × P_ambient`.

# Integration status

**Not integrated.** No call from [`dive.c`](/components/dive-cli.md), no field
in the [output format](/interfaces/dive-stdio-format.md). Wiring OTU in means
adding an accumulator to the dive loop and extending the output contract — the
latter breaks [`visoutput.py`](/tooling/visoutput.md), which indexes the
trailing fields positionally.

# Test coverage

`test_otu_const()` and `test_otu_descend()` between them cover both sides of the
threshold, all four crossing combinations, time linearity, and exact-value
checks against hand-computed expressions. Strong — and specifically hardened
against the two historical bugs.
